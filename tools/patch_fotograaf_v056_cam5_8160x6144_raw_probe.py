#!/usr/bin/env python3
from pathlib import Path

P = Path("suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt")
s = P.read_text(encoding="utf-8")

repls = {
    'import android.content.Intent': 'import android.content.Intent\nimport android.content.ContentValues',
    'import android.os.Bundle': 'import android.os.Bundle\nimport android.os.Environment',
    'import android.media.ImageReader': 'import android.media.ImageReader\nimport android.provider.MediaStore',
    'private val cameraThread = HandlerThread("truthraw-200mp-v053").apply { start() }':
    'private val cameraThread = HandlerThread("truthraw-cam5-8160-v057").apply { start() }',
    'private const val TARGET_W = 16320': 'private const val TARGET_W = 8160',
    'private const val TARGET_H = 12288': 'private const val TARGET_H = 6144',
    'private const val TARGET_SAMPLES = 200_540_160L': 'private const val TARGET_SAMPLES = 50_135_040L',
    'button("Stap 3 · PHYSICAL-SCOPED CAPTURE · 16320×12288")':
    'button("Stap 3 · PHYSICAL-SCOPED TEST · 8160×6144")',
    '"Donkere preview is geen blokkade. Stage 3 PASS vereist 16320×12288 RAW_SENSOR + physical Camera-5 result + timestampidentiteit. Returned SENSOR_PIXEL_MODE wordt pas ná sealing geïnterpreteerd."':
    '"Donkere preview is geen blokkade. v0.57 READ-ONLY RESULT INVENTORY · 8160×6144 RAW_SENSOR + physical Camera-5 result + timestampidentiteit. RAW wordt vóór interpretatie verzegeld; geen native-ADC-promotie."',
    '"schema", "truthraw.fotograaf-camera5-200mp-v014-route-replay.v0.53"':
    '"schema", "truthraw.fotograaf-camera5-8160x6144-runtime-result-inventory.v0.57"',
    '"TRUTHRAW_${stamp}_CAM5_200MP_${TARGET_W}x${TARGET_H}_v011.${if (contiguous) "rawsensor" else "rawbuffer"}"':
    '"TRUTHRAW_${stamp}_CAM5_8160_PROBE_SOURCE_${TARGET_W}x${TARGET_H}_v057.${if (contiguous) "rawsensor" else "rawbuffer"}"',
    'val report = File(cacheDir, "TRUTHRAW_${stamp}_CAM5_200MP_EVIDENCE_v053.json")':
    'val report = File(cacheDir, "TRUTHRAW_${stamp}_CAM5_8160_RESULT_INVENTORY_EVIDENCE_v057.json")',
}

for old,new in repls.items():
    if old not in s:
        raise SystemExit(f"required anchor missing: {old}")
    s=s.replace(old,new,1)

source_export_anchor = '''            val rawEvidence = persistOriginalRawBuffer(image, stamp)
            capturedRaw = rawEvidence.file

            val returnedPixelMode'''
source_export_replacement = '''            val rawEvidence = persistOriginalRawBuffer(image, stamp)
            capturedRaw = rawEvidence.file

            // v0.56c: publish the already-sealed source BEFORE DNG creation or topology admission.
            // Downloads/TRUTHRAW is intentionally used for arbitrary research files on scoped storage.
            val sourceExport = runCatching {
                exportResearchFileToDownloadsTruthRaw(rawEvidence.file, "application/octet-stream")
            }.fold(
                onSuccess = { "SOURCE EXPORTED ✓ · $it" },
                onFailure = { "SOURCE EXPORT FAIL · ${it.javaClass.simpleName}: ${it.message}" },
            )
            setStatusAny(sourceExport)

            val returnedPixelMode'''
if source_export_anchor not in s:
    raise SystemExit("source export insertion anchor missing")
s=s.replace(source_export_anchor, source_export_replacement, 1)

evidence_anchor = '''            val evidence = buildEvidence(logicalResult, physicalResult, image, rawEvidence, dng, dngSha, dngError)
            image.close()'''
evidence_replacement = '''            val evidence = buildEvidence(logicalResult, physicalResult, image, rawEvidence, dng, dngSha, dngError)
            evidence.put("v056cSourceExport", sourceExport)\n            evidence.put("v057RuntimeResultInventory", buildRuntimeResultInventory(logicalResult, physicalResult))
            image.close()'''
if evidence_anchor not in s:
    raise SystemExit("source export evidence anchor missing")
s=s.replace(evidence_anchor, evidence_replacement, 1)
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
            val evidenceExportStatus = runCatching {
                val location = exportResearchFileToDownloadsTruthRaw(report, "application/json")
                "EVIDENCE EXPORTED ✓ · $location"
            }.getOrElse { "EVIDENCE EXPORT FAIL · ${it.javaClass.simpleName}: ${it.message}" }
            setStatusAny(evidenceExportStatus)

            val admitted = admittedProcessingDng
'''
if anchor not in s:
    raise SystemExit("auto-export insertion anchor missing")
s=s.replace(anchor,replacement,1)

helper_anchor = '''    private fun saveFile(file: File?, mime: String, requestCode: Int) {
'''
helper = '''    private fun buildRuntimeResultInventory(
        logicalResult: TotalCaptureResult,
        physicalResult: CaptureResult,
    ): JSONObject {
        val physical = physical5Characteristics
        val advertisedResultKeys = physical?.availableCaptureResultKeys.orEmpty().map { it.name }.sorted()
        val advertisedRequestKeys = physical?.availableCaptureRequestKeys.orEmpty().map { it.name }.sorted()
        return JSONObject()
            .put("schema", "truthraw.camera5-runtime-result-inventory.v0.57")
            .put("authority", "READ_ONLY_CAPTURE_RESULT_METADATA_OBSERVATION")
            .put("vendorRequestWrittenByV057", false)
            .put("captureTopologyModifiedByV057", false)
            .put("imagePayloadModifiedByV057", false)
            .put("physicalCameraId", PHYSICAL_ID)
            .put("advertisedCaptureResultKeyCount", advertisedResultKeys.size)
            .put("advertisedCaptureRequestKeyCount", advertisedRequestKeys.size)
            .put("advertisedNonAndroidResultKeys", JSONArray(advertisedResultKeys.filter { !it.startsWith("android.") }))
            .put("advertisedNonAndroidRequestKeys", JSONArray(advertisedRequestKeys.filter { !it.startsWith("android.") }))
            .put("logicalResult", resultKeyInventory(logicalResult))
            .put("physical5Result", resultKeyInventory(physicalResult))
            .put("directTypedHintUserValue", JSONObject()
                .put("name", "com.hihonor.capture.metadata.hintUserValue")
                .put("staticApkType", "java.lang.Integer / Int")
                .put("logicalRead", directReadIntResultKey(logicalResult, "com.hihonor.capture.metadata.hintUserValue"))
                .put("physical5Read", directReadIntResultKey(physicalResult, "com.hihonor.capture.metadata.hintUserValue")))
            .put("suffixTargets", JSONArray(listOf(
                "hintUserValue",
                "qcomRemosaicEnable",
                "quadraRemosaicHdMode",
                "quadrawCaptureStatus",
                "captureFormat",
                "captureStreamResolution",
                "SMART_SCENE_MODE",
            )))
            .put("boundary", "RESULT_METADATA_ONLY__NO_VENDOR_WRITE_NO_OEM_PRIVILEGE_NO_SENSOR_OR_ADC_PROMOTION")
    }

    private fun resultKeyInventory(result: CaptureResult): JSONObject {
        val all = result.keys.sortedBy { it.name }
        val vendor = all.filter { !it.name.startsWith("android.") }
        val tokens = listOf(
            "honor", "hihonor", "qti", "qcom", "codeaurora", "remosaic", "quad",
            "ultra", "raw", "scene", "capture", "stream", "pixel", "sensor",
        )
        val interesting = all.filter { key ->
            val n = key.name.lowercase(Locale.ROOT)
            tokens.any { token -> n.contains(token) }
        }
        return JSONObject()
            .put("cameraId", result.cameraId)
            .put("allKeyCount", all.size)
            .put("vendorKeyCount", vendor.size)
            .put("vendorKeys", JSONArray(vendor.map { readResultKey(result, it) }))
            .put("interestingKeys", JSONArray(interesting.map { readResultKey(result, it) }))
    }

    @Suppress("UNCHECKED_CAST")
    private fun readResultKey(result: CaptureResult, key: CaptureResult.Key<*>): JSONObject {
        val out = JSONObject().put("name", key.name)
        runCatching {
            result.get(key as CaptureResult.Key<Any>)
        }.onSuccess { value ->
            out.put("readCompleted", true)
            out.put("valueIsNull", value == null)
            out.put("runtimeClass", value?.javaClass?.name ?: JSONObject.NULL)
            out.put("value", jsonMetadataValue(value))
        }.onFailure { e ->
            out.put("readCompleted", false)
            out.put("errorClass", e.javaClass.name)
            out.put("errorMessage", e.message ?: JSONObject.NULL)
        }
        return out
    }

    private fun directReadIntResultKey(result: CaptureResult, name: String): JSONObject {
        val out = JSONObject()
        return runCatching {
            val key = CaptureResult.Key(name, Integer::class.java)
            val value = result.get(key)
            out.put("keyConstructed", true)
            out.put("readCompleted", true)
            out.put("value", value ?: JSONObject.NULL)
        }.getOrElse { e ->
            out.put("keyConstructed", true)
            out.put("readCompleted", false)
            out.put("errorClass", e.javaClass.name)
            out.put("errorMessage", e.message ?: JSONObject.NULL)
        }
    }

    private fun jsonMetadataValue(value: Any?): Any = when (value) {
        null -> JSONObject.NULL
        is IntArray -> JSONArray(value.toList())
        is LongArray -> JSONArray(value.toList())
        is FloatArray -> JSONArray(value.map { it.toDouble() })
        is DoubleArray -> JSONArray(value.toList())
        is ByteArray -> JSONArray(value.map { it.toInt() and 0xff })
        is ShortArray -> JSONArray(value.map { it.toInt() })
        is BooleanArray -> JSONArray(value.toList())
        is Array<*> -> JSONArray(value.map { jsonMetadataValue(it) })
        is Collection<*> -> JSONArray(value.map { jsonMetadataValue(it) })
        is Byte -> value.toInt() and 0xff
        is Number, is Boolean, is String -> value
        else -> value.toString()
    }

    private fun exportResearchFileToDownloadsTruthRaw(file: File, mime: String): String {
        val values = ContentValues().apply {
            put(MediaStore.MediaColumns.DISPLAY_NAME, file.name)
            put(MediaStore.MediaColumns.MIME_TYPE, mime)
            put(MediaStore.MediaColumns.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS + "/TRUTHRAW")
            put(MediaStore.MediaColumns.IS_PENDING, 1)
        }
        val collection = MediaStore.Downloads.getContentUri(MediaStore.VOLUME_EXTERNAL_PRIMARY)
        val uri = contentResolver.insert(collection, values)
            ?: error("Downloads MediaStore insert returned null")
        try {
            contentResolver.openOutputStream(uri, "w")?.use { out ->
                FileInputStream(file).use { input -> input.copyTo(out, 1024 * 1024) }
            } ?: error("Downloads MediaStore output stream unavailable")
            val publish = ContentValues().apply { put(MediaStore.MediaColumns.IS_PENDING, 0) }
            contentResolver.update(uri, publish, null, null)
        } catch (t: Throwable) {
            runCatching { contentResolver.delete(uri, null, null) }
            throw t
        }
        return "Downloads/TRUTHRAW/${file.name} · $uri"
    }

    private fun exportResearchFileToDcimTruthRaw(file: File, mime: String) {
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
