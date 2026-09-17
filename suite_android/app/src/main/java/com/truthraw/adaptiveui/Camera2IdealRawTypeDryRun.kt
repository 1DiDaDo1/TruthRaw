package com.truthraw.adaptiveui

import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CaptureRequest
import org.json.JSONArray
import org.json.JSONObject

/**
 * v0.22 observation-only app-side type/marshalling probe for the single vendor key
 * `org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW`.
 *
 * This helper deliberately does NOT attach session parameters and does NOT submit a request.
 * Every candidate is tested only in a disposable CaptureRequest.Builder created in app space.
 * The Android public CaptureRequest.Key(String, Class<T>) constructor exists specifically for
 * testing/custom fields (API 29+), so this lets us ask Camera2 which candidate representations can
 * survive local set/get/build round-trips without guessing a HAL/session write.
 *
 * A passing candidate is only an app-side Camera2 marshalling candidate. It is NOT proof of vendor
 * semantics and it is NOT yet permission to claim that EnableIdealRAW changes RAW routing.
 */
object Camera2IdealRawTypeDryRun {
    const val KEY_NAME = "org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW"

    data class ProbeResult(
        val evidence: JSONObject,
        val passingKinds: List<String>,
    )

    fun probe(
        device: CameraDevice,
        logical: CameraCharacteristics,
        physical: CameraCharacteristics,
    ): ProbeResult {
        val logicalSession = logical.availableSessionKeys.orEmpty()
        val physicalSession = physical.availableSessionKeys.orEmpty()
        val logicalRequest = logical.availableCaptureRequestKeys.orEmpty()
        val physicalRequest = physical.availableCaptureRequestKeys.orEmpty()
        val advertisedLogicalSessionKey = logicalSession.firstOrNull { it.name == KEY_NAME }

        val out = JSONObject()
            .put("schema", "truthraw.camera2-idealraw-type-dryrun.v0.1")
            .put("keyName", KEY_NAME)
            .put("observationOnly", true)
            .put("pixelAccess", false)
            .put("sourceMutation", false)
            .put("sessionParametersAttached", false)
            .put("requestSubmitted", false)
            .put("captureSubmitted", false)
            .put("halSubmission", false)
            .put("semanticMeaningAssumed", false)
            .put("logicalSessionAdvertised", advertisedLogicalSessionKey != null)
            .put("physicalSessionAdvertised", physicalSession.any { it.name == KEY_NAME })
            .put("logicalRequestAdvertised", logicalRequest.any { it.name == KEY_NAME })
            .put("physicalRequestAdvertised", physicalRequest.any { it.name == KEY_NAME })

        if (advertisedLogicalSessionKey == null) {
            out.put("classification", "BLOCKED_IDEALRAW_NOT_ADVERTISED_AS_LOGICAL_SESSION_KEY")
                .put("candidateResults", JSONArray())
                .put("passingKinds", JSONArray())
            return ProbeResult(out, emptyList())
        }

        val results = JSONArray()
        val passing = mutableListOf<String>()

        fun record(kind: String, body: () -> JSONObject) {
            val r = runCatching { body() }.getOrElse { e ->
                JSONObject()
                    .put("kind", kind)
                    .put("constructPass", false)
                    .put("setPass", false)
                    .put("builderReadbackPass", false)
                    .put("buildPass", false)
                    .put("requestReadbackPass", false)
                    .put("error", "${e.javaClass.simpleName}: ${e.message}")
            }
            results.put(r)
            if (r.optBoolean("requestReadbackPass", false)) passing += kind
        }

        record("Byte") { probeByte(device) }
        record("byte[]") { probeByteArray(device) }
        record("Int") { probeInt(device) }
        record("int[]") { probeIntArray(device) }
        record("Long") { probeLong(device) }
        record("long[]") { probeLongArray(device) }
        record("Float") { probeFloat(device) }
        record("float[]") { probeFloatArray(device) }
        record("Double") { probeDouble(device) }
        record("double[]") { probeDoubleArray(device) }

        out.put("candidateResults", results)
            .put("passingKinds", JSONArray(passing))
            .put("passingCount", passing.size)
            .put("uniquePassingKind", if (passing.size == 1) passing.single() else JSONObject.NULL)
            .put(
                "classification",
                when {
                    passing.isEmpty() -> "NO_APP_SIDE_MARSHALLING_CANDIDATE_PASSED"
                    passing.size == 1 -> "UNIQUE_APP_SIDE_MARSHALLING_CANDIDATE__NO_HAL_SUBMISSION"
                    else -> "MULTIPLE_APP_SIDE_MARSHALLING_CANDIDATES__AMBIGUOUS_NO_HAL_SUBMISSION"
                },
            )
            .put("authority", "APP_SIDE_CAMERA2_MARSHALLING_DIAGNOSTIC_ONLY")

        return ProbeResult(out, passing)
    }

    private fun fresh(device: CameraDevice): CaptureRequest.Builder =
        device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE)

    private fun base(kind: String) = JSONObject()
        .put("kind", kind)
        .put("constructPass", true)
        .put("setPass", false)
        .put("builderReadbackPass", false)
        .put("buildPass", false)
        .put("requestReadbackPass", false)

    private fun probeByte(device: CameraDevice): JSONObject {
        val r = base("Byte")
        val key = CaptureRequest.Key(KEY_NAME, Byte::class.javaObjectType)
        val b = fresh(device)
        b.set(key, 1.toByte()); r.put("setPass", true)
        val br = b.get(key); r.put("builderReadback", br?.toInt() ?: JSONObject.NULL)
        r.put("builderReadbackPass", br?.toInt() == 1)
        val req = b.build(); r.put("buildPass", true)
        val rr = req.get(key); r.put("requestReadback", rr?.toInt() ?: JSONObject.NULL)
        r.put("requestReadbackPass", rr?.toInt() == 1)
        return r
    }

    private fun probeByteArray(device: CameraDevice): JSONObject {
        val r = base("byte[]")
        val key = CaptureRequest.Key(KEY_NAME, ByteArray::class.java)
        val b = fresh(device)
        b.set(key, byteArrayOf(1)); r.put("setPass", true)
        val br = b.get(key); r.put("builderReadback", encodeByteArray(br))
        r.put("builderReadbackPass", br?.contentEquals(byteArrayOf(1)) == true)
        val req = b.build(); r.put("buildPass", true)
        val rr = req.get(key); r.put("requestReadback", encodeByteArray(rr))
        r.put("requestReadbackPass", rr?.contentEquals(byteArrayOf(1)) == true)
        return r
    }

    private fun probeInt(device: CameraDevice): JSONObject {
        val r = base("Int")
        val key = CaptureRequest.Key(KEY_NAME, Int::class.javaObjectType)
        val b = fresh(device)
        b.set(key, 1); r.put("setPass", true)
        val br = b.get(key); r.put("builderReadback", br ?: JSONObject.NULL)
        r.put("builderReadbackPass", br == 1)
        val req = b.build(); r.put("buildPass", true)
        val rr = req.get(key); r.put("requestReadback", rr ?: JSONObject.NULL)
        r.put("requestReadbackPass", rr == 1)
        return r
    }

    private fun probeIntArray(device: CameraDevice): JSONObject {
        val r = base("int[]")
        val key = CaptureRequest.Key(KEY_NAME, IntArray::class.java)
        val b = fresh(device)
        b.set(key, intArrayOf(1)); r.put("setPass", true)
        val br = b.get(key); r.put("builderReadback", encodeIntArray(br))
        r.put("builderReadbackPass", br?.contentEquals(intArrayOf(1)) == true)
        val req = b.build(); r.put("buildPass", true)
        val rr = req.get(key); r.put("requestReadback", encodeIntArray(rr))
        r.put("requestReadbackPass", rr?.contentEquals(intArrayOf(1)) == true)
        return r
    }

    private fun probeLong(device: CameraDevice): JSONObject {
        val r = base("Long")
        val key = CaptureRequest.Key(KEY_NAME, Long::class.javaObjectType)
        val b = fresh(device)
        b.set(key, 1L); r.put("setPass", true)
        val br = b.get(key); r.put("builderReadback", br ?: JSONObject.NULL)
        r.put("builderReadbackPass", br == 1L)
        val req = b.build(); r.put("buildPass", true)
        val rr = req.get(key); r.put("requestReadback", rr ?: JSONObject.NULL)
        r.put("requestReadbackPass", rr == 1L)
        return r
    }

    private fun probeLongArray(device: CameraDevice): JSONObject {
        val r = base("long[]")
        val key = CaptureRequest.Key(KEY_NAME, LongArray::class.java)
        val b = fresh(device)
        b.set(key, longArrayOf(1L)); r.put("setPass", true)
        val br = b.get(key); r.put("builderReadback", br?.joinToString(prefix = "[", postfix = "]") ?: JSONObject.NULL)
        r.put("builderReadbackPass", br?.contentEquals(longArrayOf(1L)) == true)
        val req = b.build(); r.put("buildPass", true)
        val rr = req.get(key); r.put("requestReadback", rr?.joinToString(prefix = "[", postfix = "]") ?: JSONObject.NULL)
        r.put("requestReadbackPass", rr?.contentEquals(longArrayOf(1L)) == true)
        return r
    }

    private fun probeFloat(device: CameraDevice): JSONObject {
        val r = base("Float")
        val key = CaptureRequest.Key(KEY_NAME, Float::class.javaObjectType)
        val b = fresh(device)
        b.set(key, 1f); r.put("setPass", true)
        val br = b.get(key); r.put("builderReadback", br ?: JSONObject.NULL)
        r.put("builderReadbackPass", br == 1f)
        val req = b.build(); r.put("buildPass", true)
        val rr = req.get(key); r.put("requestReadback", rr ?: JSONObject.NULL)
        r.put("requestReadbackPass", rr == 1f)
        return r
    }

    private fun probeFloatArray(device: CameraDevice): JSONObject {
        val r = base("float[]")
        val key = CaptureRequest.Key(KEY_NAME, FloatArray::class.java)
        val b = fresh(device)
        b.set(key, floatArrayOf(1f)); r.put("setPass", true)
        val br = b.get(key); r.put("builderReadback", br?.joinToString(prefix = "[", postfix = "]") ?: JSONObject.NULL)
        r.put("builderReadbackPass", br?.contentEquals(floatArrayOf(1f)) == true)
        val req = b.build(); r.put("buildPass", true)
        val rr = req.get(key); r.put("requestReadback", rr?.joinToString(prefix = "[", postfix = "]") ?: JSONObject.NULL)
        r.put("requestReadbackPass", rr?.contentEquals(floatArrayOf(1f)) == true)
        return r
    }

    private fun probeDouble(device: CameraDevice): JSONObject {
        val r = base("Double")
        val key = CaptureRequest.Key(KEY_NAME, Double::class.javaObjectType)
        val b = fresh(device)
        b.set(key, 1.0); r.put("setPass", true)
        val br = b.get(key); r.put("builderReadback", br ?: JSONObject.NULL)
        r.put("builderReadbackPass", br == 1.0)
        val req = b.build(); r.put("buildPass", true)
        val rr = req.get(key); r.put("requestReadback", rr ?: JSONObject.NULL)
        r.put("requestReadbackPass", rr == 1.0)
        return r
    }

    private fun probeDoubleArray(device: CameraDevice): JSONObject {
        val r = base("double[]")
        val key = CaptureRequest.Key(KEY_NAME, DoubleArray::class.java)
        val b = fresh(device)
        b.set(key, doubleArrayOf(1.0)); r.put("setPass", true)
        val br = b.get(key); r.put("builderReadback", br?.joinToString(prefix = "[", postfix = "]") ?: JSONObject.NULL)
        r.put("builderReadbackPass", br?.contentEquals(doubleArrayOf(1.0)) == true)
        val req = b.build(); r.put("buildPass", true)
        val rr = req.get(key); r.put("requestReadback", rr?.joinToString(prefix = "[", postfix = "]") ?: JSONObject.NULL)
        r.put("requestReadbackPass", rr?.contentEquals(doubleArrayOf(1.0)) == true)
        return r
    }

    private fun encodeByteArray(v: ByteArray?): Any = if (v == null) JSONObject.NULL else JSONArray(v.map { it.toInt() and 0xff })
    private fun encodeIntArray(v: IntArray?): Any = if (v == null) JSONObject.NULL else JSONArray(v.toList())
}
