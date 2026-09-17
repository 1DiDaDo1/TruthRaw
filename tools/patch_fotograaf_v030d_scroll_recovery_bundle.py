#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct v0.30c first. v0.30d remains UI/provenance recovery only:
# - scrolling remains enabled;
# - completed matrix progress is recovered from already-written v0.30 evidence JSON filenames;
# - all cached v0.30 matrix evidence JSONs can be exported as one small ZIP.
# No capture, vendor, sealing, Stage 3.6 or Stage 3.7 behavior is changed.
ns = {}
exec(
    compile(
        Path('tools/patch_fotograaf_v030c_scrollable_matrix_ui.py').read_text(),
        'patch_fotograaf_v030c_scrollable_matrix_ui.py',
        'exec',
    ),
    ns,
    ns,
)
s = p.read_text()

import_needle = 'import java.util.Locale\n'
import_replacement = 'import java.util.Locale\nimport java.util.zip.ZipEntry\nimport java.util.zip.ZipOutputStream\n'
if import_needle not in s:
    raise SystemExit('v0.30d zip import anchor not found')
s = s.replace(import_needle, import_replacement, 1)

field_needle = '''    private lateinit var matrixProfileButton: Button\n    private lateinit var saveRawButton: Button\n'''
field_replacement = '''    private lateinit var matrixProfileButton: Button\n    private lateinit var exportMatrixEvidenceBundleButton: Button\n    private lateinit var saveRawButton: Button\n'''
if field_needle not in s:
    raise SystemExit('v0.30d bundle button field anchor not found')
s = s.replace(field_needle, field_replacement, 1)

state_needle = '''    private var matrixRunIndex: Int = 0\n    private var activeMatrixRunIndex: Int = 0\n\n    private var capturedRaw: File? = null\n'''
state_replacement = '''    private var matrixRunIndex: Int = 0\n    private var activeMatrixRunIndex: Int = 0\n    private var matrixEvidenceBundle: File? = null\n\n    private var capturedRaw: File? = null\n'''
if state_needle not in s:
    raise SystemExit('v0.30d bundle state anchor not found')
s = s.replace(state_needle, state_replacement, 1)

# Run recovery after the already-versioned Stage-0 status is initialized, immediately before
# the permission branch. This survives UI-version text changes from the reconstruction chain.
oncreate_needle = '''        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {\n'''
oncreate_replacement = '''        restoreMatrixProgressFromCache()\n        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {\n'''
if oncreate_needle not in s:
    raise SystemExit('v0.30d onCreate recovery anchor not found')
s = s.replace(oncreate_needle, oncreate_replacement, 1)

ui_needle = '''        matrixProfileButton = button(matrixProfileLabel(matrixRunIndex)) {\n            matrixRunIndex = (matrixRunIndex + 1) % Camera2VendorRouteFullFactorialMatrix.TOTAL_RUNS\n            matrixProfileButton.text = matrixProfileLabel(matrixRunIndex)\n            val p = Camera2VendorRouteFullFactorialMatrix.profileForRun(matrixRunIndex)\n            setStatus("Matrix handmatig geselecteerd: run ${p.runIndex + 1}/16 · ABCD=${p.bits} · ${p.id}.\\nLow=UNSET; high=numeric 1 met bewezen native type. Geen vendorsemantiek aangenomen.")\n        }\n        saveRawButton = button("Originele 200MP RAW buffer opslaan") { saveFile(capturedRaw, "application/octet-stream", REQUEST_SAVE_RAW) }.apply { isEnabled = false }\n'''
ui_replacement = '''        matrixProfileButton = button(matrixProfileLabel(matrixRunIndex)) {\n            matrixRunIndex = (matrixRunIndex + 1) % Camera2VendorRouteFullFactorialMatrix.TOTAL_RUNS\n            matrixProfileButton.text = matrixProfileLabel(matrixRunIndex)\n            val p = Camera2VendorRouteFullFactorialMatrix.profileForRun(matrixRunIndex)\n            setStatus("Matrix handmatig geselecteerd: run ${p.runIndex + 1}/16 · ABCD=${p.bits} · ${p.id}.\\nLow=UNSET; high=numeric 1 met bewezen native type. Geen vendorsemantiek aangenomen.")\n        }\n        exportMatrixEvidenceBundleButton = button("Matrix · exporteer alle cached v0.30 evidence JSONs als ZIP") {\n            exportCachedMatrixEvidenceBundle()\n        }\n        saveRawButton = button("Originele 200MP RAW buffer opslaan") { saveFile(capturedRaw, "application/octet-stream", REQUEST_SAVE_RAW) }.apply { isEnabled = false }\n'''
if ui_needle not in s:
    raise SystemExit('v0.30d bundle UI creation anchor not found')
s = s.replace(ui_needle, ui_replacement, 1)

add_needle = '''        root.addView(previewButton)\n        root.addView(matrixProfileButton)\n        root.addView(captureButton)\n'''
add_replacement = '''        root.addView(previewButton)\n        root.addView(matrixProfileButton)\n        root.addView(exportMatrixEvidenceBundleButton)\n        root.addView(captureButton)\n'''
if add_needle not in s:
    raise SystemExit('v0.30d bundle addView anchor not found')
s = s.replace(add_needle, add_replacement, 1)

method_anchor = '''    private fun matrixProfileLabel(runIndex: Int): String {\n'''
method_block = '''    private val matrixEvidenceNameRegex = Regex(".*_R(\\\\d{2})_ABCD_([01]{4})_EVIDENCE_v030\\\\.json$")\n\n    private fun cachedMatrixEvidenceFiles(): List<File> =\n        cacheDir.listFiles().orEmpty()\n            .filter { it.isFile && matrixEvidenceNameRegex.matches(it.name) }\n            .sortedBy { it.name }\n\n    private fun restoreMatrixProgressFromCache() {\n        val files = cachedMatrixEvidenceFiles()\n        if (files.isEmpty()) return\n        val completed = files.mapNotNull { file ->\n            matrixEvidenceNameRegex.matchEntire(file.name)?.groupValues?.getOrNull(1)?.toIntOrNull()\n        }.distinct().sorted()\n        if (completed.isEmpty()) return\n        val highestCompletedRun = completed.maxOrNull() ?: return\n        matrixRunIndex = if (highestCompletedRun >= Camera2VendorRouteFullFactorialMatrix.TOTAL_RUNS) {\n            0\n        } else {\n            highestCompletedRun\n        }\n        activeMatrixRunIndex = matrixRunIndex\n        if (::matrixProfileButton.isInitialized) matrixProfileButton.text = matrixProfileLabel(matrixRunIndex)\n        val next = Camera2VendorRouteFullFactorialMatrix.profileForRun(matrixRunIndex)\n        setStatus(\n            "v0.30d cache recovery: ${files.size} matrix evidence JSON(s) gevonden; voltooide runs=${completed.joinToString()}.\\n" +\n                "Volgende geselecteerde run=${next.runIndex + 1}/16 · ABCD=${next.bits} · ${next.id}.\\n" +\n                "Gebruik de ZIP-knop om ook eerder vastgelegde JSONs veilig uit app-cache te exporteren.",\n        )\n    }\n\n    private fun exportCachedMatrixEvidenceBundle() {\n        val files = cachedMatrixEvidenceFiles()\n        if (files.isEmpty()) {\n            setStatus("Geen cached v0.30 matrix evidence JSONs gevonden; niets geëxporteerd.")\n            return\n        }\n        runCatching {\n            val bundle = File(cacheDir, "TRUTHRAW_CAM5_V030_MATRIX_EVIDENCE_BUNDLE_${System.currentTimeMillis()}.zip")\n            val manifestFiles = JSONArray()\n            ZipOutputStream(FileOutputStream(bundle)).use { zip ->\n                for (file in files) {\n                    zip.putNextEntry(ZipEntry(file.name))\n                    FileInputStream(file).use { input -> input.copyTo(zip) }\n                    zip.closeEntry()\n                    manifestFiles.put(\n                        JSONObject()\n                            .put("file", file.name)\n                            .put("bytes", file.length())\n                            .put("sha256", sha256File(file)),\n                    )\n                }\n                val manifest = JSONObject()\n                    .put("schema", "truthraw.camera5-v030-matrix-evidence-bundle.v0.30d")\n                    .put("createdAtUtc", Instant.now().toString())\n                    .put("evidenceJsonCount", files.size)\n                    .put("containsRawSourceBytes", false)\n                    .put("containsDerivedRawPayloadBytes", false)\n                    .put("files", manifestFiles)\n                    .put("semanticPromotionAllowed", false)\n                zip.putNextEntry(ZipEntry("TRUTHRAW_V030_MATRIX_BUNDLE_MANIFEST.json"))\n                zip.write(manifest.toString(2).toByteArray(Charsets.UTF_8))\n                zip.closeEntry()\n            }\n            matrixEvidenceBundle = bundle\n            setStatus("Matrix evidence bundle klaar: ${files.size} JSON(s) · ${bundle.length()} bytes. Kies nu opslaglocatie.")\n            saveFile(bundle, "application/zip", REQUEST_SAVE_MATRIX_EVIDENCE_BUNDLE)\n        }.onFailure { e ->\n            setStatus("Matrix evidence bundle maken faalde: ${e.javaClass.simpleName}: ${e.message}")\n        }\n    }\n\n    private fun matrixProfileLabel(runIndex: Int): String {\n'''
if method_anchor not in s:
    raise SystemExit('v0.30d helper method anchor not found')
s = s.replace(method_anchor, method_block, 1)

result_needle = '''            REQUEST_SAVE_GEOMETRY_PAYLOAD -> capturedGeometryPayload\n            REQUEST_SAVE_GEOMETRY_PREVIEW -> capturedGeometryPreview\n            else -> null\n'''
result_replacement = '''            REQUEST_SAVE_GEOMETRY_PAYLOAD -> capturedGeometryPayload\n            REQUEST_SAVE_GEOMETRY_PREVIEW -> capturedGeometryPreview\n            REQUEST_SAVE_MATRIX_EVIDENCE_BUNDLE -> matrixEvidenceBundle\n            else -> null\n'''
if result_needle not in s:
    raise SystemExit('v0.30d activity result anchor not found')
s = s.replace(result_needle, result_replacement, 1)

const_needle = '''        private const val REQUEST_SAVE_GEOMETRY_PAYLOAD = 5804\n        private const val REQUEST_SAVE_GEOMETRY_PREVIEW = 5805\n'''
const_replacement = '''        private const val REQUEST_SAVE_GEOMETRY_PAYLOAD = 5804\n        private const val REQUEST_SAVE_GEOMETRY_PREVIEW = 5805\n        private const val REQUEST_SAVE_MATRIX_EVIDENCE_BUNDLE = 5806\n'''
if const_needle not in s:
    raise SystemExit('v0.30d request constant anchor not found')
s = s.replace(const_needle, const_replacement, 1)

# Recovery/export must not alter scientific acquisition ordering.
assert 'return ScrollView(this).apply {' in s
assert 'restoreMatrixProgressFromCache()' in s
assert 'exportCachedMatrixEvidenceBundle()' in s
assert 'truthraw.camera5-v030-matrix-evidence-bundle.v0.30d' in s
assert 'Camera2VendorRouteFullFactorialMatrix.applyProfile(' in s
assert s.count('Camera2VendorRouteFullFactorialMatrix.applyProfile(') == 1
assert s.index('Camera2PreHalGate.observeSession(') < s.index('Camera2VendorRouteFullFactorialMatrix.applyProfile(')
assert s.index('Camera2VendorRouteFullFactorialMatrix.applyProfile(') < s.index('device.isSessionConfigurationSupported(config)')
assert s.index('persistOriginalRawBuffer(image, stamp)') < s.index('Camera2EnvelopeProbe.observe(image, physical, physicalResult)')
assert s.index('RawSensorRasterAudit.audit(') < s.index('RawPayloadGeometryDecoder.decode(')

p.write_text(s)
print('patched v0.30d scroll + cache recovery + evidence bundle UI', p)
print('bytes', p.stat().st_size)
