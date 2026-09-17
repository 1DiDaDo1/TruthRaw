package com.truthraw.adaptiveui

import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.params.SessionConfiguration
import org.json.JSONArray
import org.json.JSONObject

/**
 * v0.21 single-variable route experiment.
 *
 * This helper changes exactly one named vendor control surface:
 * `org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW`.
 *
 * Safety / authority rules:
 * - the key must be advertised as a logical-camera session key;
 * - its runtime Java type must be discoverable;
 * - only an exact one-byte scalar/array representation is accepted;
 * - no other vendor key is touched;
 * - the setting is attached before SessionConfiguration is submitted;
 * - the helper never reads/maps/writes RAW pixels;
 * - a successful setter is an intervention fact only, not proof of vendor semantics.
 */
object Camera2IdealRawSessionProbe {
    const val KEY_NAME = "org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW"

    data class ApplyResult(
        val applied: Boolean,
        val evidence: JSONObject,
    )

    fun applyExperiment(
        device: CameraDevice,
        logical: CameraCharacteristics,
        physical: CameraCharacteristics,
        config: SessionConfiguration,
    ): ApplyResult {
        val logicalSession = logical.availableSessionKeys.orEmpty()
        val physicalSession = physical.availableSessionKeys.orEmpty()
        val logicalRequest = logical.availableCaptureRequestKeys.orEmpty()
        val physicalRequest = physical.availableCaptureRequestKeys.orEmpty()

        val key = logicalSession.firstOrNull { it.name == KEY_NAME }
        val out = JSONObject()
            .put("schema", "truthraw.camera2-idealraw-single-variable.v0.1")
            .put("experiment", "ENABLE_IDEAL_RAW_SESSION_PARAMETER_ONE")
            .put("keyName", KEY_NAME)
            .put("singleUnknownVendorVariable", true)
            .put("pixelAccess", false)
            .put("sourceMutation", false)
            .put("semanticMeaningAssumed", false)
            .put("controlReference", "TruthRaw v0.20 unchanged")
            .put("logicalSessionAdvertised", key != null)
            .put("physicalSessionAdvertised", physicalSession.any { it.name == KEY_NAME })
            .put("logicalRequestAdvertised", logicalRequest.any { it.name == KEY_NAME })
            .put("physicalRequestAdvertised", physicalRequest.any { it.name == KEY_NAME })
            .put("requestedNumericEnable", 1)
            .put("applied", false)

        if (key == null) {
            out.put("classification", "BLOCKED_IDEALRAW_NOT_ADVERTISED_AS_LOGICAL_SESSION_KEY")
            return ApplyResult(false, out)
        }

        val valueClass = vendorKeyValueClass(key)
        out.put("runtimeValueClass", valueClass?.name ?: JSONObject.NULL)
        if (valueClass == null) {
            out.put("classification", "BLOCKED_IDEALRAW_RUNTIME_TYPE_UNAVAILABLE")
            return ApplyResult(false, out)
        }

        val builder = runCatching { device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE) }
            .getOrElse { e ->
                out.put("classification", "BLOCKED_SESSION_PARAMETER_BUILDER_CREATE_FAILED")
                out.put("error", "${e.javaClass.simpleName}: ${e.message}")
                return ApplyResult(false, out)
            }

        val writeKind = runCatching {
            @Suppress("UNCHECKED_CAST")
            when {
                valueClass == java.lang.Byte::class.java || valueClass == java.lang.Byte.TYPE -> {
                    builder.set(key as CaptureRequest.Key<Byte>, 1.toByte())
                    "Byte(1)"
                }
                valueClass == ByteArray::class.java -> {
                    builder.set(key as CaptureRequest.Key<ByteArray>, byteArrayOf(1))
                    "byte[1]{1}"
                }
                else -> error("unsupported safe IdealRAW runtime type ${valueClass.name}")
            }
        }.getOrElse { e ->
            out.put("classification", "BLOCKED_IDEALRAW_RUNTIME_TYPE_NOT_SAFE_ONE_BYTE_DOMAIN")
            out.put("error", "${e.javaClass.simpleName}: ${e.message}")
            return ApplyResult(false, out)
        }
        out.put("writeRepresentation", writeKind)

        val readback = runCatching { requestValue(builder, key) }.getOrElse { e ->
            out.put("classification", "BLOCKED_IDEALRAW_BUILDER_READBACK_FAILED")
            out.put("error", "${e.javaClass.simpleName}: ${e.message}")
            return ApplyResult(false, out)
        }
        out.put("builderReadback", encodeValue(readback))

        val readbackPass = when (readback) {
            is Byte -> readback.toInt() == 1
            is ByteArray -> readback.size == 1 && (readback[0].toInt() and 0xff) == 1
            else -> false
        }
        out.put("builderReadbackPass", readbackPass)
        if (!readbackPass) {
            out.put("classification", "BLOCKED_IDEALRAW_BUILDER_READBACK_MISMATCH")
            return ApplyResult(false, out)
        }

        val sessionRequest = runCatching { builder.build() }.getOrElse { e ->
            out.put("classification", "BLOCKED_IDEALRAW_SESSION_REQUEST_BUILD_FAILED")
            out.put("error", "${e.javaClass.simpleName}: ${e.message}")
            return ApplyResult(false, out)
        }

        val attach = runCatching { config.setSessionParameters(sessionRequest) }
        if (attach.isFailure) {
            val e = attach.exceptionOrNull()
            out.put("classification", "BLOCKED_IDEALRAW_SET_SESSION_PARAMETERS_FAILED")
            out.put("error", "${e?.javaClass?.simpleName}: ${e?.message}")
            return ApplyResult(false, out)
        }

        out.put("sessionParametersAttached", true)
            .put("applied", true)
            .put("classification", "IDEALRAW_ONE_APPLIED_AS_SINGLE_VENDOR_SESSION_VARIABLE__SEMANTICS_UNPROVEN")
            .put("authority", "CONTROL_INTERVENTION_ONLY_NOT_SENSOR_OR_RAW_SEMANTICS_PROOF")
        return ApplyResult(true, out)
    }

    private fun vendorKeyValueClass(key: CaptureRequest.Key<*>): Class<*>? {
        key.javaClass.methods.firstOrNull { it.name == "getType" && it.parameterCount == 0 }?.let { m ->
            val type = runCatching { m.invoke(key) as? Class<*> }.getOrNull()
            if (type != null) return type
        }
        var clazz: Class<*>? = key.javaClass
        while (clazz != null) {
            val method = runCatching {
                clazz.getDeclaredMethod("getType").apply { isAccessible = true }
            }.getOrNull()
            if (method != null) {
                val type = runCatching { method.invoke(key) as? Class<*> }.getOrNull()
                if (type != null) return type
            }
            clazz = clazz.superclass
        }
        return null
    }

    @Suppress("UNCHECKED_CAST")
    private fun requestValue(builder: CaptureRequest.Builder, key: CaptureRequest.Key<*>): Any? =
        builder.get(key as CaptureRequest.Key<Any>)

    private fun encodeValue(value: Any?): Any = when (value) {
        null -> JSONObject.NULL
        is Boolean, is Number, is String -> value
        is ByteArray -> JSONObject()
            .put("type", "byte[]")
            .put("length", value.size)
            .put("preview", JSONArray(value.take(16).map { it.toInt() and 0xff }))
        else -> value.toString()
    }
}
