package com.truthraw.adaptiveui

import android.hardware.HardwareBuffer
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CaptureResult
import android.media.Image
import android.os.Build
import org.json.JSONArray
import org.json.JSONObject

/**
 * Read-only observation of the Android buffer envelope that carries a Camera2 RAW_SENSOR Image.
 *
 * This probe deliberately does NOT lock, map, write, reinterpret, demosaic, normalize, or copy the
 * HardwareBuffer. The original Image.Plane[0] remains the primary byte evidence and must already be
 * sealed before this probe is invoked.
 */
object Camera2EnvelopeProbe {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    private external fun nativeDescribeHardwareBuffer(buffer: HardwareBuffer): String

    fun observe(
        image: Image,
        physicalCharacteristics: CameraCharacteristics,
        physicalResult: CaptureResult,
    ): JSONObject {
        val root = JSONObject()
            .put("schema", "truthraw.camera2-hal-buffer-envelope.v0.1")
            .put("observationOnly", true)
            .put("hardwareBufferLocked", false)
            .put("hardwareBufferWritten", false)
            .put("image", JSONObject()
                .put("width", image.width)
                .put("height", image.height)
                .put("format", image.format)
                .put("timestampNs", image.timestamp)
                .put("planeCount", image.planes.size)
                .put("cropRect", image.cropRect.toShortString())
                .put("dataSpace", if (Build.VERSION.SDK_INT >= 33) image.dataSpace else JSONObject.NULL))

        val planeArray = JSONArray()
        image.planes.forEachIndexed { index, plane ->
            val duplicate = plane.buffer.duplicate()
            planeArray.put(JSONObject()
                .put("index", index)
                .put("pixelStride", plane.pixelStride)
                .put("rowStride", plane.rowStride)
                .put("bufferPosition", duplicate.position())
                .put("bufferLimit", duplicate.limit())
                .put("bufferCapacity", duplicate.capacity())
                .put("bufferRemaining", duplicate.remaining())
                .put("direct", duplicate.isDirect)
                .put("readOnly", duplicate.isReadOnly))
        }
        root.getJSONObject("image").put("planes", planeArray)

        val hardware = runCatching { image.hardwareBuffer }.getOrNull()
        if (hardware == null) {
            root.put("hardwareBuffer", JSONObject()
                .put("available", false)
                .put("reason", "Image.getHardwareBuffer returned null"))
        } else {
            // Do not close this wrapper here: the Image still owns the live capture buffer. The
            // probe is one-shot and the wrapper becomes unusable when Image.close() is called.
            val javaView = JSONObject()
                .put("available", true)
                .put("width", hardware.width)
                .put("height", hardware.height)
                .put("layers", hardware.layers)
                .put("format", hardware.format)
                .put("usageUnsignedHex", java.lang.Long.toUnsignedString(hardware.usage, 16))
                .put("isClosed", hardware.isClosed)
                .put("javaGetId", if (Build.VERSION.SDK_INT >= 34) hardware.id else JSONObject.NULL)
                .put("wrapperExplicitlyClosedByProbe", false)
            val native = runCatching { JSONObject(nativeDescribeHardwareBuffer(hardware)) }
                .getOrElse { JSONObject().put("error", "${it.javaClass.simpleName}: ${it.message}") }
            javaView.put("nativeDescriptor", native)
            root.put("hardwareBuffer", javaView)
        }

        root.put("physicalCharacteristicsMetadata", characteristicsEnvelope(physicalCharacteristics))
        root.put("physicalCaptureResultMetadata", captureResultEnvelope(physicalResult))
        return root
    }

    private fun characteristicsEnvelope(c: CameraCharacteristics): JSONObject {
        val allNames = c.keys.map { it.name }.sorted()
        val requestNames = c.availableCaptureRequestKeys?.map { it.name }?.sorted().orEmpty()
        val resultNames = c.availableCaptureResultKeys?.map { it.name }?.sorted().orEmpty()
        val physicalRequestNames = c.availablePhysicalCameraRequestKeys?.map { it.name }?.sorted().orEmpty()
        val vendorEntries = JSONArray()
        c.keys.filter { isVendor(it.name) }.sortedBy { it.name }.forEach { key ->
            vendorEntries.put(JSONObject()
                .put("name", key.name)
                .put("value", runCatching { characteristicValue(c, key) }.fold(
                    onSuccess = { encodeValue(it) },
                    onFailure = { "ERROR:${it.javaClass.simpleName}:${it.message}" },
                )))
        }
        return JSONObject()
            .put("allKeyCount", allNames.size)
            .put("allKeyNames", JSONArray(allNames))
            .put("availableCaptureRequestKeyNames", JSONArray(requestNames))
            .put("availableCaptureResultKeyNames", JSONArray(resultNames))
            .put("availablePhysicalCameraRequestKeyNames", JSONArray(physicalRequestNames))
            .put("vendorEntries", vendorEntries)
    }

    private fun captureResultEnvelope(r: CaptureResult): JSONObject {
        val keys = r.keys.sortedBy { it.name }
        val vendor = JSONArray()
        keys.filter { isVendor(it.name) }.forEach { key ->
            vendor.put(JSONObject()
                .put("name", key.name)
                .put("value", runCatching { resultValue(r, key) }.fold(
                    onSuccess = { encodeValue(it) },
                    onFailure = { "ERROR:${it.javaClass.simpleName}:${it.message}" },
                )))
        }
        return JSONObject()
            .put("cameraId", r.cameraId)
            .put("keyCount", keys.size)
            .put("keyNames", JSONArray(keys.map { it.name }))
            .put("vendorEntries", vendor)
    }

    private fun isVendor(name: String): Boolean = !name.startsWith("android.")

    @Suppress("UNCHECKED_CAST")
    private fun characteristicValue(c: CameraCharacteristics, key: CameraCharacteristics.Key<*>): Any? =
        c.get(key as CameraCharacteristics.Key<Any>)

    @Suppress("UNCHECKED_CAST")
    private fun resultValue(r: CaptureResult, key: CaptureResult.Key<*>): Any? =
        r.get(key as CaptureResult.Key<Any>)

    private fun encodeValue(value: Any?): Any = when (value) {
        null -> JSONObject.NULL
        is Boolean, is Number, is String -> value
        is ByteArray -> encodedArray("byte[]", value.size, value.take(64).map { it.toInt() and 0xff })
        is IntArray -> encodedArray("int[]", value.size, value.take(64))
        is LongArray -> encodedArray("long[]", value.size, value.take(64))
        is FloatArray -> encodedArray("float[]", value.size, value.take(64))
        is DoubleArray -> encodedArray("double[]", value.size, value.take(64))
        is ShortArray -> encodedArray("short[]", value.size, value.take(64))
        is BooleanArray -> encodedArray("boolean[]", value.size, value.take(64))
        is Array<*> -> encodedArray("Object[]", value.size, value.take(64).map { it?.toString() })
        else -> value.toString()
    }

    private fun encodedArray(type: String, length: Int, preview: List<*>): JSONObject =
        JSONObject()
            .put("type", type)
            .put("length", length)
            .put("preview", JSONArray(preview))
            .put("truncated", length > preview.size)
}
