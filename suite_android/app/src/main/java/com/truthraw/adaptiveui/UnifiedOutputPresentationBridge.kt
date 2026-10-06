package com.truthraw.adaptiveui

import android.graphics.Bitmap

/**
 * Lifetime-safe downstream bridge from an already-rendered Unified Output
 * preview into presentation consumers such as Workspace / Free Raster.
 *
 * Scientific boundary:
 * - never renders or reconstructs pixels;
 * - never infers authority;
 * - never writes Scientific Master state;
 * - never changes the upstream bitmap;
 * - copies pixels only to establish independent presentation ownership.
 *
 * Cable rule: stable outside, flexible inside. The public publication/acquire
 * boundary stays stable while transport implementations remain replaceable.
 */
internal object UnifiedOutputPresentationBridge {
    const val ORIGIN_UNIFIED_OUTPUT_READY = "UNIFIED_OUTPUT_PREVIEW_READY"
    const val PRESENTATION_CONTRACT_ID = "draw.unified_output.presentation_snapshot.v0.1"

    const val META_ORIGIN = "origin"
    const val META_SOURCE_JOB_ID = "source_job_id"
    const val META_SOURCE_DISPLAY_NAME = "source_display_name"
    const val META_SOURCE_URI = "source_uri"
    const val META_SOURCE_SHA256 = "source_sha256"
    const val META_SOURCE_BINDING_KIND = "source_binding_kind"
    const val META_ROUTE = "route"
    const val META_OUTPUT_LABEL = "output_label"
    const val META_PREVIEW_WIDTH = "preview_width"
    const val META_PREVIEW_HEIGHT = "preview_height"
    const val META_SOURCE_WIDTH = "source_width"
    const val META_SOURCE_HEIGHT = "source_height"
    const val META_SOURCE_SPACE_CODE = "source_space_code"
    const val META_DISPLAY_QUARTER_TURNS = "display_quarter_turns"
    const val META_PRIMARY_TILE_SOURCE_DIRECT = "primary_tile_source_used_directly"
    const val META_APPEARANCE_ADDED = "appearance_added_by_preview"
    const val META_SCIENTIFIC_WRITEBACK_ALLOWED = "scientific_writeback_allowed"
    const val META_CREATES_NEW_EVIDENCE = "creates_new_evidence"
    const val META_PRESENTATION_LAYER = "presentation_layer"

    const val PRESENTATION_LAYER_VIEW_ONLY = "VIEW_ONLY_COPY"
    const val SOURCE_BINDING_ACTIVE_JOB = "ACTIVE_JOB_PROCESS_LOCAL"
    const val UNKNOWN_SOURCE_SHA256 = "UNKNOWN"

    data class Publication(
        val bitmap: Bitmap,
        val metadata: Map<String, String> = emptyMap(),
    )

    data class Snapshot(
        val bitmap: Bitmap,
        val metadata: Map<String, String>,
        val transportId: String,
        val contractId: String = PRESENTATION_CONTRACT_ID,
    )

    sealed interface PublishResult {
        data class Ready(
            val transportId: String,
            val width: Int,
            val height: Int,
        ) : PublishResult

        data class Failure(
            val kind: FailureKind,
            val detail: String,
        ) : PublishResult
    }

    enum class FailureKind {
        SOURCE_RECYCLED,
        CONTRACT_REJECTED,
        COPY_FAILED,
        OUT_OF_MEMORY,
        NO_TRANSPORT,
    }

    /**
     * Publish an already-accepted MainActivity Unified Output Ready state.
     *
     * sourceJobId is deliberately labelled process-local: it is a lifetime
     * binding used to prevent stale UI transport, not a cryptographic evidence
     * identity. sourceSha256 is transported only when an existing upstream
     * profile already provides it; UNKNOWN remains valid and is never promoted.
     */
    @Synchronized
    fun publishReady(
        ready: UnifiedOutputPreviewResult.Ready,
        sourceJobId: String,
        sourceDisplayName: String,
        sourceUri: String,
        sourceSha256: String?,
        route: String,
    ): PublishResult {
        if (
            sourceJobId.isBlank() ||
            sourceUri.isBlank() ||
            route.isBlank() ||
            ready.outputLabel.isBlank()
        ) {
            return PublishResult.Failure(
                kind = FailureKind.CONTRACT_REJECTED,
                detail = "Unified Output presentation mist actieve source/route/output-binding",
            )
        }

        if (ready.metrics.scientificWritebackAllowed) {
            return PublishResult.Failure(
                kind = FailureKind.CONTRACT_REJECTED,
                detail = "Unified Output presentation geweigerd: scientific writeback staat niet dicht",
            )
        }

        val normalizedSha = sourceSha256
            ?.trim()
            ?.takeIf { value ->
                value.length == 64 && value.all { it in '0'..'9' || it in 'a'..'f' || it in 'A'..'F' }
            }
            ?.lowercase()
            ?: UNKNOWN_SOURCE_SHA256
        val metrics = ready.metrics

        return publish(
            bitmap = ready.bitmap,
            metadata = linkedMapOf(
                META_ORIGIN to ORIGIN_UNIFIED_OUTPUT_READY,
                META_SOURCE_JOB_ID to sourceJobId,
                META_SOURCE_DISPLAY_NAME to sourceDisplayName,
                META_SOURCE_URI to sourceUri,
                META_SOURCE_SHA256 to normalizedSha,
                META_SOURCE_BINDING_KIND to SOURCE_BINDING_ACTIVE_JOB,
                META_ROUTE to route,
                META_OUTPUT_LABEL to ready.outputLabel,
                META_PREVIEW_WIDTH to ready.bitmap.width.toString(),
                META_PREVIEW_HEIGHT to ready.bitmap.height.toString(),
                META_SOURCE_WIDTH to metrics.sourceWidth.toString(),
                META_SOURCE_HEIGHT to metrics.sourceHeight.toString(),
                META_SOURCE_SPACE_CODE to metrics.sourceSpaceCode.toString(),
                META_DISPLAY_QUARTER_TURNS to metrics.displayQuarterTurns.toString(),
                META_PRIMARY_TILE_SOURCE_DIRECT to metrics.primaryTileSourceUsedDirectly.toString(),
                META_APPEARANCE_ADDED to metrics.appearanceAddedByPreview.toString(),
                META_SCIENTIFIC_WRITEBACK_ALLOWED to "false",
                META_CREATES_NEW_EVIDENCE to "false",
                META_PRESENTATION_LAYER to PRESENTATION_LAYER_VIEW_ONLY,
            ),
        )
    }

    /**
     * Replaceable internal transport. A transport owns every bitmap it stores
     * and must return a fresh consumer-owned bitmap from acquire().
     */
    internal interface Transport {
        val id: String

        fun publish(publication: Publication): PublishResult

        fun acquire(): Snapshot?

        fun clear(reason: String)
    }

    private val transports: List<Transport> = listOf(InMemoryCopyTransport)

    /**
     * Publish an existing rendered output. This is a copy-only presentation
     * operation. Metadata is descriptive provenance/diagnostics only and is not
     * interpreted as scientific authority.
     */
    @Synchronized
    fun publish(
        bitmap: Bitmap,
        metadata: Map<String, String> = emptyMap(),
    ): PublishResult {
        if (bitmap.isRecycled) {
            return PublishResult.Failure(
                kind = FailureKind.SOURCE_RECYCLED,
                detail = "Unified Output-bitmap was al gerecycled vóór snapshotpublicatie",
            )
        }

        val transport = transports.firstOrNull()
            ?: return PublishResult.Failure(
                kind = FailureKind.NO_TRANSPORT,
                detail = "Geen presentation snapshot-transport geregistreerd",
            )

        return transport.publish(
            Publication(
                bitmap = bitmap,
                metadata = metadata.toMap(),
            ),
        )
    }

    /**
     * Acquire a consumer-owned snapshot. The returned bitmap may safely be
     * recycled by Workspace without affecting MainActivity or bridge storage.
     */
    @Synchronized
    fun acquire(): Snapshot? {
        transports.forEach { transport ->
            transport.acquire()?.let { return it }
        }
        return null
    }

    @Synchronized
    fun clear(reason: String) {
        transports.forEach { it.clear(reason) }
    }

    /**
     * Current process-local transport. Process death intentionally removes the
     * snapshot; consumers then fail closed instead of reconstructing provenance
     * from stale UI state.
     */
    private object InMemoryCopyTransport : Transport {
        override val id: String = "process_memory_owned_copy.v1"

        private var storedBitmap: Bitmap? = null
        private var storedMetadata: Map<String, String> = emptyMap()

        override fun publish(publication: Publication): PublishResult {
            val source = publication.bitmap
            if (source.isRecycled) {
                return PublishResult.Failure(
                    kind = FailureKind.SOURCE_RECYCLED,
                    detail = "Unified Output-bitmap was gerecycled",
                )
            }

            val ownedCopy = try {
                copyBitmap(source)
            } catch (_: OutOfMemoryError) {
                return PublishResult.Failure(
                    kind = FailureKind.OUT_OF_MEMORY,
                    detail = "Onvoldoende geheugen voor onafhankelijke Unified Output-snapshot",
                )
            } catch (error: Exception) {
                return PublishResult.Failure(
                    kind = FailureKind.COPY_FAILED,
                    detail = error.message ?: error::class.java.simpleName,
                )
            }

            if (ownedCopy == null) {
                return PublishResult.Failure(
                    kind = FailureKind.COPY_FAILED,
                    detail = "Bitmapkopie voor presentation snapshot mislukte",
                )
            }

            storedBitmap?.takeUnless { it.isRecycled }?.recycle()
            storedBitmap = ownedCopy
            storedMetadata = publication.metadata.toMap()

            return PublishResult.Ready(
                transportId = id,
                width = ownedCopy.width,
                height = ownedCopy.height,
            )
        }

        override fun acquire(): Snapshot? {
            val stored = storedBitmap ?: return null
            if (stored.isRecycled) {
                storedBitmap = null
                storedMetadata = emptyMap()
                return null
            }

            val consumerCopy = try {
                copyBitmap(stored)
            } catch (_: OutOfMemoryError) {
                null
            } catch (_: Exception) {
                null
            } ?: return null

            return Snapshot(
                bitmap = consumerCopy,
                metadata = storedMetadata.toMap(),
                transportId = id,
            )
        }

        override fun clear(reason: String) {
            storedBitmap?.takeUnless { it.isRecycled }?.recycle()
            storedBitmap = null
            storedMetadata = if (reason.isBlank()) {
                emptyMap()
            } else {
                mapOf("clear_reason" to reason)
            }
        }

        private fun copyBitmap(source: Bitmap): Bitmap? {
            val config = source.config ?: Bitmap.Config.ARGB_8888
            return source.copy(config, false)
                ?: if (config != Bitmap.Config.ARGB_8888) {
                    source.copy(Bitmap.Config.ARGB_8888, false)
                } else {
                    null
                }
        }
    }
}
