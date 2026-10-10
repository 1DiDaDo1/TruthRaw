package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.net.Uri
import java.io.FileNotFoundException

/**
 * Presentation-only raster loader for Workspace / Free Raster.
 *
 * The outer contract is intentionally stable while the decoder chain is
 * flexible inside. Additional deterministic presentation decoders can be added
 * as adapters without changing the caller or creating a second scientific
 * pipeline.
 *
 * This object deliberately accepts only already-rendered image formats. It is
 * not a RAW decoder, does not create scientific authority, and may never write
 * Scientific Master state.
 */
internal object PresentationRasterLoader {
    private const val DEFAULT_MAX_DIMENSION = 2048

    sealed interface Result {
        data class Ready(
            val bitmap: Bitmap,
            val mimeType: String?,
            val sourceWidth: Int,
            val sourceHeight: Int,
            val sampleSize: Int,
            val decoderId: String,
        ) : Result

        data class Failure(
            val kind: FailureKind,
            val detail: String,
        ) : Result
    }

    enum class FailureKind {
        PERMISSION_DENIED,
        PROVIDER_UNAVAILABLE,
        UNSUPPORTED_TYPE,
        DECODE_FAILED,
        OUT_OF_MEMORY,
    }

    /**
     * Open adapter point for the inside of the presentation cable.
     *
     * A decoder may only produce a presentation raster. It may not decode RAW,
     * infer evidence authority or write Scientific Master state.
     */
    internal interface Decoder {
        val id: String

        fun supports(mimeType: String?): Boolean

        fun decode(
            resolver: ContentResolver,
            uri: Uri,
            mimeType: String?,
            maxDimension: Int,
        ): Result
    }

    private val defaultDecoders: List<Decoder> = listOf(BitmapFactoryDecoder)

    fun load(
        context: Context,
        uri: Uri,
        maxDimension: Int = DEFAULT_MAX_DIMENSION,
        decoders: List<Decoder> = defaultDecoders,
    ): Result {
        if (maxDimension <= 0) {
            return Result.Failure(
                kind = FailureKind.DECODE_FAILED,
                detail = "Ongeldige maximale preview-afmeting: $maxDimension",
            )
        }
        if (decoders.isEmpty()) {
            return Result.Failure(
                kind = FailureKind.UNSUPPORTED_TYPE,
                detail = "Geen presentatie-decoderadapter beschikbaar",
            )
        }

        val resolver = context.contentResolver
        persistReadPermissionBestEffort(resolver, uri)

        val mimeType = resolver.getType(uri)?.lowercase()
        val matchingDecoders = decoders.filter { decoder ->
            try {
                decoder.supports(mimeType)
            } catch (_: Exception) {
                false
            }
        }

        if (mimeType != null && matchingDecoders.isEmpty()) {
            return Result.Failure(
                kind = FailureKind.UNSUPPORTED_TYPE,
                detail = "Niet-ondersteund presentatie-rastertype: $mimeType",
            )
        }

        // Unknown MIME types are offered to the registered adapters in order.
        // This keeps provider quirks out of the outer Workspace contract.
        val candidates = if (matchingDecoders.isNotEmpty()) matchingDecoders else decoders
        var lastFailure: Result.Failure? = null

        candidates.forEach { decoder ->
            val result = try {
                decoder.decode(
                    resolver = resolver,
                    uri = uri,
                    mimeType = mimeType,
                    maxDimension = maxDimension,
                )
            } catch (error: SecurityException) {
                return Result.Failure(
                    kind = FailureKind.PERMISSION_DENIED,
                    detail = error.message ?: "Geen leestoegang tot documentprovider",
                )
            } catch (error: FileNotFoundException) {
                Result.Failure(
                    kind = FailureKind.PROVIDER_UNAVAILABLE,
                    detail = error.message ?: "Documentprovider kon het bestand niet openen",
                )
            } catch (_: OutOfMemoryError) {
                return Result.Failure(
                    kind = FailureKind.OUT_OF_MEMORY,
                    detail = "Onvoldoende geheugen voor presentatie-raster",
                )
            } catch (error: Exception) {
                Result.Failure(
                    kind = FailureKind.PROVIDER_UNAVAILABLE,
                    detail = error.message ?: error::class.java.simpleName,
                )
            }

            when (result) {
                is Result.Ready -> return result
                is Result.Failure -> lastFailure = result
            }
        }

        return lastFailure ?: Result.Failure(
            kind = FailureKind.DECODE_FAILED,
            detail = "Geen geregistreerde presentatie-decoder kon het raster lezen",
        )
    }

    /**
     * Current default adapter. More presentation-only adapters can be inserted
     * alongside it without changing the outer load(...) call.
     */
    private object BitmapFactoryDecoder : Decoder {
        override val id: String = "android.bitmap_factory.v1"

        override fun supports(mimeType: String?): Boolean =
            mimeType == null || mimeType in SUPPORTED_MIME_TYPES

        override fun decode(
            resolver: ContentResolver,
            uri: Uri,
            mimeType: String?,
            maxDimension: Int,
        ): Result {
            val bounds = BitmapFactory.Options().apply { inJustDecodeBounds = true }
            decodeFresh(resolver, uri, bounds)

            val sourceWidth = bounds.outWidth
            val sourceHeight = bounds.outHeight
            if (sourceWidth <= 0 || sourceHeight <= 0) {
                return Result.Failure(
                    kind = FailureKind.DECODE_FAILED,
                    detail = "Documentprovider leverde geen leesbare rasterafmetingen",
                )
            }

            var sampleSize = 1
            while (
                sourceWidth / sampleSize > maxDimension ||
                sourceHeight / sampleSize > maxDimension
            ) {
                sampleSize *= 2
            }

            val decodeOptions = BitmapFactory.Options().apply {
                inSampleSize = sampleSize
                inPreferredConfig = Bitmap.Config.ARGB_8888
            }
            val bitmap = decodeFresh(resolver, uri, decodeOptions)
                ?: return Result.Failure(
                    kind = FailureKind.DECODE_FAILED,
                    detail = "Documentprovider leverde geen decodeerbaar raster",
                )

            return Result.Ready(
                bitmap = bitmap,
                mimeType = mimeType,
                sourceWidth = sourceWidth,
                sourceHeight = sourceHeight,
                sampleSize = sampleSize,
                decoderId = id,
            )
        }

        /**
         * Opens a fresh provider handle for every decode. Bounds and pixel
         * decode therefore never depend on a reused stream. File-descriptor
         * decode is preferred because several DocumentsProvider implementations
         * do not expose a reliable InputStream for seekable media.
         */
        private fun decodeFresh(
            resolver: ContentResolver,
            uri: Uri,
            options: BitmapFactory.Options,
        ): Bitmap? {
            try {
                resolver.openFileDescriptor(uri, "r")?.use { descriptor ->
                    val bitmap = BitmapFactory.decodeFileDescriptor(
                        descriptor.fileDescriptor,
                        null,
                        options,
                    )
                    if (bitmap != null) return bitmap
                    if (
                        options.inJustDecodeBounds &&
                        options.outWidth > 0 &&
                        options.outHeight > 0
                    ) {
                        return null
                    }
                }
            } catch (_: FileNotFoundException) {
                // Some providers do not expose a file descriptor. Re-open as a stream below.
            }

            resolver.openInputStream(uri)?.use { stream ->
                return BitmapFactory.decodeStream(stream, null, options)
            }
            return null
        }

        private val SUPPORTED_MIME_TYPES = setOf(
            "image/jpeg",
            "image/jpg",
            "image/png",
            "image/webp",
        )
    }

    private fun persistReadPermissionBestEffort(
        resolver: ContentResolver,
        uri: Uri,
    ) {
        try {
            resolver.takePersistableUriPermission(
                uri,
                Intent.FLAG_GRANT_READ_URI_PERMISSION,
            )
        } catch (_: SecurityException) {
            // Temporary grants remain sufficient for this presentation-only decode.
        } catch (_: UnsupportedOperationException) {
            // Not every provider supports persistable grants.
        }
    }
}
