#!/usr/bin/env python3
from pathlib import Path

P = Path("suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt")
s = P.read_text(encoding="utf-8")

repls = {
    'import android.content.Intent': 'import android.content.Intent\nimport android.content.ContentValues',
    'import android.os.Bundle': 'import android.os.Bundle\nimport android.os.Environment',
    'import android.media.ImageReader': 'import android.media.ImageReader\nimport android.provider.MediaStore',
    'private val cameraThread = HandlerThread("truthraw-200mp-v053").apply { start() }':
    'private val cameraThread = HandlerThread("truthraw-cam5-8160-v056").apply { start() }',
    'private const val TARGET_W = 16320': 'private const val TARGET_W = 8160',
    'private const val TARGET_H = 12288': 'private const val TARGET_H = 6144',
    'private const val TARGET_SAMPLES = 200_540_160L': 'private const val TARGET_SAMPLES = 50_135_040L',
    'button("Stap 3 · PHYSICAL-SCOPED CAPTURE · 16320×12288")':
    'button("Stap 3 · PHYSICAL-SCOPED TEST · 8160×6144")',
    '"Donkere preview is geen blokkade. Stage 3 PASS vereist 16320×12288 RAW_SENSOR + physical Camera-5 result + timestampidentiteit. Returned SENSOR_PIXEL_MODE wordt pas ná sealing geïnterpreteerd."':
    '"Donkere preview is geen blokkade. v0.56 TEST vereist 8160×6144 RAW_SENSOR + physical Camera-5 result + timestampidentiteit. RAW wordt vóór interpretatie verzegeld; geen native-ADC-promotie."',
    '"schema", "truthraw.fotograaf-camera5-200mp-v014-route-replay.v0.53"':
    '"schema", "truthraw.fotograaf-camera5-8160x6144-raw-probe.v0.56"',
    '"TRUTHRAW_${stamp}_CAM5_200MP_${TARGET_W}x${TARGET_H}_v011.${if (contiguous) "rawsensor" else "rawbuffer"}"':
    '"TRUTHRAW_${stamp}_CAM5_8160_PROBE_SOURCE_${TARGET_W}x${TARGET_H}_v056.${if (contiguous) "rawsensor" else "rawbuffer"}"',
    'val report = File(cacheDir, "TRUTHRAW_${stamp}_CAM5_200MP_EVIDENCE_v053.json")':
    'val report = File(cacheDir, "TRUTHRAW_${stamp}_CAM5_8160_PROBE_EVIDENCE_v056.json")',
}

for old,new in repls.items():
    if old not in s:
        raise SystemExit(f"required anchor missing: {old}")
    s=s.replace(old,new,1)

anchor = '''            capturedDng = dng
            capturedJson = report

            val admitted = admittedProcessingDng
'''
replacement = '''            capturedDng = dng
            capturedJson = report

            // v0.56 research convenience only: copy the already-sealed source evidence,
            // completed evidence JSON and diagnostic preview to DCIM/TRUTHRAW automatically.
            // These are exports of existing evidence; this does not alter authority or admission.
            val diagnosticPreview = File(cacheDir, "TRUTHRAW_${stamp}_CAM5_ADMITTED_diagnostic.png")
            val autoExports = listOf(
                rawEvidence.file to "application/octet-stream",
                report to "application/json",
                diagnosticPreview to "image/png",
            ).filter { it.first.isFile }.map { (file, mime) ->
                runCatching { exportResearchFileToDcimTruthRaw(file, mime) }
                    .fold(
                        onSuccess = { "${file.name}=OK" },
                        onFailure = { "${file.name}=FAIL:${it.javaClass.simpleName}:${it.message}" },
                    )
            }
            evidence.put("v056AutomaticResearchExports", JSONArray(autoExports))
            // Rewrite report once so it also records the export attempt outcomes.
            report.writeText(evidence.toString(2))

            val admitted = admittedProcessingDng
'''
if anchor not in s:
    raise SystemExit("auto-export insertion anchor missing")
s=s.replace(anchor,replacement,1)

helper_anchor = '''    private fun saveFile(file: File?, mime: String, requestCode: Int) {
'''
helper = '''    private fun exportResearchFileToDcimTruthRaw(file: File, mime: String) {
        val values = ContentValues().apply {
            put(MediaStore.MediaColumns.DISPLAY_NAME, file.name)
            put(MediaStore.MediaColumns.MIME_TYPE, mime)
            put(MediaStore.MediaColumns.RELATIVE_PATH, Environment.DIRECTORY_DCIM + "/TRUTHRAW")
            put(MediaStore.MediaColumns.IS_PENDING, 1)
        }
        val collection = MediaStore.Files.getContentUri(MediaStore.VOLUME_EXTERNAL_PRIMARY)
        val uri = contentResolver.insert(collection, values)
            ?: error("MediaStore insert returned null")
        try {
            contentResolver.openOutputStream(uri, "w")?.use { out ->
                FileInputStream(file).use { input -> input.copyTo(out, 1024 * 1024) }
            } ?: error("MediaStore output stream unavailable")
            val publish = ContentValues().apply { put(MediaStore.MediaColumns.IS_PENDING, 0) }
            contentResolver.update(uri, publish, null, null)
        } catch (t: Throwable) {
            runCatching { contentResolver.delete(uri, null, null) }
            throw t
        }
    }

    private fun saveFile(file: File?, mime: String, requestCode: Int) {
'''
if helper_anchor not in s:
    raise SystemExit("helper insertion anchor missing")
s=s.replace(helper_anchor,helper,1)

# Do not alter source-first sealing, physical Camera-5 binding, timestamp identity,
# topology audit/admission, or scientific authority gates.
P.write_text(s,encoding="utf-8")
print("patched", P)
