package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import android.os.ParcelFileDescriptor
import java.io.File
import java.util.zip.CRC32

private const val PURE_FLOAT_MAGIC = 0x54525046L
private const val PURE_FLOAT_PACKET_LONGS = 34
private const val PURE_MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
private const val PURE_MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024
private const val PURE_POSTWRITE_SCAN_BYTES = 64 * 1024
private const val PURE_SELF_BINDING_CONTRACT = "TRUTHRAW_PURE_SELF_BINDING_V0_63"

object PureFloat32DngNativeBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun exportPureFloat32Dng(
        sourceFd: Int,
        outputFd: Int,
        userQuarterTurns: Int,
        exportMode: Int,
        sourceRouteCode: Int,
        advancedFlags: Int,
        previewFd: Int,
        previewWidth: Int,
        previewHeight: Int,
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
    val outputChannelAuthorityAvailable: Boolean,
    val outputChannelAuthorityMappingMode: Int,
    val outputAuthorityCalibratedChannels: Long,
    val outputAuthorityReconstructedChannels: Long,
    val outputAuthorityCensoredChannels: Long,
    val outputAuthorityUnknownChannels: Long,
    val outputAuthorityCensoredSupportPixels: Long,
    val outputAuthorityArtifactSha256: String,
    val postWriteSelfBindingVerified: Boolean = false,
)

sealed interface PureFloat32DngExportResult {
    data class Success(val metrics: PureFloat32DngMetrics) : PureFloat32DngExportResult
    data class Failed(val reason: String) : PureFloat32DngExportResult
}

enum class Float32DngExportFlavor(val nativeCode: Int) {
    PURE(0),
    FULL_COLOUR_SCIENTIFIC_MASTER(1),
    ADVANCED_RENDER_EDIT(2),
    TRUTHNEGATIVE_200MP_FULL_COLOUR(3),
}

object PureFloat32DngExporter {
    private fun detailStrengthFromFlags(flags: Int): Int {
        if ((flags and TruthRawAdvancedOptions.FLAG_DETAIL) == 0) return 0
        val encoded =
            (flags ushr TruthRawAdvancedOptions.DETAIL_STRENGTH_SHIFT) and 0x7f
        return if (encoded == 0) 100 else encoded.coerceIn(1, 100)
    }

    private fun colorFullnessFromFlags(flags: Int): Int {
        if ((flags and TruthRawAdvancedOptions.FLAG_COLOR_CONTROL) == 0) return 0
        val encoded =
            ((flags ushr TruthRawAdvancedOptions.COLOR_FULLNESS_SHIFT) and 0x7f)
                .coerceIn(0, 100)
        return encoded - 50
    }

    private fun exposureFromFlags(flags: Int): Float {
        if ((flags and TruthRawAdvancedOptions.FLAG_EXPOSURE_CONTROL) == 0) return 0f
        val encoded =
            ((flags ushr TruthRawAdvancedOptions.EXPOSURE_SHIFT) and 0x7f)
                .coerceIn(0, 100)
        return (encoded - 50) * 0.04f
    }

    private fun shadowLevelFromFlags(flags: Int): Int =
        ((flags ushr TruthRawAdvancedOptions.SHADOW_STRENGTH_SHIFT) and 0x03)
            .coerceIn(0, 3)

    private fun advancedAppearanceBaked(flags: Int): Boolean =
        (flags and (
            TruthRawAdvancedOptions.FLAG_LIGHT or
                TruthRawAdvancedOptions.FLAG_DETAIL or
                TruthRawAdvancedOptions.FLAG_RESTORATION
            )) != 0 ||
            colorFullnessFromFlags(flags) != 0 ||
            kotlin.math.abs(exposureFromFlags(flags)) > 1e-6f ||
            shadowLevelFromFlags(flags) > 0

    fun export(
        resolver: ContentResolver,
        job: RawJob,
        destination: Uri,
        userQuarterTurns: Int = 0,
        flavor: Float32DngExportFlavor = Float32DngExportFlavor.PURE,
        advancedFlags: Int = 0,
        previewFile: File? = null,
        previewWidth: Int = 0,
        previewHeight: Int = 0,
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

            val preview = if (previewFile != null) {
                runCatching {
                    ParcelFileDescriptor.open(
                        previewFile,
                        ParcelFileDescriptor.MODE_READ_ONLY,
                    )
                }.getOrNull()
                    ?: run {
                        source.close()
                        output.close()
                        return PureFloat32DngExportResult.Failed(
                            "Float32 DNG previewbestand kon niet worden geopend.",
                        )
                    }
            } else {
                null
            }

            val packet = source.use { src ->
                output.use { dst ->
                    if (preview != null) {
                        preview.use { p ->
                            PureFloat32DngNativeBridge.exportPureFloat32Dng(
                                src.fd,
                                dst.fd,
                                userQuarterTurns,
                                flavor.nativeCode,
                                when (job.source.sourceRoute) {
                                    SourceIngressRoute.IMPORTED_FILE -> 0
                                    SourceIngressRoute.CAMERA_CAPTURE -> 1
                                },
                                if (flavor == Float32DngExportFlavor.ADVANCED_RENDER_EDIT) advancedFlags else 0,
                                p.fd,
                                previewWidth,
                                previewHeight,
                                PURE_MAX_SOURCE_RESIDENT_BYTES,
                                PURE_MAX_LOGICAL_RESIDENT_BYTES,
                            )
                        }
                    } else {
                        PureFloat32DngNativeBridge.exportPureFloat32Dng(
                            src.fd,
                            dst.fd,
                            userQuarterTurns,
                            flavor.nativeCode,
                            when (job.source.sourceRoute) {
                                SourceIngressRoute.IMPORTED_FILE -> 0
                                SourceIngressRoute.CAMERA_CAPTURE -> 1
                            },
                            if (flavor == Float32DngExportFlavor.ADVANCED_RENDER_EDIT) advancedFlags else 0,
                            -1,
                            0,
                            0,
                            PURE_MAX_SOURCE_RESIDENT_BYTES,
                            PURE_MAX_LOGICAL_RESIDENT_BYTES,
                        )
                    }
                }
            }

            val decoded = decode(packet, flavor, advancedFlags)
            if (decoded is PureFloat32DngExportResult.Failed) {
                runCatching { resolver.delete(destination, null, null) }
                return decoded
            }

            val success = decoded as PureFloat32DngExportResult.Success
            val postWrite = verifySavedPureDng(
                resolver,
                destination,
                flavor,
                advancedFlags,
                previewFile != null,
            )
            if (!postWrite.ok) {
                runCatching { resolver.delete(destination, null, null) }
                return PureFloat32DngExportResult.Failed(
                    "Fail-closed: writer meldde succes, maar de opgeslagen Float32 DNG kon de " +
                        "self-binding/edit-binding niet terugbewijzen (${postWrite.reason}).",
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
        flavor: Float32DngExportFlavor,
        advancedFlags: Int,
        previewExpected: Boolean,
    ): PostWriteVerification {
        val roleMarker = when (flavor) {
            Float32DngExportFlavor.PURE ->
                "role=TRUTHRAW_PURE_FLOAT32_XYZ_D50_LINEAR_DNG_PROJECTION"
            Float32DngExportFlavor.FULL_COLOUR_SCIENTIFIC_MASTER ->
                "role=TRUTHRAW_FULL_COLOUR_SCIENTIFIC_MASTER_FLOAT32_CAMERA_NATIVE_LINEAR_DNG_V0_1"
            Float32DngExportFlavor.ADVANCED_RENDER_EDIT ->
                "role=TRUTHRAW_ADVANCED_RENDER_EDIT_FLOAT32_XYZ_D50_LINEAR_DNG_V0_1"
            Float32DngExportFlavor.TRUTHNEGATIVE_200MP_FULL_COLOUR ->
                "role=TRUTHRAW_TRUTHNEGATIVE_200MP_FULL_COLOUR_FLOAT32_CAMERA_NATIVE_LINEAR_DNG_V0_1"
        }
        val requiredMarkers = mutableListOf(
            "TruthRaw scientific-master-linear-dng-projection-v0.1",
            roleMarker,
            "private_contract=$PURE_SELF_BINDING_CONTRACT",
            "sealed_source_sha256=",
            "scientific_master_sha256=",
            "zero_line_sha256=",
            "zero_line_l0_f64_bits=0x",
            "zero_line_gauge_id=",
            "scene_scale_sha256=",
            "scene_scale_id=",
            "technical_backplane_version=1",
            "technical_backplane_crc_scope=PREFIX_176_BYTES",
            "technical_backplane_crc32=0x",
            "technical_backplane_serialized_hex=",
            "precision_policy_id=",
            "runtime_reconstruction_backend_id=",
            "physical_frame_count=1",
            "independent_evidence_count=1",
            "output_channel_authority_bound=1",
            "output_channel_authority_manifest_begin",
            "artifact_sha256=",
            "reconstructed_channels=0",
            "unknown_channels=",
            "scientific_writeback_allowed=0",
            "creates_new_evidence=0",
            "output_channel_authority_manifest_end",
        )
        if (flavor == Float32DngExportFlavor.TRUTHNEGATIVE_200MP_FULL_COLOUR) {
            requiredMarkers += listOf(
                "schema=TruthNegativeOutputChannelAuthority/0.1",
                "parent_schema=TruthRawOutputChannelAuthority/0.84",
                "mapping_mode=RESAMPLED_UNIFORM_UNKNOWN_IMPLICIT",
                "source_width=4080",
                "source_height=3072",
                "output_width=16320",
                "output_height=12288",
                "uniform_authority=UNKNOWN",
                "target_support_role=RECONSTRUCTED_DENSE_SUPPORT",
                "measured_target_claim_count=0",
                "backend_changes_authority=0",
            )
        } else {
            requiredMarkers += listOf(
                "schema=TruthRawOutputChannelAuthority/0.84",
                "mapping_mode=FULL_RESOLUTION_CONSERVATIVE",
                "orientation_transform_changes_authority=0",
            )
        }
        if (previewExpected) {
            requiredMarkers += listOf(
                "embedded_jpeg_preview=1",
                "preview_role=NON_AUTHORITY_RENDERED_PREVIEW",
                "preview_scientific_writeback_allowed=0",
            )
        } else {
            requiredMarkers += "embedded_jpeg_preview=0"
        }
        if (flavor == Float32DngExportFlavor.FULL_COLOUR_SCIENTIFIC_MASTER) {
            requiredMarkers += listOf(
                "primary_storage_space=CAMERA_NATIVE_SCIENTIFIC_MASTER_RGB_FLOAT32",
                "camera_profile_role=DERIVED_FROM_AUTHORIZED_CAMERA_TO_XYZ_D50",
                "primary_linearraw_is_full_colour=1",
                "primary_is_jpeg_snapshot=0",
                "downstream_edit_manifest_begin",
                "schema=TruthRawFullColourScientificMaster/0.1",
                "primary_image_role=CAMERA_NATIVE_SCIENTIFIC_MASTER_RGB_FLOAT32",
                "stored_primary_space=CAMERA_NATIVE_SCIENTIFIC_MASTER_RGB_FLOAT32",
                "photometric_role=LINEARRAW_FULL_COLOUR_CAMERA_NATIVE",
                "source_scientific_master_unchanged=1",
                "primary_raster_equals_scientific_master=1",
                "appearance_baked_into_primary=0",
                "advanced_recipe_flags=0",
                "negative_components_preserved=1",
                "over_one_components_preserved=1",
                "jpeg_role=NON_AUTHORITY_PREVIEW_ONLY",
                "lightroom_editable_primary=1",
                "scientific_writeback_allowed=0",
                "creates_new_evidence=0",
                "downstream_edit_manifest_end",
            )
        }
        if (flavor == Float32DngExportFlavor.TRUTHNEGATIVE_200MP_FULL_COLOUR) {
            requiredMarkers += listOf(
                "derivative_projection=1",
                "projected_raster_sha256=",
                "open_scene_state_sha256=",
                "primary_storage_space=CAMERA_NATIVE_SCIENTIFIC_MASTER_RGB_FLOAT32",
                "camera_profile_role=DERIVED_FROM_AUTHORIZED_CAMERA_TO_XYZ_D50",
                "primary_linearraw_is_full_colour=1",
                "primary_is_jpeg_snapshot=0",
                "downstream_edit_manifest_begin",
                "schema=TruthRawTruthNegative200MpFullColour/0.1",
                "primary_image_role=TRUTHNEGATIVE_DENSE_CAMERA_NATIVE_RGB_FLOAT32",
                "stored_primary_space=CAMERA_NATIVE_SCIENTIFIC_MASTER_RGB_FLOAT32_DERIVATIVE",
                "source_scientific_master_width=4080",
                "source_scientific_master_height=3072",
                "target_width=16320",
                "target_height=12288",
                "scale_x=4",
                "scale_y=4",
                "target_geometry_matches_historical_camera5_envelope=1",
                "source_scientific_master_unchanged=1",
                "primary_raster_equals_scientific_master=0",
                "target_authority=RECONSTRUCTED_DENSE_SUPPORT",
                "measured_target_claim_count=0",
                "compute_backend=",
                "accelerator_eligible=",
                "accelerator_used=",
                "appearance_baked_into_primary=0",
                "negative_components_preserved=1",
                "over_one_components_preserved=1",
                "jpeg_role=NON_AUTHORITY_PREVIEW_ONLY",
                "lightroom_editable_primary=1",
                "scientific_writeback_allowed=0",
                "creates_new_evidence=0",
                "downstream_edit_manifest_end",
            )
        }
        if (flavor == Float32DngExportFlavor.ADVANCED_RENDER_EDIT) {
            val bakedAppearance = advancedAppearanceBaked(advancedFlags)
            requiredMarkers += listOf(
                "derivative_projection=1",
                "projected_raster_sha256=",
                "open_scene_state_sha256=",
                "restoration_derivative=0",
                "projected_appearance_applied=" + if (bakedAppearance) "1" else "0",
                "downstream_edit_manifest_begin",
                "schema=TruthRawAdvancedRenderEdit/0.1",
                "derivative_identity_space=EXTENDED_LINEAR_SRGB_FLOAT32",
                "stored_primary_space=XYZ_D50_LINEAR_FLOAT32",
                "storage_transform=LINEAR_SRGB_TO_XYZ_D50",
                "source_scientific_master_unchanged=1",
                "negative_components_preserved=1",
                "over_one_components_preserved=1",
                "advanced_flags=$advancedFlags",
                "detail_strength_percent=${detailStrengthFromFlags(advancedFlags)}",
                "color_fullness=${colorFullnessFromFlags(advancedFlags)}",
                "color_fullness_role=APPEARANCE_ONLY_LUMINANCE_PRESERVING",
                "exposure_compensation_ev=${exposureFromFlags(advancedFlags)}",
                "shadow_recovery_level=${shadowLevelFromFlags(advancedFlags)}",
                "tone_controls_role=APPEARANCE_ONLY_NO_BLACKLEVEL_WRITEBACK",
                "detail_baked_into_primary=" + if ((advancedFlags and 0x04) != 0) "1" else "0",
                "light_baked_into_primary=" + if ((advancedFlags and 0x01) != 0) "1" else "0",
                "restoration_baked_into_primary=" + if ((advancedFlags and 0x08) != 0) "1" else "0",
                "restoration_role=AESTHETIC_REINTEGRATION_ONLY",
                "natural_hdr_baked_into_primary=0",
                "natural_hdr_recipe_only=" + if ((advancedFlags and 0x02) != 0) "1" else "0",
                "hdr_authority=APPEARANCE_ONLY_OUTPUT_CHANNEL_MAP_HAS_UNKNOWN",
                "output_acutance_baked_into_primary=0",
                "lightroom_editable_primary=1",
                "scientific_writeback_allowed=0",
                "creates_new_evidence=0",
                "downstream_edit_manifest_end",
            )
        }
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

            val hashKeys = mutableListOf(
                "sealed_source_sha256=",
                "scientific_master_sha256=",
                "zero_line_sha256=",
                "scene_scale_sha256=",
            )
            if (flavor == Float32DngExportFlavor.ADVANCED_RENDER_EDIT) {
                hashKeys += "projected_raster_sha256="
                hashKeys += "open_scene_state_sha256="
            }
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

            val backplaneHex = valueOf("technical_backplane_serialized_hex=").orEmpty()
            if (!isHex(backplaneHex, 180 * 2)) {
                return PostWriteVerification(
                    false,
                    "Technical Backplane is niet exact 180 bytes",
                )
            }

            fun decodeHex(value: String): ByteArray {
                val out = ByteArray(value.length / 2)
                var i = 0
                while (i < value.length) {
                    out[i / 2] = value.substring(i, i + 2).toInt(16).toByte()
                    i += 2
                }
                return out
            }
            fun ByteArray.hexRange(from: Int, until: Int): String =
                copyOfRange(from, until).joinToString(separator = "") { byte ->
                    "%02x".format(byte.toInt() and 0xff)
                }
            fun u16Le(data: ByteArray, offset: Int): Int =
                (data[offset].toInt() and 0xff) or
                    ((data[offset + 1].toInt() and 0xff) shl 8)
            fun u32Le(data: ByteArray, offset: Int): Long =
                ((data[offset].toLong() and 0xffL) or
                    ((data[offset + 1].toLong() and 0xffL) shl 8) or
                    ((data[offset + 2].toLong() and 0xffL) shl 16) or
                    ((data[offset + 3].toLong() and 0xffL) shl 24)) and 0xffffffffL

            val backplane = decodeHex(backplaneHex)
            if (!backplane.copyOfRange(0, 8).contentEquals("TRBACK01".toByteArray(Charsets.US_ASCII))) {
                return PostWriteVerification(false, "Technical Backplane magic klopt niet")
            }
            if (u16Le(backplane, 8) != 1 || u16Le(backplane, 10) != 180) {
                return PostWriteVerification(false, "Technical Backplane versie/lengte klopt niet")
            }
            if (u32Le(backplane, 12) != 0L) {
                return PostWriteVerification(false, "Technical Backplane forbiddenFlags is niet nul")
            }
            if (u32Le(backplane, 144) != 1L || u32Le(backplane, 148) != 1L) {
                return PostWriteVerification(false, "Technical Backplane frame/evidence is niet 1/1")
            }
            if ((165 until 176).any { backplane[it] != 0.toByte() }) {
                return PostWriteVerification(false, "Technical Backplane reserved bytes zijn niet nul")
            }

            val bindingPairs = listOf(
                "sealed_source_sha256=" to backplane.hexRange(16, 48),
                "scientific_master_sha256=" to backplane.hexRange(48, 80),
                "zero_line_sha256=" to backplane.hexRange(80, 112),
                "scene_scale_sha256=" to backplane.hexRange(112, 144),
            )
            val mismatchedBinding = bindingPairs.firstOrNull { (key, embedded) ->
                !valueOf(key).orEmpty().equals(embedded, ignoreCase = true)
            }
            if (mismatchedBinding != null) {
                return PostWriteVerification(
                    false,
                    "Technical Backplane lineage mismatch: ${mismatchedBinding.first}",
                )
            }

            val crcText = valueOf("technical_backplane_crc32=").orEmpty()
            if (!crcText.startsWith("0x") || !isHex(crcText.drop(2), 8)) {
                return PostWriteVerification(false, "ongeldige Technical Backplane CRC32")
            }
            if (valueOf("technical_backplane_crc_scope=") != "PREFIX_176_BYTES") {
                return PostWriteVerification(false, "Technical Backplane CRC scope klopt niet")
            }

            val crcEngine = CRC32()
            crcEngine.update(backplane, 0, 176)
            val recomputedCrc = crcEngine.value and 0xffffffffL
            val embeddedCrc = u32Le(backplane, 176)
            val declaredCrc = crcText.drop(2).toLong(16) and 0xffffffffL
            if (recomputedCrc != embeddedCrc) {
                return PostWriteVerification(
                    false,
                    "Technical Backplane interne CRC32 faalt: recomputed=%08x embedded=%08x"
                        .format(recomputedCrc, embeddedCrc),
                )
            }
            if (declaredCrc != recomputedCrc) {
                return PostWriteVerification(
                    false,
                    "DNG metadata CRC32 faalt: declared=%08x recomputed=%08x"
                        .format(declaredCrc, recomputedCrc),
                )
            }

            if (valueOf("precision_policy_id=").isNullOrBlank() ||
                valueOf("runtime_reconstruction_backend_id=").isNullOrBlank()
            ) {
                return PostWriteVerification(false, "precision/runtime provenance ontbreekt")
            }

            PostWriteVerification(
                true,
                when (flavor) {
                    Float32DngExportFlavor.PURE ->
                        "v0.63 PURE contract + Backplane CRC inhoudelijk geverifieerd"
                    Float32DngExportFlavor.FULL_COLOUR_SCIENTIFIC_MASTER ->
                        "Full Colour Scientific Master Float32 camera-native primary + lineage geverifieerd"
                    Float32DngExportFlavor.ADVANCED_RENDER_EDIT ->
                        "ADVANCED Render/Edit derivative + projected-raster/Open-Scene binding geverifieerd"
                    Float32DngExportFlavor.TRUTHNEGATIVE_200MP_FULL_COLOUR ->
                        "TruthNegative 200MP full-colour Float32 derivative + fail-closed authority geverifieerd"
                },
            )
        } catch (error: Throwable) {
            PostWriteVerification(
                false,
                error.message ?: error.javaClass.simpleName,
            )
        }
    }

    private fun decode(
        packet: LongArray,
        flavor: Float32DngExportFlavor,
        advancedFlags: Int,
    ): PureFloat32DngExportResult {
        if (packet.size != PURE_FLOAT_PACKET_LONGS || packet[0] != PURE_FLOAT_MAGIC) {
            return PureFloat32DngExportResult.Failed(
                "Ongeldig native TRUTHRAW PURE Float32-resultaat.",
            )
        }
        val status = packet[1]
        if (status != 0L) {
            return PureFloat32DngExportResult.Failed(statusDescription(status))
        }

        val outputAuthorityArtifactSha256 = buildString(64) {
            for (word in 0 until 8) {
                val value = packet[26 + word].toInt()
                for (byte in 0 until 4) {
                    append(((value ushr (byte * 8)) and 0xff).toString(16).padStart(2, '0'))
                }
            }
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
            outputChannelAuthorityAvailable = packet[18] != 0L,
            outputChannelAuthorityMappingMode = packet[19].toInt(),
            outputAuthorityCalibratedChannels = packet[20],
            outputAuthorityReconstructedChannels = packet[21],
            outputAuthorityCensoredChannels = packet[22],
            outputAuthorityUnknownChannels = packet[23],
            outputAuthorityCensoredSupportPixels = packet[24],
            outputAuthorityArtifactSha256 = outputAuthorityArtifactSha256,
        )

        val commonViolation =
            metrics.width <= 0L ||
                metrics.height <= 0L ||
                metrics.samplesPerPixel != 3L ||
                metrics.bitsPerSample != 32L ||
                metrics.outputBytes <= 0L ||
                metrics.projectedPixels != metrics.width * metrics.height ||
                metrics.tilesWritten <= 0L ||
                metrics.logicalResidentUpperBoundBytes <= 0L ||
                metrics.logicalResidentUpperBoundBytes > PURE_MAX_LOGICAL_RESIDENT_BYTES.toLong() ||
                metrics.counterfactualObservationCreated ||
                metrics.physicalFrameCount != 1L ||
                metrics.independentEvidenceCount != 1L ||
                metrics.colorClaimScopeCode !in 1L..2L ||
                !metrics.outputChannelAuthorityAvailable ||
                metrics.outputAuthorityReconstructedChannels != 0L ||
                metrics.outputAuthorityUnknownChannels <= 0L ||
                metrics.outputAuthorityCalibratedChannels +
                    metrics.outputAuthorityReconstructedChannels +
                    metrics.outputAuthorityCensoredChannels +
                    metrics.outputAuthorityUnknownChannels !=
                    metrics.projectedPixels * 3L ||
                metrics.outputAuthorityArtifactSha256.all { it == '0' }

        val authorityGeometryViolation =
            if (flavor == Float32DngExportFlavor.TRUTHNEGATIVE_200MP_FULL_COLOUR) {
                metrics.width != 16320L ||
                    metrics.height != 12288L ||
                    metrics.outputChannelAuthorityMappingMode != 2 ||
                    metrics.outputAuthorityCalibratedChannels != 0L ||
                    metrics.outputAuthorityCensoredChannels != 0L ||
                    metrics.outputAuthorityUnknownChannels != metrics.projectedPixels * 3L
            } else {
                metrics.outputChannelAuthorityMappingMode != 1
            }

        val expectedRenderAppearance = advancedAppearanceBaked(advancedFlags)
        val flavorViolation = when (flavor) {
            Float32DngExportFlavor.PURE,
            Float32DngExportFlavor.FULL_COLOUR_SCIENTIFIC_MASTER ->
                !metrics.scientificMasterIdentityVerified ||
                    metrics.appearanceApplied
            Float32DngExportFlavor.ADVANCED_RENDER_EDIT ->
                metrics.scientificMasterIdentityVerified ||
                    metrics.appearanceApplied != expectedRenderAppearance
            Float32DngExportFlavor.TRUTHNEGATIVE_200MP_FULL_COLOUR ->
                metrics.scientificMasterIdentityVerified ||
                    metrics.appearanceApplied
        }

        val violation =
            commonViolation || authorityGeometryViolation || flavorViolation

        if (violation) {
            return PureFloat32DngExportResult.Failed(
                "Fail-closed: Float32 DNG schond flavor-, representation- of evidencecontract.",
            )
        }

        return PureFloat32DngExportResult.Success(metrics)
    }

    private fun statusDescription(status: Long): String = when (status) {
        -1L -> "PURE Float32: ongeldige exportparameters."
        -2L -> "PURE Float32: pre-master authority-state was niet canoniek."
        -3L -> "PURE Float32: Phase-2/Scientific-Master/source binding kwam niet exact overeen."
        -4L -> "PURE Float32: projection probeerde master/appearance/evidence-invariant te schenden."
        -5L -> "Float32 DNG: ingebedde JPEG-preview kon niet veilig worden gelezen."
        -6L -> "Float32 DNG: canonical Open Scene v0.70 kon niet worden gebonden."
        -7L -> "Float32 DNG: v0.79 uncertainty-admission promoveerde onverwacht authority."
        -8L -> "Float32 DNG: v0.78 source-channel authority kon niet fail-closed worden opgebouwd."
        -9L -> "Float32 DNG: v0.84 output-channel authority kon niet fail-closed worden opgebouwd."
        -10L -> "ADVANCED Render/Edit: sealed exposure-analyse faalde fail-closed."
        -11L -> "TruthNegative 200MP: alleen admitted Camera-5 4080×3072 mag deze projectieroute gebruiken."
        -12L -> "TruthNegative 200MP: dense projectiebron kon niet veilig worden opgebouwd."
        -13L -> "TruthNegative 200MP: projected-raster identity/authority-contract faalde fail-closed."

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
