package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri

private const val PURE_FLOAT_MAGIC = 0x54525046L
private const val PURE_FLOAT_PACKET_LONGS = 18
private const val PURE_MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
private const val PURE_MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024
private const val PURE_POSTWRITE_SCAN_BYTES = 64 * 1024
private const val PURE_SELF_BINDING_CONTRACT = "TRUTHRAW_PURE_SELF_BINDING_V0_61"

object PureFloat32DngNativeBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun exportPureFloat32Dng(
        sourceFd: Int,
        outputFd: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): LongArray
}

data class PureFloat32DngMetrics(
    val width: Long,
    val height: Long,
    val samplesPerPixel: Long,
    val bitsPerSample: Long,
    val outputBytes: Long,
    val projectedPixels: Long,
    val negativeComponentCount: Long,
    val overOneComponentCount: Long,
    val tilesWritten: Long,
    val logicalResidentUpperBoundBytes: Long,
    val scientificMasterIdentityVerified: Boolean,
    val appearanceApplied: Boolean,
    val counterfactualObservationCreated: Boolean,
    val physicalFrameCount: Long,
    val independentEvidenceCount: Long,
    val colorClaimScopeCode: Long,
    val postWriteSelfBindingVerified: Boolean = false,
)

sealed interface PureFloat32DngExportResult {
    data class Success(val metrics: PureFloat32DngMetrics) : PureFloat32DngExportResult
    data class Failed(val reason: String) : PureFloat32DngExportResult
}

object PureFloat32DngExporter {
    fun export(
        resolver: ContentResolver,
        job: RawJob,
        destination: Uri,
    ): PureFloat32DngExportResult {
        if (!job.source.format.nativeProcessingReady || job.source.format.id != "DNG") {
            return PureFloat32DngExportResult.Failed(
                "TRUTHRAW PURE Float32 is momenteel alleen toegelaten voor de volledig admitted DNG-route.",
            )
        }

        return try {
            val source = resolver.openFileDescriptor(job.source.uri, "r")
                ?: return PureFloat32DngExportResult.Failed(
                    "Bronprovider gaf geen leesbare file descriptor.",
                )
            val output = resolver.openFileDescriptor(destination, "rw")
                ?: run {
                    source.close()
                    return PureFloat32DngExportResult.Failed(
                        "Bestemmingsprovider gaf geen schrijfbare file descriptor.",
                    )
                }

            val packet = source.use { src ->
                output.use { dst ->
                    PureFloat32DngNativeBridge.exportPureFloat32Dng(
                        src.fd,
                        dst.fd,
                        PURE_MAX_SOURCE_RESIDENT_BYTES,
                        PURE_MAX_LOGICAL_RESIDENT_BYTES,
                    )
                }
            }

            val decoded = decode(packet)
            if (decoded is PureFloat32DngExportResult.Failed) {
                runCatching { resolver.delete(destination, null, null) }
                return decoded
            }

            val success = decoded as PureFloat32DngExportResult.Success
            val postWrite = verifySavedPureDng(resolver, destination)
            if (!postWrite.ok) {
                runCatching { resolver.delete(destination, null, null) }
                return PureFloat32DngExportResult.Failed(
                    "Fail-closed: writer meldde succes, maar het opgeslagen DNG-bestand kon de " +
                        "v0.61 self-binding niet terugbewijzen (${postWrite.reason}).",
                )
            }

            PureFloat32DngExportResult.Success(
                success.metrics.copy(postWriteSelfBindingVerified = true),
            )
        } catch (error: Throwable) {
            runCatching { resolver.delete(destination, null, null) }
            PureFloat32DngExportResult.Failed(
                "TRUTHRAW PURE Float32-export faalde: " +
                    (error.message ?: error.javaClass.simpleName),
            )
        }
    }

    private data class PostWriteVerification(
        val ok: Boolean,
        val reason: String,
    )

    private fun verifySavedPureDng(
        resolver: ContentResolver,
        destination: Uri,
    ): PostWriteVerification {
        val requiredMarkers = listOf(
            "TruthRaw scientific-master-linear-dng-projection-v0.1",
            "role=TRUTHRAW_PURE_FLOAT32_XYZ_D50_LINEAR_DNG_PROJECTION",
            "private_contract=$PURE_SELF_BINDING_CONTRACT",
            "sealed_source_sha256=",
            "scientific_master_sha256=",
            "zero_line_sha256=",
            "zero_line_l0_f64_bits=0x",
            "zero_line_gauge_id=",
            "scene_scale_sha256=",
            "scene_scale_id=",
            "technical_backplane_version=1",
            "technical_backplane_crc32=0x",
            "technical_backplane_serialized_hex=",
            "precision_policy_id=",
            "runtime_reconstruction_backend_id=",
            "physical_frame_count=1",
            "independent_evidence_count=1",
        )
        val forbiddenMarkers = listOf(
            "role=LINEAR_DNG_XYZ_D50_COMPATIBILITY_PROJECTION",
        )

        return try {
            val input = resolver.openInputStream(destination)
                ?: return PostWriteVerification(false, "bestemming is niet terugleesbaar")
            val bytes = ByteArray(PURE_POSTWRITE_SCAN_BYTES)
            var used = 0
            input.use { stream ->
                while (used < bytes.size) {
                    val read = stream.read(bytes, used, bytes.size - used)
                    if (read <= 0) break
                    used += read
                }
            }
            if (used <= 0) {
                return PostWriteVerification(false, "leeg bestand na commit")
            }

            val headerText = String(bytes, 0, used, Charsets.ISO_8859_1)
            val missing = requiredMarkers.filterNot(headerText::contains)
            if (missing.isNotEmpty()) {
                return PostWriteVerification(
                    false,
                    "ontbrekende marker(s): ${missing.joinToString()}",
                )
            }
            val forbidden = forbiddenMarkers.firstOrNull(headerText::contains)
            if (forbidden != null) {
                return PostWriteVerification(
                    false,
                    "oude route-marker aanwezig: $forbidden",
                )
            }

            fun valueOf(key: String): String? {
                val start = headerText.indexOf(key)
                if (start < 0) return null
                val valueStart = start + key.length
                val end = headerText.indexOf('\n', valueStart).let {
                    if (it < 0) headerText.length else it
                }
                return headerText.substring(valueStart, end).trim()
            }
            fun isHex(value: String, chars: Int): Boolean =
                value.length == chars && value.all {
                    it in '0'..'9' || it in 'a'..'f' || it in 'A'..'F'
                }

            val hashKeys = listOf(
                "sealed_source_sha256=",
                "scientific_master_sha256=",
                "zero_line_sha256=",
                "scene_scale_sha256=",
            )
            val badHash = hashKeys.firstOrNull { key ->
                !isHex(valueOf(key).orEmpty(), 64)
            }
            if (badHash != null) {
                return PostWriteVerification(false, "ongeldige SHA-256 waarde: $badHash")
            }

            val l0Bits = valueOf("zero_line_l0_f64_bits=").orEmpty()
            if (!l0Bits.startsWith("0x") || !isHex(l0Bits.drop(2), 16)) {
                return PostWriteVerification(false, "ongeldige exacte L0 binary64 bits")
            }

            val backplane = valueOf("technical_backplane_serialized_hex=").orEmpty()
            if (!isHex(backplane, 180 * 2)) {
                return PostWriteVerification(
                    false,
                    "Technical Backplane is niet exact 180 bytes",
                )
            }

            val crc = valueOf("technical_backplane_crc32=").orEmpty()
            if (!crc.startsWith("0x") || !isHex(crc.drop(2), 8)) {
                return PostWriteVerification(false, "ongeldige Technical Backplane CRC32")
            }

            if (valueOf("precision_policy_id=").isNullOrBlank() ||
                valueOf("runtime_reconstruction_backend_id=").isNullOrBlank()
            ) {
                return PostWriteVerification(false, "precision/runtime provenance ontbreekt")
            }

            PostWriteVerification(true, "contract en payloadlengtes teruggelezen")
        } catch (error: Throwable) {
            PostWriteVerification(
                false,
                error.message ?: error.javaClass.simpleName,
            )
        }
    }

    private fun decode(packet: LongArray): PureFloat32DngExportResult {
        if (packet.size != PURE_FLOAT_PACKET_LONGS || packet[0] != PURE_FLOAT_MAGIC) {
            return PureFloat32DngExportResult.Failed(
                "Ongeldig native TRUTHRAW PURE Float32-resultaat.",
            )
        }
        val status = packet[1]
        if (status != 0L) {
            return PureFloat32DngExportResult.Failed(statusDescription(status))
        }

        val metrics = PureFloat32DngMetrics(
            width = packet[2],
            height = packet[3],
            samplesPerPixel = packet[4],
            bitsPerSample = packet[5],
            outputBytes = packet[6],
            projectedPixels = packet[7],
            negativeComponentCount = packet[8],
            overOneComponentCount = packet[9],
            tilesWritten = packet[10],
            logicalResidentUpperBoundBytes = packet[11],
            scientificMasterIdentityVerified = packet[12] != 0L,
            appearanceApplied = packet[13] != 0L,
            counterfactualObservationCreated = packet[14] != 0L,
            physicalFrameCount = packet[15],
            independentEvidenceCount = packet[16],
            colorClaimScopeCode = packet[17],
        )

        val violation =
            metrics.width <= 0L ||
                metrics.height <= 0L ||
                metrics.samplesPerPixel != 3L ||
                metrics.bitsPerSample != 32L ||
                metrics.outputBytes <= 0L ||
                metrics.projectedPixels != metrics.width * metrics.height ||
                metrics.tilesWritten <= 0L ||
                metrics.logicalResidentUpperBoundBytes <= 0L ||
                metrics.logicalResidentUpperBoundBytes > PURE_MAX_LOGICAL_RESIDENT_BYTES.toLong() ||
                !metrics.scientificMasterIdentityVerified ||
                metrics.appearanceApplied ||
                metrics.counterfactualObservationCreated ||
                metrics.physicalFrameCount != 1L ||
                metrics.independentEvidenceCount != 1L ||
                metrics.colorClaimScopeCode !in 1L..2L

        if (violation) {
            return PureFloat32DngExportResult.Failed(
                "Fail-closed: PURE Float32 DNG schond master-, representation- of evidencecontract.",
            )
        }

        return PureFloat32DngExportResult.Success(metrics)
    }

    private fun statusDescription(status: Long): String = when (status) {
        -1L -> "PURE Float32: ongeldige exportparameters."
        -2L -> "PURE Float32: pre-master authority-state was niet canoniek."
        -3L -> "PURE Float32: Phase-2/Scientific-Master/source binding kwam niet exact overeen."
        -4L -> "PURE Float32: projection probeerde master/appearance/evidence-invariant te schenden."

        in 2001L..2099L -> "PURE Float32: source binding faalde (status $status)."
        in 2101L..2199L -> "PURE Float32: DNG color binding faalde (status $status)."
        in 7001L..7099L -> "PURE Float32: generieke RAW/DNG adapter faalde (status $status)."
        in 8001L..8099L -> "PURE Float32: Scientific Master streaming faalde (status $status)."
        in 9001L..9099L -> "PURE Float32: Technical Backplane Phase-2 faalde (status $status)."
        in 10001L..10099L -> when (status) {
            10001L -> "Float32 Scientific DNG writer: ongeldig argument."
            10002L -> "Float32 Scientific DNG writer: ongeldige kleurtransformatie."
            10003L -> "Float32 Scientific DNG writer: TIFF/DNG-grootte overflow."
            10004L -> "Float32 Scientific DNG writer: Scientific-Master tile-read faalde."
            10005L -> "Float32 Scientific DNG writer: master-digest kon niet worden opgebouwd."
            10006L -> "Float32 Scientific DNG writer: replayed Master hash mismatch."
            10007L -> "Float32 Scientific DNG writer: transactionele output-sink faalde."
            10008L -> "Float32 Scientific DNG writer: Zero-Line/scene-scale/Technical-Backplane binding mismatch."
            else -> "Float32 Scientific DNG writer faalde (status $status)."
        }
        else -> "Onbekende TRUTHRAW PURE Float32-status $status."
    }
}
