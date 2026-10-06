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
        COPY_FAILED,
        OUT_OF_MEMORY,
        NO_TRANSPORT,
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
