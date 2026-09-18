#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

ns = {}
exec(
    compile(
        Path('tools/patch_fotograaf_v020_payload_geometry_decoder.py').read_text(),
        'patch_fotograaf_v020_payload_geometry_decoder.py',
        'exec',
    ),
    ns,
    ns,
)
s = p.read_text()

# v0.39 is a capture-context experiment only. It starts from v0.20 and adds no
# unknown vendor session/request control.
field_anchor = '    private var capturedRaw: File? = null\n'
field_insert = '''    private lateinit var geometryContextButton: Button
    private var selectedGeometryContextIndex = 0
    private var activeGeometryContext: Camera2RawGeometryContextLadder.Profile? = null
    private var discoveredRawRoutes: RawRoutes? = null
    private var lastGeometryLadderValidation: JSONObject? = null

'''
if field_anchor not in s:
    raise SystemExit('v0.39 field anchor missing')
s = s.replace(field_anchor, field_insert + field_anchor, 1)

# UI: add one selector while keeping the proven v0.20 buttons/source-first route.
ui_anchor = '''        saveGeometryPreviewButton = button("Stage 3.7 · geometry diagnostic PNG opslaan") { saveFile(capturedGeometryPreview, "image/png", REQUEST_SAVE_GEOMETRY_PREVIEW) }.apply { isEnabled = false }
'''
ui_repl = ui_anchor + '''        geometryContextButton = button("") {
            selectedGeometryContextIndex = Camera2RawGeometryContextLadder.nextIndex(selectedGeometryContextIndex)
            updateGeometryContextButton()
            captureButton.isEnabled = false
            setStatus("Context geselecteerd: ${Camera2RawGeometryContextLadder.profile(selectedGeometryContextIndex).label}. Start opnieuw Stap 2.")
        }
        updateGeometryContextButton()
'''
if ui_anchor not in s:
    raise SystemExit('v0.39 UI selector anchor missing')
s = s.replace(ui_anchor, ui_repl, 1)

add_anchor = '''        root.addView(previewButton)
        root.addView(captureButton)
'''
add_repl = '''        root.addView(previewButton)
        root.addView(geometryContextButton)
        root.addView(captureButton)
'''
if add_anchor not in s:
    raise SystemExit('v0.39 UI add anchor missing')
s = s.replace(add_anchor, add_repl, 1)

s = s.replace(
    'TruthRaw · 200MP Tele Test v0.20 · Camera-5 payload geometry decoder',
    'TruthRaw · Camera-5 RAW geometry/context ladder v0.39',
    1,
)
s = s.replace(
    'physical-5-scoped MAX request; exact RAW bytes are sealed first, HAL envelope and full raster are audited, then any populated prefix is matched read-only against advertised standard RAW geometries',
    'no unknown vendor intervention; compare advertised Camera-5 RAW output contexts 4080×3072 STANDARD, 8160×6144 MAX, and 16320×12288 MAX high-res; every source is sealed first before read-only geometry audits',
    1,
)
s = s.replace('STAGE 0 PASS · v0.20 payload-geometry UI', 'STAGE 0 PASS · v0.39 geometry/context ladder UI', 1)

# Capability discovery must prove all three exact advertised contexts before capture.
cap_anchor = '''                val maximumHigh = safeSizes { maximum?.getHighResolutionOutputSizes(ImageFormat.RAW_SENSOR) }

                val selected = when {
'''
cap_repl = '''                val maximumHigh = safeSizes { maximum?.getHighResolutionOutputSizes(ImageFormat.RAW_SENSOR) }

                val ladderValidation = Camera2RawGeometryContextLadder.validateAdvertisements(
                    standardOutput = standardOutput,
                    maximumOutput = maximumOutput,
                    maximumHigh = maximumHigh,
                )
                require(ladderValidation.optBoolean("allRequiredContextsAdvertised", false)) {
                    "v0.39 ladder niet volledig geadverteerd: $ladderValidation"
                }

                val selected = when {
'''
if cap_anchor not in s:
    raise SystemExit('v0.39 capability validation anchor missing')
s = s.replace(cap_anchor, cap_repl, 1)

cap_success = '''                    capabilitySource = packed.second.selectedSource
                    capabilityReady = true
'''
cap_success_repl = '''                    capabilitySource = "V039_GEOMETRY_CONTEXT_LADDER"
                    discoveredRawRoutes = packed.second
                    lastGeometryLadderValidation = Camera2RawGeometryContextLadder.validateAdvertisements(
                        standardOutput = packed.second.standardOutput,
                        maximumOutput = packed.second.maximumOutput,
                        maximumHigh = packed.second.maximumHigh,
                    )
                    capabilityReady = true
'''
if cap_success not in s:
    raise SystemExit('v0.39 capability success anchor missing')
s = s.replace(cap_success, cap_success_repl, 1)

# Capture function: bind the selected advertised geometry. STANDARD does not request MAX mode.
capture_start = s.index('    private fun capture200Mp() {')
capture_end = s.index('    private fun submitForcedPhysicalMaxStill(', capture_start)
capture = s[capture_start:capture_end]

profile_anchor = '''        val physical = physical5Characteristics ?: return

        captureButton.isEnabled = false
'''
profile_repl = '''        val physical = physical5Characteristics ?: return
        val routes = discoveredRawRoutes ?: run {
            setStatus("Stap 3 geblokkeerd: v0.39 RAW-routes ontbreken.")
            return
        }
        val profile = Camera2RawGeometryContextLadder.profile(selectedGeometryContextIndex)
        val advertised = when (profile.routeClass) {
            "STANDARD_MAP_OUTPUT" -> Camera2RawGeometryContextLadder.containsExact(routes.standardOutput, profile)
            "MAXIMUM_MAP_OUTPUT" -> Camera2RawGeometryContextLadder.containsExact(routes.maximumOutput, profile)
            "MAXIMUM_MAP_HIGH_RESOLUTION" -> Camera2RawGeometryContextLadder.containsExact(routes.maximumHigh, profile)
            else -> false
        }
        if (!advertised) {
            setStatus("STAGE 3 BLOCKED · ${profile.label} niet meer op verwachte route geadverteerd.")
            return
        }
        activeGeometryContext = profile

        captureButton.isEnabled = false
'''
if profile_anchor not in capture:
    raise SystemExit('v0.39 capture profile anchor missing')
capture = capture.replace(profile_anchor, profile_repl, 1)
capture = capture.replace(
    'ImageReader.newInstance(TARGET_W, TARGET_H, ImageFormat.RAW_SENSOR, 1)',
    'ImageReader.newInstance(profile.width, profile.height, ImageFormat.RAW_SENSOR, 1)',
    1,
)
output_old = '''        val outputSetup = runCatching {
            output.setPhysicalCameraId(PHYSICAL_ID)
            output.addSensorPixelModeUsed(CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)
        }
'''
output_new = '''        val outputSetup = runCatching {
            output.setPhysicalCameraId(PHYSICAL_ID)
            if (profile.useMaximumResolutionPixelMode) {
                output.addSensorPixelModeUsed(CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)
            }
        }
'''
if output_old not in capture:
    raise SystemExit('v0.39 output-mode anchor missing')
capture = capture.replace(output_old, output_new, 1)
capture = capture.replace('width = TARGET_W,', 'width = profile.width,', 1)
capture = capture.replace('height = TARGET_H,', 'height = profile.height,', 1)
capture = capture.replace(
    'outputMaximumResolutionModeDeclared = true,',
    'outputMaximumResolutionModeDeclared = profile.useMaximumResolutionPixelMode,',
    1,
)
capture = capture.replace(
    'submitForcedPhysicalMaxStill(device, s, logical)',
    'if (profile.useMaximumResolutionPixelMode) submitForcedPhysicalMaxStill(device, s, logical) else submitStandardPhysicalStill(device, s, logical)',
    1,
)
capture = capture.replace(
    '"STAGE 3 · physical-5/MAX session · isSessionConfigurationSupported=$support"',
    '"STAGE 3 · ${profile.label} · isSessionConfigurationSupported=$support"',
    1,
)
capture = capture.replace(
    '"STAGE 3 BLOCKED · Android meldt de exacte 200MP sessie unsupported."',
    '"STAGE 3 BLOCKED · Android meldt ${profile.label} unsupported."',
    1,
)
s = s[:capture_start] + capture + s[capture_end:]

# STANDARD request path: physical Camera 5, no SENSOR_PIXEL_MODE MAX and no unknown vendor key.
standard_submit = r'''    private fun submitStandardPhysicalStill(
        device: CameraDevice,
        s: CameraCaptureSession,
        logical: CameraCharacteristics,
    ) {
        val reader = rawReader ?: return
        try {
            val requestBuilder = try {
                lastScopedRequestUsed = true
                lastScopedRequestError = null
                device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE, setOf(PHYSICAL_ID))
            } catch (t: Throwable) {
                lastScopedRequestUsed = false
                lastScopedRequestError = "${t.javaClass.simpleName}: ${t.message}"
                device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE)
            }
            requestBuilder.addTarget(reader.surface)

            lastGlobalPixelModeWritten = false
            lastPhysicalPixelModeWritten = false
            lastPhysicalPixelModeAdvertised = physicalOverrideSupported(logical, CaptureRequest.SENSOR_PIXEL_MODE)
            lastPhysicalPixelModeForceError = null

            val physical = physical5Characteristics ?: error("physical characteristics ontbreken")
            lastPreHalRequestGate = runCatching {
                Camera2PreHalGate.observeRequest(
                    builder = requestBuilder,
                    logical = logical,
                    physical = physical,
                    physicalId = PHYSICAL_ID,
                    scopedPhysicalRequestUsed = lastScopedRequestUsed,
                    globalPixelModeWritten = false,
                    physicalPixelModeWritten = false,
                )
            }.getOrNull()

            val request = requestBuilder.build()
            setStatusAny(
                "STAGE 3 CAPTURE SENT · STANDARD 4080×3072 · scoped=$lastScopedRequestUsed · " +
                    "globalMAX=false · physicalMAX=false · unknownVendorKeys=0",
            )
            s.capture(request, object : CameraCaptureSession.CaptureCallback() {
                override fun onCaptureCompleted(
                    session: CameraCaptureSession,
                    request: CaptureRequest,
                    result: TotalCaptureResult,
                ) {
                    synchronized(pairLock) { pendingResult = result }
                    finalizeIfPaired()
                }

                override fun onCaptureFailed(
                    session: CameraCaptureSession,
                    request: CaptureRequest,
                    failure: CaptureFailure,
                ) {
                    setStatusAny(
                        "STAGE 3 STANDARD CAPTURE FAIL · reason=${failure.reason} · " +
                            "wasImageCaptured=${failure.wasImageCaptured()} · frame=${failure.frameNumber}",
                    )
                    runOnUiThread { previewButton.isEnabled = true }
                }
            }, cameraHandler)
        } catch (e: Throwable) {
            setStatusAny("STAGE 3 STANDARD request FAIL · ${e.javaClass.simpleName}: ${e.message}")
            runOnUiThread { previewButton.isEnabled = true }
        }
    }

'''
insert_at = s.index('    private fun submitForcedPhysicalMaxStill(')
s = s[:insert_at] + standard_submit + s[insert_at:]

# Finalize/audits: use actual selected geometry, never the old fixed 16320x12288 constants.
fin_start = s.index('    private fun finalizeCapture(image: Image, logicalResult: TotalCaptureResult) {')
fin_end = s.index('    private fun persistOriginalRawBuffer(', fin_start)
fin = s[fin_start:fin_end]
fin = fin.replace(
    '    private fun finalizeCapture(image: Image, logicalResult: TotalCaptureResult) {\n        try {',
    '    private fun finalizeCapture(image: Image, logicalResult: TotalCaptureResult) {\n        val profile = activeGeometryContext ?: error("active v0.39 geometry context ontbreekt")\n        try {',
    1,
)
fin = fin.replace('TARGET_W', 'profile.width')
fin = fin.replace('TARGET_H', 'profile.height')
fin = fin.replace(
    '"STAGE 3 RAW SEALED · physical 5 · 16320×12288 · 200,540,160 samples · timestamp exact.\\n"',
    '"STAGE 3 RAW SEALED · ${profile.label} · ${profile.width.toLong() * profile.height.toLong()} samples · timestamp exact.\\n"',
)
fin = fin.replace('requested/output MAX=true', 'requested/output MAX=${profile.useMaximumResolutionPixelMode}')
fin = fin.replace('_v020.dng', '_v039_${profile.id}.dng')
fin = fin.replace('_EVIDENCE_v020.json', '_${profile.id}_EVIDENCE_v039.json')
fin = fin.replace('_v020.rawpayload', '_v039_${profile.id}.rawpayload')
fin = fin.replace('_v020.png', '_v039_${profile.id}.png')

# Preserve returned mode as observation but compare it to the selected context.
mode_anchor = '''            val returnedPixelModeIsMaximum =
                returnedPixelMode == CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION

'''
mode_repl = mode_anchor + '''            val returnedPixelModeConsistentWithRequestedContext =
                if (profile.useMaximumResolutionPixelMode) returnedPixelModeIsMaximum
                else !returnedPixelModeIsMaximum

'''
if mode_anchor not in fin:
    raise SystemExit('v0.39 returned mode anchor missing')
fin = fin.replace(mode_anchor, mode_repl, 1)
fin = fin.replace(
    '.put("returnedSensorPixelModeMismatchPreserved", !returnedPixelModeIsMaximum)',
    '.put("returnedPixelModeConsistentWithRequestedContext", returnedPixelModeConsistentWithRequestedContext)\n'
    '                    .put("returnedSensorPixelModeMismatchPreserved", !returnedPixelModeConsistentWithRequestedContext)\n'
    '                    .put("geometryContextProfile", Camera2RawGeometryContextLadder.profileEvidence(profile))\n'
    '                    .put("geometryContextLadderValidation", lastGeometryLadderValidation ?: JSONObject.NULL)\n'
    '                    .put("unknownVendorKeysWritten", 0)\n'
    '                    .put("vendorInterventionUsed", false)',
    1,
)

# Stage 3.8: row-aligned context resolver downstream of Stage 3.6/3.7.
resolver_anchor = '''            capturedGeometryPayload = geometryPayloadCandidate.takeIf { it.exists() && it.length() > 0L }
            capturedGeometryPreview = geometryPreviewCandidate.takeIf { it.exists() && it.length() > 0L }

            // Enrich only the JSON sidecar. Never reopen the source file for writing.
'''
resolver_repl = '''            capturedGeometryPayload = geometryPayloadCandidate.takeIf { it.exists() && it.length() > 0L }
            capturedGeometryPreview = geometryPreviewCandidate.takeIf { it.exists() && it.length() > 0L }

            val contextResolveAttempt = if (rasterAudit != null) runCatching {
                RawGeometryContextResolver.resolve(rasterAudit, profile)
            } else null
            val contextResolve = contextResolveAttempt?.getOrNull()
            val contextResolveError = contextResolveAttempt?.exceptionOrNull()?.let {
                "${it.javaClass.simpleName}: ${it.message}"
            } ?: if (rasterAudit == null) "Stage 3.6 unavailable; Stage 3.8 not run" else null
            val contextClassification =
                contextResolve?.optString("classification", "UNCLASSIFIED")
                    ?: "CONTEXT_RESOLVER_UNAVAILABLE"

            // Enrich only the JSON sidecar. Never reopen the source file for writing.
'''
if resolver_anchor not in fin:
    raise SystemExit('v0.39 resolver anchor missing')
fin = fin.replace(resolver_anchor, resolver_repl, 1)

enrich_anchor = '''                    .put("stage37GeometrySemanticPromotionAllowed", false)
'''
enrich_repl = enrich_anchor + '''                    .put("stage38RawGeometryContextResolver", contextResolve ?: JSONObject.NULL)
                    .put("stage38RawGeometryContextResolverError", contextResolveError ?: JSONObject.NULL)
                    .put("stage38UsesStage36Only", true)
                    .put("stage38SourceModified", false)
                    .put("stage38SensorNativeGeometryClaimMade", false)
                    .put("stage38OpticalResolutionClaimMade", false)
                    .put("stage38SemanticPromotionAllowed", false)
'''
if enrich_anchor not in fin:
    raise SystemExit('v0.39 enrichment anchor missing')
fin = fin.replace(enrich_anchor, enrich_repl, 1)

status_anchor = '''                    "Stage 3.7 geometry=$geometryClassification · exactPrefix=" +
                    (capturedGeometryPayload?.length() ?: 0L) + " bytes",
'''
status_repl = '''                    "Stage 3.7 geometry=$geometryClassification · exactPrefix=" +
                    (capturedGeometryPayload?.length() ?: 0L) + " bytes\\n" +
                    "Stage 3.8 context=$contextClassification · rowExtent=" +
                    (contextResolve?.optLong("rowAlignedPopulatedExtentBytes", -1L) ?: -1L) + " bytes",
'''
if status_anchor not in fin:
    raise SystemExit('v0.39 status anchor missing')
fin = fin.replace(status_anchor, status_repl, 1)

ui_success_anchor = '''                saveGeometryPreviewButton.isEnabled = capturedGeometryPreview?.exists() == true
                previewButton.isEnabled = true
'''
ui_success_repl = '''                saveGeometryPreviewButton.isEnabled = capturedGeometryPreview?.exists() == true
                selectedGeometryContextIndex = Camera2RawGeometryContextLadder.nextIndex(profile.index)
                updateGeometryContextButton()
                previewButton.isEnabled = true
                captureButton.isEnabled = false
'''
if ui_success_anchor not in fin:
    raise SystemExit('v0.39 auto-advance anchor missing')
fin = fin.replace(ui_success_anchor, ui_success_repl, 1)

finally_anchor = '''            runCatching { camera?.close() }
            camera = null
        }
    }
'''
finally_repl = '''            runCatching { camera?.close() }
            camera = null
            activeGeometryContext = null
        }
    }
'''
if finally_anchor not in fin:
    raise SystemExit('v0.39 finalize cleanup anchor missing')
fin = fin.replace(finally_anchor, finally_repl, 1)
s = s[:fin_start] + fin + s[fin_end:]

# Persist exact buffer using selected context geometry.
per_start = s.index('    private fun persistOriginalRawBuffer(')
per_end = s.index('    private fun buildEvidence(', per_start)
per = s[per_start:per_end]
per = per.replace(
    '    private fun persistOriginalRawBuffer(image: Image, stamp: Long): RawEvidence {\n        val plane',
    '    private fun persistOriginalRawBuffer(image: Image, stamp: Long): RawEvidence {\n'
    '        val profile = activeGeometryContext ?: error("active v0.39 geometry context ontbreekt")\n'
    '        val plane',
    1,
)
per = per.replace('TARGET_SAMPLES * 2L', 'profile.nominalBytesU16')
per = per.replace('TARGET_W', 'profile.width')
per = per.replace('TARGET_H', 'profile.height')
per = per.replace('_v020.', '_v039_${profile.id}.')
s = s[:per_start] + per + s[per_end:]

# Base evidence must describe the requested context rather than fixed 200MP constants.
ev_start = s.index('    private fun buildEvidence(')
ev_end = s.index('    private fun safeSizes(', ev_start)
ev = s[ev_start:ev_end]
ev = ev.replace(
    '    ): JSONObject {\n        return JSONObject()',
    '    ): JSONObject {\n'
    '        val profile = activeGeometryContext ?: error("active v0.39 geometry context ontbreekt")\n'
    '        return JSONObject()',
    1,
)
ev = ev.replace('TARGET_W', 'profile.width')
ev = ev.replace('TARGET_H', 'profile.height')
ev = ev.replace('TARGET_SAMPLES * 2L', 'profile.nominalBytesU16')
ev = ev.replace(
    '.put("independentEvidenceCount", 1)',
    '.put("independentEvidenceCount", 1)\n'
    '            .put("experiment", "ADVERTISED_RAW_OUTPUT_CONTEXT_GEOMETRY_LADDER")\n'
    '            .put("geometryContextProfile", Camera2RawGeometryContextLadder.profileEvidence(profile))\n'
    '            .put("unknownVendorKeysWritten", 0)\n'
    '            .put("vendorInterventionUsed", false)',
    1,
)
ev = ev.replace(
    '.put("outputMaximumResolutionModeDeclared", true)',
    '.put("outputMaximumResolutionModeDeclared", profile.useMaximumResolutionPixelMode)',
)
ev = ev.replace(
    '.put("requestedMaximumResolution", true)',
    '.put("requestedMaximumResolution", profile.useMaximumResolutionPixelMode)',
)
s = s[:ev_start] + ev + s[ev_end:]

# Small UI helper.
helper_anchor = '    private fun safeSizes('
helper = '''    private fun updateGeometryContextButton() {
        if (!::geometryContextButton.isInitialized) return
        geometryContextButton.text =
            Camera2RawGeometryContextLadder.profile(selectedGeometryContextIndex).label + " · tik→volgende"
    }

'''
s = s.replace(helper_anchor, helper + helper_anchor, 1)

# v0.39 output/evidence labels.
s = s.replace('staged-evidence.v0.20', 'raw-geometry-context-evidence.v0.39')
s = s.replace('truthraw-200mp-v020', 'truthraw-geometry-v039')
s = s.replace('truthraw-v020-capability', 'truthraw-v039-capability')

# Invariants.
assert 'TruthRaw · Camera-5 RAW geometry/context ladder v0.39' in s
assert 'Camera2RawGeometryContextLadder.validateAdvertisements(' in s
assert 'submitStandardPhysicalStill' in s
assert 'if (profile.useMaximumResolutionPixelMode) submitForcedPhysicalMaxStill' in s
assert 'ImageReader.newInstance(profile.width, profile.height' in s
assert 'RawGeometryContextResolver.resolve(rasterAudit, profile)' in s
assert 'stage38RawGeometryContextResolver' in s
assert 'unknownVendorKeysWritten", 0' in s
assert 'vendorInterventionUsed", false' in s
assert s.index('persistOriginalRawBuffer(image, stamp)') < s.index('Camera2EnvelopeProbe.observe(image, physical, physicalResult)')
assert s.index('image.close()') < s.index('RawSensorRasterAudit.audit(')
assert s.index('RawSensorRasterAudit.audit(') < s.index('RawPayloadGeometryDecoder.decode(')
assert s.index('RawPayloadGeometryDecoder.decode(') < s.index('RawGeometryContextResolver.resolve(rasterAudit, profile)')
assert 'MAXIMUM_RESOLUTION vereist' not in s
assert 'Camera2PhysicalOnlyInt32CandidateSweep' not in s
assert 'Camera2PhysicalInSensorZoomByteSessionProbe' not in s

p.write_text(s)
print('patched v0.39', p)
print('bytes', p.stat().st_size)
