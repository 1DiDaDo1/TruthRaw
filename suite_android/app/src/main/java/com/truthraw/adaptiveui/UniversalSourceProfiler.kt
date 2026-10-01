package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import android.os.ParcelFileDescriptor
import org.json.JSONArray
import org.json.JSONObject
import java.security.MessageDigest
import java.io.File

/**
 * Universal read-only intake profiler for the full D.RAW suite.
 *
 * It looks at two independent sides of the same immutable source:
 * 1. back side: bytes/container/source metadata;
 * 2. front side: visible scene structure through FrontsideSceneInspector.
 *
 * Device-specific maps are never required to decide scientific truth. Unknown
 * facts stay UNKNOWN. Frontside interpretation is APPEARANCE_DERIVED_ONLY.
 */
object UniversalSourceProfiler {

    fun profile(
        resolver: ContentResolver,
        source: RawHandle,
        cacheDir: File,
        progress: ((String) -> Unit)? = null,
        derivedStageCacheDir: File? = null,
    ): JSONObject {
        val trace = ResearchPerformanceDiagnosticsV01.ProfileTrace()
        var result: JSONObject? = null
        var cacheReleaseAttempted = false
        var cacheReleaseSucceeded = false

        try {
            val completed =
                profileInternal(
                    resolver = resolver,
                    source = source,
                    cacheDir = cacheDir,
                    progress = { event ->
                        trace.onProgress(event)
                        progress?.invoke(event)
                    },
                    derivedStageCacheDir = derivedStageCacheDir,
                )
            result = completed
            return completed
        } finally {
            // Bound the native shared preparation to exactly one profile,
            // regardless of whether this profiler was invoked from the
            // foreground Research service or another read-only intake path.
            cacheReleaseAttempted = true
            cacheReleaseSucceeded =
                runCatching {
                    TruthNegativeN2FactoredConfidenceBridge
                        .clearSharedPipelineCache()
                }.getOrDefault(false)

            result?.let {
                trace.attach(
                    profile = it,
                    sharedCacheReleaseAttempted =
                        cacheReleaseAttempted,
                    sharedCacheReleaseSucceeded =
                        cacheReleaseSucceeded,
                )
            }
        }
    }

    private fun profileInternal(
        resolver: ContentResolver,
        source: RawHandle,
        cacheDir: File,
        progress: ((String) -> Unit)? = null,
        derivedStageCacheDir: File? = null,
    ): JSONObject {
        progress?.invoke("SOURCE_SHA256")
        val sourceSha256 = sha256(resolver, source.uri)
        val byteLength = source.declaredSizeBytes ?: queryLength(resolver, source.uri)

        fun cachedStage(
            stageId: String,
            inputFingerprint: String,
            compute: () -> JSONObject,
        ): JSONObject {
            val cached =
                derivedStageCacheDir?.let {
                    ResearchProfileStageCacheV01.load(
                        filesDir = it,
                        sourceSha256 = sourceSha256,
                        stageId = stageId,
                        inputFingerprint = inputFingerprint,
                    )
                }
            if (cached != null) {
                progress?.invoke(stageId + "_CACHE_HIT")
                return cached
            }
            progress?.invoke(stageId)
            val result = compute()
            derivedStageCacheDir?.let {
                ResearchProfileStageCacheV01.save(
                    filesDir = it,
                    sourceSha256 = sourceSha256,
                    stageId = stageId,
                    inputFingerprint = inputFingerprint,
                    result = result,
                )
            }
            return result
        }

        val base = JSONObject()
            .put("schema", "D.RAW/UniversalSourceProfile/0.2")
            .put("status", "AUTO_PROFILED_IN_FULL_DRAW_SUITE")
            .put("source_sha256", sourceSha256)
            .put("processing_source_sha256", sourceSha256)
            .put(
                "upstream_sealed_source_sha256",
                source.upstreamSealedSourceSha256 ?: JSONObject.NULL,
            )
            .put(
                "upstream_source_role",
                source.upstreamSourceRole ?: JSONObject.NULL,
            )
            .put(
                "acquisition_evidence_sha256",
                source.acquisitionEvidenceSha256 ?: JSONObject.NULL,
            )
            .put(
                "camera_processing_source_is_derived_container",
                source.sourceRoute == SourceIngressRoute.CAMERA_CAPTURE &&
                    source.upstreamSealedSourceSha256 != null,
            )
            .put(
                "source_lineage",
                JSONObject()
                    .put("processing_source_sha256", sourceSha256)
                    .put(
                        "physical_evidence_root_sha256",
                        source.upstreamSealedSourceSha256 ?: sourceSha256,
                    )
                    .put(
                        "upstream_sealed_source_sha256",
                        source.upstreamSealedSourceSha256 ?: JSONObject.NULL,
                    )
                    .put(
                        "acquisition_evidence_sha256",
                        source.acquisitionEvidenceSha256 ?: JSONObject.NULL,
                    )
                    .put(
                        "processing_source_is_derived_from_upstream",
                        source.upstreamSealedSourceSha256 != null,
                    )
                    .put("processing_source_becomes_new_physical_frame", false)
                    .put("physical_frame_count_increment", 0),
            )
            .put("display_name", source.displayName)
            .put("byte_length", byteLength ?: JSONObject.NULL)
            .put("source_route", source.sourceRoute.name)
            .put("format_registry_id", source.format.id)
            .put("format_registry_label", source.format.displayLabel)
            .put("decoder_backend", source.format.decoderBackend.name)
            .put("device_specific_mapping_used", false)
            .put("unknown_is_valid", true)
            .put("full_draw_suite_integrated", true)
            .put("extensions", JSONObject())

        progress?.invoke("CONTAINER_SNIFF")
        val sniff = sniffContainer(resolver, source.uri)
        base.put("container_sniff", sniff)

        if (sniff.optString("family") != "CLASSIC_TIFF") {
            val frontside =
                FrontsideSceneInspector.inspect(
                    resolver,
                    source.uri,
                    null,
                    sourceSha256,
                )
            val physicalNoiseContext =
                PhysicalObservationNoiseContextV01.describe(
                    sourceSha256 = sourceSha256,
                    metadata = JSONObject(),
                    raster = JSONObject(),
                    backsideSignalSupport = null,
                    opticalFieldChart = null,
                )
            val atlas =
                UniversalObservationCalibrationAtlasV01.describe(
                    sourceSha256 = sourceSha256,
                    sourceClass = "OPAQUE_RAW_OR_IMAGE_CONTAINER",
                    sourceRoute = source.sourceRoute.name,
                    metadata = JSONObject(),
                    raster = JSONObject(),
                    sampleLattice = JSONObject().put("status", "UNAVAILABLE"),
                    frontside = frontside,
                    backsideSignalSupport = null,
                    opticalFieldChart = null,
                )
            return base
                .put("scientific_source_class", "OPAQUE_RAW_OR_IMAGE_CONTAINER")
                .put("metadata_parse_status", "NOT_CLASSIC_TIFF")
                .put("source_metadata", JSONObject())
                .put(
                    "route_hints",
                    JSONArray()
                        .put("KEEP_ORIGINAL_SOURCE_SEALED")
                        .put("USE_FORMAT_ADAPTER_POLICY_WITHOUT_DEVICE_MAP")
                        .put(
                            if (source.format.decoderBackend == RawDecoderBackend.DECODER_PENDING) {
                                "FORMAT_ADAPTER_PENDING"
                            } else {
                                "USE_EXISTING_FORMAT_ADAPTER"
                            },
                        ),
                )
                .put("scene_analysis", frontside)
                .put("physical_observation_noise_context_v0_1", physicalNoiseContext)
                .put("universal_observation_calibration_atlas", atlas)
                .put("authority", authorityBlock())
                .put("open_world", openWorldBlock())
        }

        progress?.invoke("TIFF_DNG_METADATA")
        val parsed = try {
            parseClassicTiff(resolver, source.uri)
        } catch (e: Exception) {
            val frontside =
                FrontsideSceneInspector.inspect(
                    resolver,
                    source.uri,
                    null,
                    sourceSha256,
                )
            val physicalNoiseContext =
                PhysicalObservationNoiseContextV01.describe(
                    sourceSha256 = sourceSha256,
                    metadata = JSONObject(),
                    raster = JSONObject(),
                    backsideSignalSupport = null,
                    opticalFieldChart = null,
                )
            val atlas =
                UniversalObservationCalibrationAtlasV01.describe(
                    sourceSha256 = sourceSha256,
                    sourceClass = "TIFF_CONTAINER_METADATA_PARSE_FAILED",
                    sourceRoute = source.sourceRoute.name,
                    metadata = JSONObject(),
                    raster = JSONObject(),
                    sampleLattice = JSONObject().put("status", "UNAVAILABLE"),
                    frontside = frontside,
                    backsideSignalSupport = null,
                    opticalFieldChart = null,
                )
            return base
                .put("scientific_source_class", "TIFF_CONTAINER_METADATA_PARSE_FAILED")
                .put("metadata_parse_status", "FAILED")
                .put("metadata_error", e.message ?: e.javaClass.simpleName)
                .put("source_metadata", JSONObject())
                .put(
                    "route_hints",
                    JSONArray()
                        .put("KEEP_ORIGINAL_SOURCE_SEALED")
                        .put("FAIL_CLOSED_OR_VERSIONED_COMPATIBILITY_ADAPTER"),
                )
                .put("scene_analysis", frontside)
                .put("physical_observation_noise_context_v0_1", physicalNoiseContext)
                .put("universal_observation_calibration_atlas", atlas)
                .put("authority", authorityBlock())
                .put("open_world", openWorldBlock())
        }

        val ifds = parsed.optJSONArray("ifds") ?: JSONArray()
        val rawCandidates = parsed.optJSONArray("rawCfaIfdCandidates") ?: JSONArray()

        val make = findTagValue(ifds, "Make")
        val model = findTagValue(ifds, "Model")
        val uniqueCameraModel = findTagValue(ifds, "UniqueCameraModel")
        val software = findTagValue(ifds, "Software")
        val dateTime = findTagValue(ifds, "DateTime")
        val dateTimeOriginal = findTagValue(ifds, "DateTimeOriginal")
        val dateTimeDigitized = findTagValue(ifds, "DateTimeDigitized")
        val subSecTime = findTagValue(ifds, "SubSecTime")
        val subSecTimeOriginal = findTagValue(ifds, "SubSecTimeOriginal")
        val subSecTimeDigitized = findTagValue(ifds, "SubSecTimeDigitized")
        val offsetTime = findTagValue(ifds, "OffsetTime")
        val offsetTimeOriginal = findTagValue(ifds, "OffsetTimeOriginal")
        val offsetTimeDigitized = findTagValue(ifds, "OffsetTimeDigitized")
        val orientation = numberValue(findTagValue(ifds, "Orientation"))?.toInt()
        val exposureTime = rationalValue(findTagValue(ifds, "ExposureTime"))
        val fNumber = rationalValue(findTagValue(ifds, "FNumber"))
        val iso = numberValue(findTagValue(ifds, "ISOSpeedRatings"))
        val focalLength = rationalValue(findTagValue(ifds, "FocalLength"))
        val dngVersion = findTagValue(ifds, "DNGVersion")
        val dngBackward = findTagValue(ifds, "DNGBackwardVersion")
        val asShotNeutral = findTagValue(ifds, "AsShotNeutral")
        val colorMatrix1 = findTagValue(ifds, "ColorMatrix1")
        val colorMatrix2 = findTagValue(ifds, "ColorMatrix2")
        val forwardMatrix1 = findTagValue(ifds, "ForwardMatrix1")
        val forwardMatrix2 = findTagValue(ifds, "ForwardMatrix2")
        val noiseProfile = findTagValue(ifds, "NoiseProfile")
        val calibration1 = findTagValue(ifds, "CameraCalibration1")
        val calibration2 = findTagValue(ifds, "CameraCalibration2")

        val primaryRaw = largestRawCandidate(rawCandidates)
        val width = numberValue(primaryRaw?.opt("imageWidth"))?.toLong()
        val height = numberValue(primaryRaw?.opt("imageLength"))?.toLong()

        val metadata = JSONObject()
            .put("make", valueOrNull(make))
            .put("model", valueOrNull(model))
            .put("unique_camera_model", valueOrNull(uniqueCameraModel))
            .put("software", valueOrNull(software))
            .put("date_time", valueOrNull(dateTime))
            .put("date_time_original", valueOrNull(dateTimeOriginal))
            .put("date_time_digitized", valueOrNull(dateTimeDigitized))
            .put("subsec_time", valueOrNull(subSecTime))
            .put("subsec_time_original", valueOrNull(subSecTimeOriginal))
            .put("subsec_time_digitized", valueOrNull(subSecTimeDigitized))
            .put("offset_time", valueOrNull(offsetTime))
            .put("offset_time_original", valueOrNull(offsetTimeOriginal))
            .put("offset_time_digitized", valueOrNull(offsetTimeDigitized))
            .put("capture_time_preferred_text", valueOrNull(dateTimeOriginal ?: dateTime))
            .put("capture_subsec_preferred_text", valueOrNull(subSecTimeOriginal ?: subSecTime))
            .put("capture_offset_preferred_text", valueOrNull(offsetTimeOriginal ?: offsetTime))
            .put("orientation", orientation ?: JSONObject.NULL)
            .put("dng_version", valueOrNull(dngVersion))
            .put("dng_backward_version", valueOrNull(dngBackward))
            .put("exposure_time_seconds", exposureTime ?: JSONObject.NULL)
            .put("f_number", fNumber ?: JSONObject.NULL)
            .put("iso", iso ?: JSONObject.NULL)
            .put("focal_length_mm", focalLength ?: JSONObject.NULL)
            .put("as_shot_neutral", valueOrNull(asShotNeutral))
            .put("color_matrix_1_present", colorMatrix1 != null)
            .put("color_matrix_2_present", colorMatrix2 != null)
            .put("camera_calibration_1_present", calibration1 != null)
            .put("camera_calibration_2_present", calibration2 != null)
            .put("forward_matrix_1_present", forwardMatrix1 != null)
            .put("forward_matrix_2_present", forwardMatrix2 != null)
            .put("noise_profile_present", noiseProfile != null)

        val hasStripStorage =
            primaryRaw?.has("stripOffsets") == true &&
                primaryRaw.has("stripByteCounts")
        val hasTileStorage =
            primaryRaw?.has("tileOffsets") == true &&
                primaryRaw.has("tileByteCounts")
        val storageKind = when {
            hasStripStorage && hasTileStorage -> "AMBIGUOUS_STRIPS_AND_TILES"
            hasStripStorage -> "STRIPS"
            hasTileStorage -> "TILES"
            else -> "UNKNOWN"
        }

        val raster = JSONObject()
            .put("raw_cfa_candidate_count", rawCandidates.length())
            .put("width", width ?: JSONObject.NULL)
            .put("height", height ?: JSONObject.NULL)
            .put("bits_per_sample", valueOrNull(primaryRaw?.opt("bitsPerSample")))
            .put("compression", valueOrNull(primaryRaw?.opt("compression")))
            .put("samples_per_pixel", valueOrNull(primaryRaw?.opt("samplesPerPixel")))
            .put("fill_order", valueOrNull(primaryRaw?.opt("fillOrder")))
            .put("planar_configuration", valueOrNull(primaryRaw?.opt("planarConfiguration")))
            .put("sample_format", valueOrNull(primaryRaw?.opt("sampleFormat")))
            .put("storage_kind", storageKind)
            .put("rows_per_strip", valueOrNull(primaryRaw?.opt("rowsPerStrip")))
            .put("tile_width", valueOrNull(primaryRaw?.opt("tileWidth")))
            .put("tile_length", valueOrNull(primaryRaw?.opt("tileLength")))
            .put("opcode_list_2_present", primaryRaw?.has("opcodeList2") == true)
            .put(
                "opcode_list_2_metadata",
                valueOrNull(primaryRaw?.opt("opcodeList2")),
            )
            .put("cfa_repeat_pattern_dim", valueOrNull(primaryRaw?.opt("cfaRepeatPatternDim")))
            .put("cfa_pattern", valueOrNull(primaryRaw?.opt("cfaPattern")))
            .put("black_level", valueOrNull(primaryRaw?.opt("blackLevel")))
            .put("white_level", valueOrNull(primaryRaw?.opt("whiteLevel")))
            .put("active_area", valueOrNull(primaryRaw?.opt("activeArea")))
            .put("default_crop_size", valueOrNull(primaryRaw?.opt("defaultCropSize")))

        val sourceClass = when {
            rawCandidates.length() > 0 -> "DNG_CFA_RAW"
            dngVersion != null || uniqueCameraModel != null ->
                "DNG_NON_CFA_OR_UNSUPPORTED_RAW_LAYOUT"
            else -> "TIFF_IMAGE_OR_UNKNOWN"
        }

        val latticeWidth =
            width?.takeIf { it in 1..Int.MAX_VALUE.toLong() }?.toInt()
        val latticeHeight =
            height?.takeIf { it in 1..Int.MAX_VALUE.toLong() }?.toInt()
        val sampleLattice =
            RasterIndependentSampleLatticeV01.describe(
                sourceSha256 = sourceSha256,
                sourceWidth = latticeWidth,
                sourceHeight = latticeHeight,
                sourceClass = sourceClass,
                cfaPattern = primaryRaw?.opt("cfaPattern"),
            )

        val routeHints = JSONArray().put("KEEP_ORIGINAL_SOURCE_SEALED")
        if (rawCandidates.length() > 0) {
            routeHints.put("TRY_DIRECT_COMMON_DNG_SCIENTIFIC_INGRESS")
        }
        if (orientation != null && orientation !in 1..8) {
            routeHints.put("ORIENTATION_METADATA_INVALID_CHECK_VERSIONED_COMPATIBILITY_INGRESS")
        }
        if (rawCandidates.length() == 0) {
            routeHints.put("DO_NOT_ASSUME_CFA_SCIENTIFIC_SOURCE")
        }

        val sourceIdentityHint = JSONObject()
            .put("make", valueOrNull(make))
            .put("model", valueOrNull(model))
            .put("unique_camera_model", valueOrNull(uniqueCameraModel))
            .put("authority", "SOURCE_METADATA_BOUND_NOT_DEVICE_CALIBRATION")

        val optics = JSONObject()
            .put("focal_length_mm", focalLength ?: JSONObject.NULL)
            .put("f_number", fNumber ?: JSONObject.NULL)
            .put("lens_role", "UNKNOWN")
            .put("lens_role_authority", "UNKNOWN")
            .put("field_of_view", "UNKNOWN")
            .put("optical_support", "UNKNOWN")
            .put(
                "note",
                "Focal length alone does not prove lens role, sensor crop, field of view or optical resolving support.",
            )

        val backsideSignalSupport =
            cachedStage(
                stageId = "BACKSIDE_SIGNAL_FIELD_V01_R1",
                inputFingerprint =
                    ResearchProfileStageCacheV01.fingerprint(
                        parsed.toString(),
                        primaryRaw?.toString(),
                    ),
            ) {
                BacksideSignalSupportAudit.analyze(
                    resolver,
                    source.uri,
                    parsed,
                    primaryRaw,
                    sourceSha256,
                )
            }

        val darkChromaBacksideSupport = JSONObject()
            .put("authority", "SOURCE_METADATA_BOUND_HINT_PLUS_MEASURED_SIGNAL_BLOCKER")
            .put("metadata_hint_authority", "SOURCE_METADATA_BOUND_HINT_ONLY")
            .put("noise_profile_present", noiseProfile != null)
            .put("black_level_present", primaryRaw?.opt("blackLevel") != null)
            .put("white_level_present", primaryRaw?.opt("whiteLevel") != null)
            .put("local_noise_confirmation_available", false)
            .put("n2_local_support_bound", false)
            .put("signal_support_audit", backsideSignalSupport)

        val frontside =
            cachedStage(
                stageId = "FRONTSIDE_SCENE_V01_R1",
                inputFingerprint =
                    ResearchProfileStageCacheV01.fingerprint(
                        parsed.toString(),
                        darkChromaBacksideSupport.toString(),
                    ),
            ) {
                FrontsideSceneInspector.inspect(
                    resolver,
                    source.uri,
                    parsed,
                    sourceSha256,
                    darkChromaBacksideSupport,
                )
            }

        progress?.invoke("OPTICAL_FIELD_CHART")
        val observationOpticalFieldChart =
            ObservationOpticalFieldChartV01.describe(
                sourceSha256 = sourceSha256,
                sampleLattice = sampleLattice,
                primaryRawRaster = raster,
                frontside = frontside,
                optics = optics,
                measuredSignalProfile =
                    backsideSignalSupport.optJSONObject(
                        "observation_optical_field_signal_v0_1",
                    ),
            )

        val physicalNoiseContext =
            PhysicalObservationNoiseContextV01.describe(
                sourceSha256 = sourceSha256,
                metadata = metadata,
                raster = raster,
                backsideSignalSupport = backsideSignalSupport,
                opticalFieldChart = observationOpticalFieldChart,
            )

        progress?.invoke("OBSERVATION_CALIBRATION_ATLAS")
        val universalObservationCalibrationAtlas =
            UniversalObservationCalibrationAtlasV01.describe(
                sourceSha256 = sourceSha256,
                sourceClass = sourceClass,
                sourceRoute = source.sourceRoute.name,
                metadata = metadata,
                raster = raster,
                sampleLattice = sampleLattice,
                frontside = frontside,
                backsideSignalSupport = backsideSignalSupport,
                opticalFieldChart = observationOpticalFieldChart,
            )

        val n2LocalSpatialBinding =
            if (source.format.id == "DNG" && source.format.nativeProcessingReady) {
                val frontsideInput =
                    frontside.optJSONObject("dark_chroma_stability_v0_1")
                cachedStage(
                    stageId = "N2_LOCAL_SPATIAL_V01_R1",
                    inputFingerprint =
                        ResearchProfileStageCacheV01.fingerprint(
                            frontsideInput?.toString(),
                        ),
                ) {
                    N2LocalSpatialBindingAudit.analyze(
                        resolver = resolver,
                        sourceUri = source.uri,
                        sourceSha256 = sourceSha256,
                        cacheDir = cacheDir,
                        frontsideV01 = frontsideInput,
                    )
                }
            } else {
                N2LocalSpatialBindingAudit.unavailable(
                    sourceSha256,
                    "NATIVE_DNG_ROUTE_NOT_AVAILABLE",
                )
            }

        val darkChromaV04 =
            DarkChromaStabilityV04Audit.analyze(
                sourceSha256 = sourceSha256,
                v03 = frontside.optJSONObject("dark_chroma_stability_v0_3"),
                localN2 = n2LocalSpatialBinding,
            )

        val frontsideV01 =
            frontside.optJSONObject("dark_chroma_stability_v0_1")
        val frontsideV01Global =
            frontsideV01?.optJSONObject("global") ?: JSONObject()
        val visibleDarkChromaCandidates =
            frontsideV01Global.optLong(
                "frontside_chroma_instability_candidate_tiles",
                0L,
            )
        val v03State =
            frontside.optJSONObject("dark_chroma_stability_v0_3")
                ?.optString("global_information_state", "UNKNOWN")
                ?: "UNKNOWN"
        val nativeDngReady =
            source.format.id == "DNG" && source.format.nativeProcessingReady
        val fineStructureNeeded =
            nativeDngReady &&
                visibleDarkChromaCandidates > 0L &&
                !v03State.startsWith("DARK_UNINFORMATIVE")

        val n2StructureSupportBinding =
            when {
                fineStructureNeeded ->
                    cachedStage(
                        stageId = "N2_STRUCTURE_SUPPORT_V01_R1",
                        inputFingerprint =
                            ResearchProfileStageCacheV01.fingerprint(
                                frontsideV01?.toString(),
                            ),
                    ) {
                        N2StructureSupportBindingAudit.analyze(
                            resolver = resolver,
                            sourceUri = source.uri,
                            sourceSha256 = sourceSha256,
                            cacheDir = cacheDir,
                            frontsideV01 = frontsideV01,
                        )
                    }
                !nativeDngReady ->
                    N2StructureSupportBindingAudit.unavailable(
                        sourceSha256,
                        "NATIVE_DNG_ROUTE_NOT_AVAILABLE",
                    )
                visibleDarkChromaCandidates <= 0L ->
                    N2StructureSupportBindingAudit.skipped(
                        sourceSha256,
                        "NO_VISIBLE_DARK_CHROMA_CANDIDATES",
                    )
                else ->
                    N2StructureSupportBindingAudit.skipped(
                        sourceSha256,
                        "GLOBAL_DARK_UNINFORMATIVE_ALREADY_BLOCKS_CORRECTION",
                    )
            }

        val darkChromaV05 =
            DarkChromaStabilityV05Audit.analyze(
                sourceSha256 = sourceSha256,
                v03 = frontside.optJSONObject("dark_chroma_stability_v0_3"),
                v04 = darkChromaV04,
                fineStructure = n2StructureSupportBinding,
            )

        val n2SampleSupportDistance =
            when {
                fineStructureNeeded ->
                    cachedStage(
                        stageId = "N2_SAMPLE_SUPPORT_DISTANCE_V01_R1",
                        inputFingerprint =
                            ResearchProfileStageCacheV01.fingerprint(
                                frontsideV01?.toString(),
                                n2StructureSupportBinding.toString(),
                            ),
                    ) {
                        N2SampleSupportDistanceAudit.analyze(
                            resolver = resolver,
                            sourceUri = source.uri,
                            sourceSha256 = sourceSha256,
                            cacheDir = cacheDir,
                            frontsideV01 = frontsideV01,
                            fineStructure = n2StructureSupportBinding,
                        )
                    }
                !nativeDngReady ->
                    N2SampleSupportDistanceAudit.unavailable(
                        sourceSha256,
                        "NATIVE_DNG_ROUTE_NOT_AVAILABLE",
                    )
                visibleDarkChromaCandidates <= 0L ->
                    N2SampleSupportDistanceAudit.skipped(
                        sourceSha256,
                        "NO_VISIBLE_DARK_CHROMA_CANDIDATES",
                    )
                else ->
                    N2SampleSupportDistanceAudit.skipped(
                        sourceSha256,
                        "GLOBAL_DARK_UNINFORMATIVE_ALREADY_BLOCKS_CORRECTION",
                    )
            }

        val darkChromaV06 =
            DarkChromaStabilityV06Audit.analyze(
                sourceSha256 = sourceSha256,
                v03 = frontside.optJSONObject("dark_chroma_stability_v0_3"),
                v05 = darkChromaV05,
                supportDistance = n2SampleSupportDistance,
            )

        val n2SampleLatticeGeometry =
            RasterIndependentSampleLatticeV01.bindSupportGeometry(
                sourceSha256 = sourceSha256,
                lattice = sampleLattice,
                supportDistance = n2SampleSupportDistance,
            )

        val darkChromaV07 =
            DarkChromaStabilityV07Audit.analyze(
                sourceSha256 = sourceSha256,
                v06 = darkChromaV06,
                lattice = sampleLattice,
                latticeGeometry = n2SampleLatticeGeometry,
            )

        // Prospective model-bank policy MUST be frozen from observation
        // geometry before the hold-out solver is allowed to reveal/score any
        // hidden CFA target. It has no runtime dependency on hold-out results.
        val universalObservationModelSelection =
            UniversalObservationModelSelectionV01.analyze(
                sourceSha256 = sourceSha256,
                sampleLattice = sampleLattice,
                latticeGeometry = n2SampleLatticeGeometry,
            )

        val universalLocalModelBankHoldout =
            UniversalLocalModelBankHoldoutV01.describe(
                sourceSha256 = sourceSha256,
                nativeDngReady = nativeDngReady,
                sampleLattice = sampleLattice,
                prospectivePolicy = universalObservationModelSelection,
            )

        val universalLocalModelBankHoldoutV02 =
            UniversalLocalModelBankHoldoutV02.describe(
                sourceSha256 = sourceSha256,
                nativeDngReady = nativeDngReady,
                sampleLattice = sampleLattice,
                prospectivePolicy = universalObservationModelSelection,
            )

        val universalLocalModelBankHoldoutV03 =
            UniversalLocalModelBankHoldoutV03.describe(
                sourceSha256 = sourceSha256,
                nativeDngReady = nativeDngReady,
                sampleLattice = sampleLattice,
                prospectivePolicy = universalObservationModelSelection,
            )

        val anchorConstrainedReconstruction =
            when {
                fineStructureNeeded &&
                    n2SampleSupportDistance.optString("status") ==
                        "AUDIT_ONLY_DISTANCE_BINDING_AVAILABLE" &&
                    sampleLattice.optString("status") == "AVAILABLE" ->
                    cachedStage(
                        stageId = "ANCHOR_LOCAL_RECONSTRUCTION_V01_R1",
                        inputFingerprint =
                            ResearchProfileStageCacheV01.fingerprint(
                                frontsideV01?.toString(),
                                sampleLattice.toString(),
                                n2SampleSupportDistance.toString(),
                            ),
                    ) {
                        AnchorConstrainedLocalReconstructionAudit.analyze(
                            resolver = resolver,
                            sourceUri = source.uri,
                            sourceSha256 = sourceSha256,
                            cacheDir = cacheDir,
                            frontsideV01 = frontsideV01,
                            sampleLattice = sampleLattice,
                            supportDistance = n2SampleSupportDistance,
                        )
                    }
                !nativeDngReady ->
                    AnchorConstrainedLocalReconstructionAudit.unavailable(
                        sourceSha256,
                        "NATIVE_DNG_ROUTE_NOT_AVAILABLE",
                    )
                visibleDarkChromaCandidates <= 0L ->
                    AnchorConstrainedLocalReconstructionAudit.skipped(
                        sourceSha256,
                        "NO_VISIBLE_DARK_CHROMA_CANDIDATES",
                    )
                v03State.startsWith("DARK_UNINFORMATIVE") ->
                    AnchorConstrainedLocalReconstructionAudit.skipped(
                        sourceSha256,
                        "GLOBAL_DARK_UNINFORMATIVE_ALREADY_BLOCKS_RECONSTRUCTION_RESEARCH",
                    )
                else ->
                    AnchorConstrainedLocalReconstructionAudit.unavailable(
                        sourceSha256,
                        "EXACT_SUPPORT_GEOMETRY_OR_SAMPLE_LATTICE_NOT_AVAILABLE",
                    )
            }

        frontside
            .put("n2_local_spatial_binding_v0_1", n2LocalSpatialBinding)
            .put("dark_chroma_stability_v0_4", darkChromaV04)
            .put(
                "n2_structure_support_binding_v0_1",
                n2StructureSupportBinding,
            )
            .put("dark_chroma_stability_v0_5", darkChromaV05)
            .put(
                "n2_sample_support_distance_v0_1",
                n2SampleSupportDistance,
            )
            .put("dark_chroma_stability_v0_6", darkChromaV06)
            .put(
                "n2_raster_independent_sample_geometry_v0_1",
                n2SampleLatticeGeometry,
            )
            .put("dark_chroma_stability_v0_7", darkChromaV07)
            .put(
                "anchor_constrained_local_reconstruction_v0_1",
                anchorConstrainedReconstruction,
            )
            .put(
                "universal_observation_model_selection_v0_1",
                universalObservationModelSelection,
            )
            .put(
                "universal_local_model_bank_holdout_v0_2",
                universalLocalModelBankHoldoutV02,
            )
            .put(
                "universal_local_model_bank_holdout_v0_3",
                universalLocalModelBankHoldoutV03,
            )
            .put(
                "observation_optical_field_chart_v0_1",
                observationOpticalFieldChart,
            )
            .put(
                "universal_observation_calibration_atlas_v0_1",
                universalObservationCalibrationAtlas,
            )
            .put(
                "physical_observation_noise_context_v0_1",
                physicalNoiseContext,
            )

        progress?.invoke("PROFILE_ASSEMBLY")
        return base
            .put("scientific_source_class", sourceClass)
            .put("metadata_parse_status", "PASS_READ_ONLY")
            .put("container_metadata", parsed)
            .put("source_metadata", metadata)
            .put("primary_raw_raster", raster)
            .put("raster_independent_sample_lattice", sampleLattice)
            .put("source_identity_hint", sourceIdentityHint)
            .put("optics", optics)
            .put(
                "observation_optical_field_chart",
                observationOpticalFieldChart,
            )
            .put(
                "universal_observation_calibration_atlas",
                universalObservationCalibrationAtlas,
            )
            .put("route_hints", routeHints)
            .put("backside_signal_support", backsideSignalSupport)
            .put(
                "physical_observation_noise_context_v0_1",
                physicalNoiseContext,
            )
            .put("n2_local_spatial_binding", n2LocalSpatialBinding)
            .put(
                "n2_structure_support_binding",
                n2StructureSupportBinding,
            )
            .put(
                "n2_sample_support_distance",
                n2SampleSupportDistance,
            )
            .put(
                "n2_raster_independent_sample_geometry",
                n2SampleLatticeGeometry,
            )
            .put(
                "anchor_constrained_local_reconstruction",
                anchorConstrainedReconstruction,
            )
            .put(
                "universal_observation_model_selection",
                universalObservationModelSelection,
            )
            .put(
                "universal_local_model_bank_holdout",
                universalLocalModelBankHoldout,
            )
            .put(
                "universal_local_model_bank_holdout_v0_2",
                universalLocalModelBankHoldoutV02,
            )
            .put(
                "universal_local_model_bank_holdout_v0_3",
                universalLocalModelBankHoldoutV03,
            )
            .put("scene_analysis", frontside)
            .put("authority", authorityBlock())
            .put("open_world", openWorldBlock())
    }

    fun pairProfile(a: JSONObject, b: JSONObject): JSONObject {
        val aMeta = a.optJSONObject("source_metadata") ?: JSONObject()
        val bMeta = b.optJSONObject("source_metadata") ?: JSONObject()
        val aOptics = a.optJSONObject("optics") ?: JSONObject()
        val bOptics = b.optJSONObject("optics") ?: JSONObject()

        val aDevice = normalizedDevice(aMeta)
        val bDevice = normalizedDevice(bMeta)
        val sameDeviceHint = aDevice != null && bDevice != null && aDevice == bDevice

        val fa = aOptics.optDouble("focal_length_mm", Double.NaN)
        val fb = bOptics.optDouble("focal_length_mm", Double.NaN)
        val focalOrdering = when {
            fa.isFinite() && fb.isFinite() && fa < fb -> "A_SHORTER_FOCAL_LENGTH_THAN_B"
            fa.isFinite() && fb.isFinite() && fb < fa -> "B_SHORTER_FOCAL_LENGTH_THAN_A"
            fa.isFinite() && fb.isFinite() -> "EQUAL_REPORTED_FOCAL_LENGTH"
            else -> "UNKNOWN"
        }

        val aScene = a.optJSONObject("scene_analysis") ?: JSONObject()
        val bScene = b.optJSONObject("scene_analysis") ?: JSONObject()
        val aReady = aScene.optJSONObject("geometry_readiness")
            ?.optBoolean("natural_feature_geometry_candidate", false) ?: false
        val bReady = bScene.optJSONObject("geometry_readiness")
            ?.optBoolean("natural_feature_geometry_candidate", false) ?: false

        val aAspect = aScene.optJSONObject("proportions")
            ?.optDouble("aspect_ratio_width_over_height", Double.NaN)
            ?: Double.NaN
        val bAspect = bScene.optJSONObject("proportions")
            ?.optDouble("aspect_ratio_width_over_height", Double.NaN)
            ?: Double.NaN

        val aspectAgreement = if (aAspect.isFinite() && bAspect.isFinite()) {
            kotlin.math.abs(aAspect - bAspect) /
                kotlin.math.max(aAspect, bAspect) <= 0.03
        } else {
            false
        }

        return JSONObject()
            .put("schema", "D.RAW/UniversalSourcePairProfile/0.2")
            .put("same_device_metadata_hint", sameDeviceHint)
            .put("same_device_hint_authority", "SOURCE_METADATA_HINT_ONLY")
            .put("reported_focal_length_ordering", focalOrdering)
            .put("focal_ordering_is_field_of_view_authority", false)
            .put("frontside_a_available", aScene.optBoolean("decoded_preview_used", false))
            .put("frontside_b_available", bScene.optBoolean("decoded_preview_used", false))
            .put("frontside_natural_feature_candidate_a", aReady)
            .put("frontside_natural_feature_candidate_b", bReady)
            .put("frontside_aspect_ratio_compatible_hint", aspectAgreement)
            .put("frontside_hint_authority", "APPEARANCE_DERIVED_ONLY")
            .put("role_assignment", "DEFER_TO_SCENE_AND_SOURCE_EVIDENCE")
            .put(
                "geometry_route",
                if (aReady && bReady) {
                    "NATURAL_FEATURE_PAIR_MATCHING_CANDIDATE"
                } else {
                    "SCIENTIFIC_RECONSTRUCTION_THEN_SCENE_INSPECTION"
                },
            )
            .put("indexed_target_required", false)
            .put("metric_scale", "UNKNOWN_UNLESS_SOURCE_OR_SCENE_SUPPLIES_SCALE")
            .put("device_specific_mapping_used", false)
            .put("extensions", JSONObject())
    }

    private fun parseClassicTiff(
        resolver: ContentResolver,
        uri: Uri,
    ): JSONObject {
        val pfd = resolver.openFileDescriptor(uri, "r")
            ?: error("Bron kon niet read-only worden geopend.")
        val statSize = pfd.statSize
        ParcelFileDescriptor.AutoCloseInputStream(pfd).use { input ->
            val channel = input.channel
            val size = if (statSize >= 0L) statSize else channel.size()
            require(size >= 8L) { "TIFF/DNG bron is te kort." }
            return DngContainerMetadataParser.parse(channel, size)
        }
    }

    private fun sha256(
        resolver: ContentResolver,
        uri: Uri,
    ): String {
        val digest = MessageDigest.getInstance("SHA-256")
        resolver.openInputStream(uri)?.use { input ->
            val buffer = ByteArray(1024 * 1024)
            while (true) {
                val n = input.read(buffer)
                if (n <= 0) break
                digest.update(buffer, 0, n)
            }
        } ?: error("Bron kon niet voor SHA-256 worden gelezen.")
        return digest.digest().joinToString("") { "%02x".format(it) }
    }

    private fun queryLength(
        resolver: ContentResolver,
        uri: Uri,
    ): Long? {
        val pfd = resolver.openFileDescriptor(uri, "r") ?: return null
        val size = pfd.statSize
        pfd.close()
        return size.takeIf { it >= 0L }
    }

    private fun sniffContainer(
        resolver: ContentResolver,
        uri: Uri,
    ): JSONObject {
        val pfd = resolver.openFileDescriptor(uri, "r")
            ?: return JSONObject().put("family", "UNKNOWN").put("reason", "OPEN_FAILED")
        ParcelFileDescriptor.AutoCloseInputStream(pfd).use { input ->
            val bytes = ByteArray(4)
            var used = 0
            while (used < bytes.size) {
                val n = input.read(bytes, used, bytes.size - used)
                if (n <= 0) break
                used += n
            }
            if (used < 4) {
                return JSONObject().put("family", "UNKNOWN").put("reason", "TOO_SHORT")
            }
            val littleTiff =
                bytes[0] == 'I'.code.toByte() &&
                    bytes[1] == 'I'.code.toByte() &&
                    (bytes[2].toInt() and 0xff) == 42 &&
                    (bytes[3].toInt() and 0xff) == 0
            val bigTiff =
                bytes[0] == 'M'.code.toByte() &&
                    bytes[1] == 'M'.code.toByte() &&
                    (bytes[2].toInt() and 0xff) == 0 &&
                    (bytes[3].toInt() and 0xff) == 42
            return when {
                littleTiff -> JSONObject()
                    .put("family", "CLASSIC_TIFF")
                    .put("byte_order", "LITTLE_ENDIAN")
                bigTiff -> JSONObject()
                    .put("family", "CLASSIC_TIFF")
                    .put("byte_order", "BIG_ENDIAN")
                else -> JSONObject()
                    .put("family", "UNKNOWN_OR_VENDOR_RAW")
                    .put(
                        "magic_prefix_hex",
                        bytes.joinToString("") { "%02x".format(it.toInt() and 0xff) },
                    )
            }
        }
    }

    private fun authorityBlock(): JSONObject =
        JSONObject()
            .put("container_fields", "SOURCE_METADATA_BOUND")
            .put("source_sha256", "MEASURED")
            .put("frontside_scene", "APPEARANCE_DERIVED_ONLY")
            .put("lens_role", "UNKNOWN_UNLESS_INDEPENDENTLY_PROVEN")
            .put("scene_geometry", "NOT_YET_MEASURED")
            .put("metric_scale", "UNKNOWN_UNLESS_EVIDENCE_EXISTS")
            .put("creates_new_sensor_evidence", false)
            .put("scientific_writeback_allowed", false)

    private fun openWorldBlock(): JSONObject =
        JSONObject()
            .put("source_self_describes_when_possible", true)
            .put("frontside_is_intake_knowledge_source", true)
            .put("unknown_fields_remain_unknown", true)
            .put("future_format_adapters_allowed", true)
            .put("ai_or_learned_scene_models_allowed", false)
            .put("future_classical_scene_analysis_allowed", true)
            .put("sealed_source_does_not_seal_interpretation", true)
            .put("representation_may_exceed_source", true)
            .put("knowledge_claims_may_not_exceed_evidence", true)
            .put("scientific_coordinate_domain_can_be_raster_independent", true)
            .put("source_raster_defines_measurement_sampling_not_world_resolution", true)
            .put("unmeasured_coordinate_positions_remain_unknown", true)
            .put("coordinate_precision_does_not_create_evidence", true)

    private fun largestRawCandidate(array: JSONArray): JSONObject? {
        var best: JSONObject? = null
        var bestArea = -1.0
        for (i in 0 until array.length()) {
            val candidate = array.optJSONObject(i) ?: continue
            val width = numberValue(candidate.opt("imageWidth")) ?: continue
            val height = numberValue(candidate.opt("imageLength")) ?: continue
            val area = width * height
            if (area > bestArea) {
                bestArea = area
                best = candidate
            }
        }
        return best
    }

    private fun findTagValue(ifds: JSONArray, name: String): Any? {
        for (i in 0 until ifds.length()) {
            val entries = ifds.optJSONObject(i)?.optJSONArray("entries") ?: continue
            for (j in 0 until entries.length()) {
                val entry = entries.optJSONObject(j) ?: continue
                if (entry.optString("name") == name) {
                    val value = entry.opt("value")
                    if (value != null && value !== JSONObject.NULL) return value
                }
            }
        }
        return null
    }

    private fun numberValue(value: Any?): Double? = when (value) {
        is Number -> value.toDouble()
        is JSONObject ->
            if (value.has("value") && !value.isNull("value")) {
                value.optDouble("value")
            } else {
                null
            }
        else -> null
    }

    private fun rationalValue(value: Any?): Double? = numberValue(value)

    private fun normalizedDevice(metadata: JSONObject): String? {
        val unique = metadata.optString("unique_camera_model", "").trim()
        if (unique.isNotEmpty() && unique != "null") return unique.lowercase()
        val make = metadata.optString("make", "").trim()
        val model = metadata.optString("model", "").trim()
        if (make.isEmpty() && model.isEmpty()) return null
        return (make + "|" + model).lowercase()
    }

    private fun valueOrNull(value: Any?): Any = value ?: JSONObject.NULL
}
