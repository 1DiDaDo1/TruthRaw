package com.truthraw.adaptiveui

import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraMetadata
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.CaptureResult
import android.hardware.camera2.params.ColorSpaceTransform
import android.hardware.camera2.params.LensShadingMap
import android.hardware.camera2.params.RggbChannelVector
import android.os.Build
import android.util.Rational
import org.json.JSONArray
import org.json.JSONObject

/**
 * Observation-only camera calibration telemetry.
 *
 * These values describe the exact Camera2 result when the vendor exposes them.
 * They never grant calibration authority and never overwrite the sealed RAW.
 */
object CameraCalibrationTelemetry {
    private const val KEY_DYNAMIC_BLACK = "android.sensor.dynamicBlackLevel"
    private const val KEY_DYNAMIC_WHITE = "android.sensor.dynamicWhiteLevel"
    private const val KEY_NOISE_PROFILE = "android.sensor.noiseProfile"
    private const val KEY_ROLLING_SHUTTER = "android.sensor.rollingShutterSkew"
    private const val KEY_LENS_INTRINSICS = "android.lens.intrinsicCalibration"
    private const val KEY_LENS_DISTORTION = "android.lens.distortion"
    private const val KEY_LENS_SHADING_MAP = "android.statistics.lensShadingMap"
    private const val KEY_COLOR_GAINS = "android.colorCorrection.gains"
    private const val KEY_COLOR_TRANSFORM = "android.colorCorrection.transform"
    private const val KEY_SENSOR_NEUTRAL = "android.sensor.neutralColorPoint"
    private const val KEY_GREEN_SPLIT = "android.sensor.greenSplit"

    fun requestLensShadingMap(
        builder: CaptureRequest.Builder,
        characteristics: CameraCharacteristics?,
        physicalCameraId: String? = null,
    ): Boolean {
        characteristics ?: return false
        val rawCapable =
            characteristics.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES)
                ?.contains(CameraMetadata.REQUEST_AVAILABLE_CAPABILITIES_RAW) == true
        if (!rawCapable) return false

        return runCatching {
            if (physicalCameraId != null && Build.VERSION.SDK_INT >= 28) {
                builder.setPhysicalCameraKey(
                    CaptureRequest.STATISTICS_LENS_SHADING_MAP_MODE,
                    CameraMetadata.STATISTICS_LENS_SHADING_MAP_MODE_ON,
                    physicalCameraId,
                )
            } else {
                builder.set(
                    CaptureRequest.STATISTICS_LENS_SHADING_MAP_MODE,
                    CameraMetadata.STATISTICS_LENS_SHADING_MAP_MODE_ON,
                )
            }
            true
        }.getOrDefault(false)
    }

    @Suppress("UNCHECKED_CAST")
    fun toJson(
        result: CaptureResult,
        characteristics: CameraCharacteristics?,
    ): JSONObject {
        val byName = result.keys.associateBy { it.name }
        fun read(name: String): Any? {
            val key = byName[name] ?: return null
            return runCatching {
                result.get(key as CaptureResult.Key<Any>)
            }.getOrNull()
        }

        val staticBlack = characteristics
            ?.get(CameraCharacteristics.SENSOR_BLACK_LEVEL_PATTERN)
        val staticWhite = characteristics
            ?.get(CameraCharacteristics.SENSOR_INFO_WHITE_LEVEL)
        val shadingApplied = characteristics
            ?.get(CameraCharacteristics.SENSOR_INFO_LENS_SHADING_APPLIED)
        val shadingSize = characteristics
            ?.get(CameraCharacteristics.LENS_INFO_SHADING_MAP_SIZE)

        return JSONObject()
            .put("authority", "CAMERA2_RESULT_OBSERVATION_ONLY")
            .put("calibrationAuthorityGranted", false)
            .put("scientificWritebackAllowed", false)
            .put("dynamicBlackLevel", render(read(KEY_DYNAMIC_BLACK)))
            .put("dynamicWhiteLevel", render(read(KEY_DYNAMIC_WHITE)))
            .put("staticBlackLevelPattern", staticBlack?.let {
                JSONArray(listOf(
                    it.getOffsetForIndex(0),
                    it.getOffsetForIndex(1),
                    it.getOffsetForIndex(2),
                    it.getOffsetForIndex(3),
                ))
            } ?: JSONObject.NULL)
            .put("staticWhiteLevel", staticWhite ?: JSONObject.NULL)
            .put("noiseProfile", render(read(KEY_NOISE_PROFILE)))
            .put("sensorNeutralColorPoint", render(read(KEY_SENSOR_NEUTRAL)))
            .put("greenSplit", render(read(KEY_GREEN_SPLIT)))
            .put("rollingShutterSkewNs", render(read(KEY_ROLLING_SHUTTER)))
            .put("lensIntrinsicCalibration", render(read(KEY_LENS_INTRINSICS)))
            .put("lensDistortion", render(read(KEY_LENS_DISTORTION)))
            .put("lensShadingAlreadyAppliedToRaw", shadingApplied ?: JSONObject.NULL)
            .put("lensShadingMapSize", shadingSize?.let {
                JSONArray(listOf(it.width, it.height))
            } ?: JSONObject.NULL)
            .put("lensShadingMap", render(read(KEY_LENS_SHADING_MAP)))
            .put("colorCorrectionGains", render(read(KEY_COLOR_GAINS)))
            .put("colorCorrectionTransform", render(read(KEY_COLOR_TRANSFORM)))
    }

    private fun render(value: Any?): Any {
        return when (value) {
            null -> JSONObject.NULL
            is FloatArray -> JSONArray(value.map { finiteNumber(it) })
            is DoubleArray -> JSONArray(value.map { finiteNumber(it) })
            is IntArray -> JSONArray(value.toList())
            is LongArray -> JSONArray(value.toList())
            is Array<*> -> JSONArray(value.map { render(it) })
            is Rational -> JSONObject()
                .put("numerator", value.numerator)
                .put("denominator", value.denominator)
                .put("float", finiteNumber(value.toFloat()))
            is Pair<*, *> -> JSONArray(listOf(render(value.first), render(value.second)))
            is RggbChannelVector -> JSONArray(listOf(
                finiteNumber(value.red),
                finiteNumber(value.greenEven),
                finiteNumber(value.greenOdd),
                finiteNumber(value.blue),
            ))
            is ColorSpaceTransform -> JSONArray((0 until 9).map { index ->
                render(value.getElement(index / 3, index % 3))
            })
            is LensShadingMap -> {
                val gains = FloatArray(value.gainFactorCount)
                value.copyGainFactors(gains, 0)
                JSONObject()
                    .put("rows", value.rowCount)
                    .put("columns", value.columnCount)
                    .put("gainFactorsRGeGoB", JSONArray(gains.map { finiteNumber(it) }))
            }
            is Number -> finiteNumber(value.toDouble())
            is Boolean, is String -> value
            else -> value.toString()
        }
    }

    private fun finiteNumber(value: Float): Any =
        if (value.isFinite()) value.toDouble() else JSONObject.NULL

    private fun finiteNumber(value: Double): Any =
        if (value.isFinite()) value else JSONObject.NULL
}
