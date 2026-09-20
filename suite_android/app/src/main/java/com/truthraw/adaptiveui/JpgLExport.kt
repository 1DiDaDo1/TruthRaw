package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import android.os.ParcelFileDescriptor
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.io.RandomAccessFile
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.security.MessageDigest

data class JpgLMetrics(
    val width: Int,
    val height: Int,
    val outputBytes: Long,
    val jpegBytes: Long,
    val scienceBytes: Long,
    val manifestBytes: Long,
    val jpegSha256: String,
    val scienceSha256: String,
    val manifestSha256: String,
    val containerSha256: String,
)

sealed interface JpgLResult {
    data class Success(val metrics: JpgLMetrics) : JpgLResult
    data class Failed(val reason: String) : JpgLResult
}

object JpgLExporter {
    private const val FOOTER_BYTES = 256
    private const val FOOTER_MAGIC = "TRJPGL01"
    private const val COPY_BUFFER = 1024 * 1024

    fun export(
        resolver: ContentResolver,
        job: RawJob,
        destination: Uri,
        route: String,
        flags: Int,
        userQuarterTurns: Int,
        workingDir: File,
    ): JpgLResult {
        workingDir.mkdirs()
        val frontResult = FullResJpegExporter.renderToPrivateJpeg(
            resolver, job, flags, userQuarterTurns, workingDir,
        )
        if (frontResult is FullResJpegResult.Failed) return JpgLResult.Failed(frontResult.reason)
        val front = frontResult as FullResJpegResult.Success

        val science = File(workingDir, "photo_science_tn3.part")
        science.delete()
        val scienceResult = exportScienceToFile(resolver, job, science)
        if (scienceResult != null) {
            front.file.delete()
            science.delete()
            return JpgLResult.Failed(scienceResult)
        }
        val scienceSha = FullResJpegExporter.sha256(science)
            ?: run {
                front.file.delete(); science.delete()
                return JpgLResult.Failed("JPG-L Scientific Master-laag kon niet worden gehasht.")
            }

        val manifest = buildManifest(route, flags, front.metrics, front.metrics.jpegSha256, scienceSha)
            .toByteArray(Charsets.UTF_8)
        val manifestSha = sha256Bytes(manifest)

        val staging = File(workingDir, "photo.jpgl.part")
        staging.delete()
        val jpegOffset = 0L
        val jpegLength = front.file.length()
        val scienceOffset = jpegLength
        val scienceLength = science.length()
        val manifestOffset = scienceOffset + scienceLength
        val manifestLength = manifest.size.toLong()
        val footerOffset = manifestOffset + manifestLength
        val totalBytes = footerOffset + FOOTER_BYTES

        try {
            FileOutputStream(staging).use { out ->
                copy(front.file, out)
                copy(science, out)
                out.write(manifest)
                val prefixSha = sha256Prefix(front.file, science, manifest)
                out.write(
                    buildFooter(
                        jpegOffset, jpegLength,
                        scienceOffset, scienceLength,
                        manifestOffset, manifestLength,
                        front.metrics.jpegSha256, scienceSha, manifestSha,
                        prefixSha, totalBytes,
                    ),
                )
                out.flush()
                out.channel.force(true)
            }
        } catch (error: Throwable) {
            front.file.delete(); science.delete(); staging.delete()
            return JpgLResult.Failed("JPG-L containerbouw faalde: ${error.message ?: error.javaClass.simpleName}")
        } finally {
            front.file.delete()
            science.delete()
        }

        val verified = verify(staging)
        if (!verified.first) {
            staging.delete()
            return JpgLResult.Failed("JPG-L verify faalde: ${verified.second}")
        }
        val containerSha = FullResJpegExporter.sha256(staging)
            ?: run { staging.delete(); return JpgLResult.Failed("JPG-L container SHA-256 ontbreekt.") }

        val committed = FullResJpegExporter.commit(resolver, staging, destination, containerSha)
        if (!committed) {
            staging.delete()
            return JpgLResult.Failed("JPG-L commit/post-write SHA-verify faalde.")
        }

        val metrics = JpgLMetrics(
            width = front.metrics.width,
            height = front.metrics.height,
            outputBytes = staging.length(),
            jpegBytes = jpegLength,
            scienceBytes = scienceLength,
            manifestBytes = manifestLength,
            jpegSha256 = front.metrics.jpegSha256,
            scienceSha256 = scienceSha,
            manifestSha256 = manifestSha,
            containerSha256 = containerSha,
        )
        staging.delete()
        return JpgLResult.Success(metrics)
    }

    private fun exportScienceToFile(
        resolver: ContentResolver,
        job: RawJob,
        file: File,
    ): String? {
        val source = try {
            if (job.source.uri.scheme == ContentResolver.SCHEME_FILE) {
                ParcelFileDescriptor.open(File(job.source.uri.path ?: return "JPG-L bronpad ontbreekt."), ParcelFileDescriptor.MODE_READ_ONLY)
            } else {
                resolver.openFileDescriptor(job.source.uri, "r")
            }
        } catch (_: Throwable) { null } ?: return "JPG-L bron-FD kon niet worden geopend."

        val output = try {
            ParcelFileDescriptor.open(
                file,
                ParcelFileDescriptor.MODE_CREATE or
                    ParcelFileDescriptor.MODE_READ_WRITE or
                    ParcelFileDescriptor.MODE_TRUNCATE,
            )
        } catch (_: Throwable) { null } ?: run {
            source.close(); return "JPG-L TN-3 staging kon niet worden geopend."
        }

        val packet = try {
            source.use { src ->
                output.use { dst ->
                    TruthNegativeNativeBridge.exportTruthNegative(
                        src.fd, dst.fd,
                        8 * 1024 * 1024,
                        64 * 1024 * 1024,
                    )
                }
            }
        } catch (error: Throwable) {
            return "JPG-L TN-3 export faalde: ${error.message ?: error.javaClass.simpleName}"
        }
        if (packet.size != 28 || packet[0] != 0x54524e47L || packet[1] != 0L ||
            packet[14] != 1L || packet[15] != 1L ||
            packet[16] != 0L || packet[17] != 0L ||
            packet[18] != 1L || packet[19] != 1L || packet[25] != 1L || packet[27] != 3L ||
            file.length() != packet[6]
        ) {
            return "JPG-L TN-3 scientific-layer invariant faalde."
        }
        val header = FileInputStream(file).use { input ->
            val bytes = ByteArray(8192)
            var offset = 0
            while (offset < bytes.size) {
                val n = input.read(bytes, offset, bytes.size - offset)
                if (n <= 0) break
                offset += n
            }
            if (offset != bytes.size) return "JPG-L TN-3 header is onvolledig."
            bytes.toString(Charsets.US_ASCII)
        }
        if (!header.contains("magic=TRUTHNEGATIVE_V0_3_TN3") ||
            !header.contains("sample_encoding=IEEE754_BINARY32_LE") ||
            !header.contains("pixel_role=CAMERA_NATIVE_SCIENTIFIC_MASTER_RGB") ||
            !header.contains("tn3_full_open_scene_state=1")
        ) return "JPG-L TN-3 headercontract ontbreekt."
        return null
    }

    private fun buildManifest(
        route: String,
        flags: Int,
        m: FullResJpegMetrics,
        jpegSha: String,
        scienceSha: String,
    ): String = buildString {
        appendLine("magic=TRUTHRAW_JPGL_MANIFEST_V0_2")
        appendLine("container_role=JPEG_COMPATIBLE_LAYERED_PHOTOGRAPH")
        appendLine("external_mime=image/jpeg")
        appendLine("recommended_extension=.jpg")
        appendLine("legacy_extension=.jpgl")
        appendLine("front_role=FULL_RESOLUTION_SRGB_JPEG_COMPATIBILITY")
        appendLine("science_role=TN3_CAMERA_NATIVE_FLOAT32_SCIENTIFIC_MASTER_OPEN_SCENE")
        appendLine("route=$route")
        appendLine("width=${m.width}")
        appendLine("height=${m.height}")
        appendLine("source_width=${m.sourceWidth}")
        appendLine("source_height=${m.sourceHeight}")
        appendLine("source_orientation=${m.sourceOrientation}")
        appendLine("user_rotation_quarter_turns=${m.userQuarterTurns}")
        appendLine("effective_front_orientation=${m.effectiveOrientation}")
        appendLine("science_orientation_unchanged=1")
        appendLine("orientation_override_role=PRESENTATION_COORDINATE_TRANSFORM_ONLY")
        appendLine("precision=F32_CANONICAL")
        appendLine("advanced_flags=$flags")
        appendLine("detail_applied_to_front=${if (m.detailApplied) 1 else 0}")
        appendLine("light_adjusted_pixels=${m.lightAdjustedPixels}")
        appendLine("hdr_positive_gain_samples=${m.hdrPositiveGainSamples}")
        appendLine("hdr_baked_into_front=${if (m.hdrBakedIntoFront) 1 else 0}")
        appendLine("hdr_front_authority=${if (m.hdrBakedIntoFront) "APPEARANCE_ONLY" else "DISABLED_OR_NO_POSITIVE_GAIN"}")
        appendLine("restoration_baked_into_front=${if (m.restorationBakedIntoFront) 1 else 0}")
        appendLine("restoration_front_role=AESTHETIC_REINTEGRATION_ONLY")
        appendLine("canonical_open_scene_v070_bound=${if (m.canonicalOpenSceneBound) 1 else 0}")
        appendLine("channel_authority_v078_bound=${if (m.channelAuthorityBound) 1 else 0}")
        appendLine("uncertainty_decision_v079_code=${m.uncertaintyDecisionCode}")
        appendLine("reconstructed_authority_allowed=${if (m.reconstructedAuthorityAllowed) 1 else 0}")
        appendLine("illumination_state_v082_bound=${if (m.illuminationStateBound) 1 else 0}")
        appendLine("illumination_white_point_known=${if (m.illuminationWhitePointKnown) 1 else 0}")
        appendLine("scientific_hdr_authority_v083_code=${m.scientificHdrAuthorityCode}")
        appendLine("presentation_hdr_authority_v083_code=${m.presentationHdrAuthorityCode}")
        appendLine("hdr_blocked_reason_v083_code=${m.hdrBlockedReasonCode}")
        appendLine("per_output_channel_authority_available=${if (m.perOutputChannelAuthorityAvailable) 1 else 0}")
        appendLine("scientific_master_bound=${if (m.scientificMasterBound) 1 else 0}")
        appendLine("technical_backplane_bound=${if (m.backplaneBound) 1 else 0}")
        appendLine("source_reverified=${if (m.sourceReverified) 1 else 0}")
        appendLine("physical_frame_count=1")
        appendLine("independent_evidence_count=1")
        appendLine("front_jpeg_sha256=$jpegSha")
        appendLine("science_tn3_sha256=$scienceSha")
        appendLine("representation_can_exceed_source=1")
        appendLine("knowledge_claims_cannot_exceed_evidence=1")
        appendLine("scientific_writeback_allowed=0")
        appendLine("creates_new_evidence=0")
        appendLine("END_MANIFEST")
    }

    private fun buildFooter(
        jpegOffset: Long, jpegLength: Long,
        scienceOffset: Long, scienceLength: Long,
        manifestOffset: Long, manifestLength: Long,
        jpegSha: String, scienceSha: String, manifestSha: String,
        prefixSha: String, totalLength: Long,
    ): ByteArray {
        val out = ByteArray(FOOTER_BYTES)
        val b = ByteBuffer.wrap(out).order(ByteOrder.LITTLE_ENDIAN)
        b.put(FOOTER_MAGIC.toByteArray(Charsets.US_ASCII))
        b.putLong(1L)
        b.putLong(jpegOffset); b.putLong(jpegLength)
        b.putLong(scienceOffset); b.putLong(scienceLength)
        b.putLong(manifestOffset); b.putLong(manifestLength)
        b.put(hex(jpegSha)); b.put(hex(scienceSha)); b.put(hex(manifestSha)); b.put(hex(prefixSha))
        b.putLong(totalLength)
        b.putLong(FOOTER_BYTES.toLong())
        return out
    }

    fun verify(file: File): Pair<Boolean, String> {
        if (!file.isFile || file.length() < FOOTER_BYTES + 4L) return false to "bestand is te klein"
        val footer = ByteArray(FOOTER_BYTES)
        RandomAccessFile(file, "r").use { raf ->
            raf.seek(file.length() - FOOTER_BYTES)
            raf.readFully(footer)
        }
        val b = ByteBuffer.wrap(footer).order(ByteOrder.LITTLE_ENDIAN)
        val magic = ByteArray(8); b.get(magic)
        if (magic.toString(Charsets.US_ASCII) != FOOTER_MAGIC) return false to "footer magic ontbreekt"
        if (b.long != 1L) return false to "onbekende footer-versie"
        val jpegOffset=b.long; val jpegLength=b.long
        val scienceOffset=b.long; val scienceLength=b.long
        val manifestOffset=b.long; val manifestLength=b.long
        val jpegSha=ByteArray(32); b.get(jpegSha)
        val scienceSha=ByteArray(32); b.get(scienceSha)
        val manifestSha=ByteArray(32); b.get(manifestSha)
        val prefixSha=ByteArray(32); b.get(prefixSha)
        val total=b.long
        val footerBytes=b.long
        if (jpegOffset != 0L || jpegLength <= 4L || scienceOffset != jpegLength ||
            scienceLength <= 8192L || manifestOffset != scienceOffset + scienceLength ||
            manifestLength <= 0L || manifestOffset + manifestLength + footerBytes != total ||
            total != file.length() || footerBytes != FOOTER_BYTES.toLong()
        ) return false to "64-bit chunk table is inconsistent"

        if (!digestRange(file, jpegOffset, jpegLength).contentEquals(jpegSha)) return false to "JPEG chunk hash mismatch"
        if (!digestRange(file, scienceOffset, scienceLength).contentEquals(scienceSha)) return false to "science chunk hash mismatch"
        if (!digestRange(file, manifestOffset, manifestLength).contentEquals(manifestSha)) return false to "manifest chunk hash mismatch"
        if (!digestRange(file, 0L, file.length()-FOOTER_BYTES).contentEquals(prefixSha)) return false to "prefix hash mismatch"

        val soi = ByteArray(2)
        RandomAccessFile(file, "r").use { raf -> raf.readFully(soi) }
        if ((soi[0].toInt() and 0xff) != 0xff || (soi[1].toInt() and 0xff) != 0xd8) {
            return false to "JPEG front SOI ontbreekt"
        }
        val tnMagic = ByteArray(32)
        RandomAccessFile(file, "r").use { raf ->
            raf.seek(scienceOffset)
            raf.readFully(tnMagic)
        }
        if (!tnMagic.toString(Charsets.US_ASCII).startsWith("magic=TRUTHNEGATIVE_V0_3_TN3")) {
            return false to "TN-3 science chunk ontbreekt"
        }
        return true to "JPEG-front + TN-3 Float32/Open Scene + manifest + 64-bit footer geverifieerd"
    }

    private fun copy(file: File, out: FileOutputStream) {
        FileInputStream(file).use { input ->
            val buffer = ByteArray(COPY_BUFFER)
            while (true) {
                val n = input.read(buffer)
                if (n <= 0) break
                out.write(buffer, 0, n)
            }
        }
    }

    private fun sha256Prefix(jpegFile: File, scienceFile: File, manifest: ByteArray): String {
        val md=MessageDigest.getInstance("SHA-256")
        for (file in listOf(jpegFile,scienceFile)) {
            FileInputStream(file).use { input ->
                val buffer=ByteArray(COPY_BUFFER)
                while(true){val n=input.read(buffer);if(n<=0)break;md.update(buffer,0,n)}
            }
        }
        md.update(manifest)
        return md.digest().joinToString("") { "%02x".format(it) }
    }

    private fun digestRange(file: File, offset: Long, length: Long): ByteArray {
        val md=MessageDigest.getInstance("SHA-256")
        RandomAccessFile(file,"r").use { raf ->
            raf.seek(offset)
            var remain=length
            val buffer=ByteArray(COPY_BUFFER)
            while(remain>0){
                val n=raf.read(buffer,0,minOf(buffer.size.toLong(),remain).toInt())
                if(n<=0) break
                md.update(buffer,0,n)
                remain-=n
            }
            if(remain!=0L) return ByteArray(0)
        }
        return md.digest()
    }

    private fun sha256Bytes(bytes: ByteArray): String =
        MessageDigest.getInstance("SHA-256").digest(bytes).joinToString("") { "%02x".format(it) }

    private fun hex(value: String): ByteArray {
        require(value.length==64)
        return ByteArray(32) { i -> value.substring(2*i,2*i+2).toInt(16).toByte() }
    }
}
