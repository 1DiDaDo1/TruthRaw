package com.truthraw.fullsensorprobe

import android.graphics.ImageFormat
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraMetadata
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CaptureResult
import android.hardware.camera2.TotalCaptureResult
import android.util.Size
import org.json.JSONArray
import org.json.JSONObject

/**
 * FotoGraaf/HONOR sample-domain observation helper v0.4.
 *
 * Evidence boundary:
 * - emits standard Camera2 static/runtime observations and deterministic math only;
 * - does not grant calibration authority;
 * - does not infer a sensor model, native photodiode identity, or a binning/remosaic mechanism.
 */
object HonorSampleDomainV04 {

    fun staticCharacteristics(
        cameraManager: CameraManager,
        logicalCameraId: String,
        physicalCameraId: String
    ): JSONObject {
        val logical = cameraManager.getCameraCharacteristics(logicalCameraId)
        val physical = cameraManager.getCameraCharacteristics(physicalCameraId)

        return JSONObject()
            .put("logicalCameraId", logicalCameraId)
            .put("physicalCameraId", physicalCameraId)
            .put("logicalPhysicalIds", JSONArray(logical.physicalCameraIds.sorted()))
            .put("hardwareLevel", physical.get(CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL))
            .put(
                "capabilities",
                JSONArray(
                    physical.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES)
                        ?.toList() ?: emptyList<Int>()
                )
            )
            .put("pixelArrayDefault", size(physical.get(CameraCharacteristics.SENSOR_INFO_PIXEL_ARRAY_SIZE)))
            .put("activeArrayDefault", rect(physical.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE)))
            .put(
                "preCorrectionActiveArrayDefault",
                rect(physical.get(CameraCharacteristics.SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE))
            )
            .put(
                "pixelArrayMaximumResolution",
                size(physical.get(CameraCharacteristics.SENSOR_INFO_PIXEL_ARRAY_SIZE_MAXIMUM_RESOLUTION))
            )
            .put(
                "activeArrayMaximumResolution",
                rect(physical.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE_MAXIMUM_RESOLUTION))
            )
            .put(
                "preCorrectionActiveArrayMaximumResolution",
                rect(physical.get(CameraCharacteristics.SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE_MAXIMUM_RESOLUTION))
            )
            .put(
                "sensorInfoBinningFactor",
                size(physical.get(CameraCharacteristics.SENSOR_INFO_BINNING_FACTOR))
            )
            .put("rawSizesDefault", rawSizes(physical, maximumResolution = false))
            .put("rawSizesMaximumResolution", rawSizes(physical, maximumResolution = true))
            .put(
                "rawHighResolutionSizesMaximumResolution",
                rawHighResolutionSizes(physical)
            )
            .put(
                "sensorPixelModeRequestKeyPresent",
                physical.availableCaptureRequestKeys?.any { it.name == "android.sensor.pixelMode" } == true
            )
            .put(
                "sensorRawBinningFactorUsedResultKeyPresent",
                physical.availableCaptureResultKeys?.any { it.name == "android.sensor.rawBinningFactorUsed" } == true
            )
    }

    fun runtimeResult(
        totalResult: TotalCaptureResult,
        physicalCameraId: String
    ): JSONObject {
        val physicalResult = totalResult.physicalCameraTotalResults[physicalCameraId]
        val effective: CaptureResult = physicalResult ?: totalResult

        return JSONObject()
            .put("resultCameraId", effective.cameraId)
            .put("physicalResultPresent", physicalResult != null)
            .put("requestedPhysicalCameraId", physicalCameraId)
            .put(
                "activePhysicalCameraId",
                totalResult.get(CaptureResult.LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID)
            )
            .put(
                "sensorPixelMode",
                pixelModeName(effective.get(CaptureResult.SENSOR_PIXEL_MODE))
            )
            .put(
                "sensorRawBinningFactorUsed",
                effective.get(CaptureResult.SENSOR_RAW_BINNING_FACTOR_USED)
            )
            .put("sensorTimestampNs", effective.get(CaptureResult.SENSOR_TIMESTAMP))
            .put("frameNumber", effective.frameNumber)
            .put("sensitivityIso", effective.get(CaptureResult.SENSOR_SENSITIVITY))
            .put("exposureTimeNs", effective.get(CaptureResult.SENSOR_EXPOSURE_TIME))
            .put("focalLengthMm", effective.get(CaptureResult.LENS_FOCAL_LENGTH))
            .put("focusDistanceDiopters", effective.get(CaptureResult.LENS_FOCUS_DISTANCE))
            .put("afMode", effective.get(CaptureResult.CONTROL_AF_MODE))
            .put("afState", effective.get(CaptureResult.CONTROL_AF_STATE))
            .put("oisMode", effective.get(CaptureResult.LENS_OPTICAL_STABILIZATION_MODE))
            .put("noiseReductionMode", effective.get(CaptureResult.NOISE_REDUCTION_MODE))
    }

    /**
     * Creates a conservative sample-domain classification from observed values.
     * `maximumWidth/Height` must come from observed standard Camera2 maximum-resolution data.
     */
    fun sampleDomain(
        physicalCameraId: String,
        width: Int,
        height: Int,
        cfa: String,
        actualPixelMode: Int?,
        rawBinningFactorUsed: Any?,
        maximumWidth: Int? = null,
        maximumHeight: Int? = null
    ): JSONObject {
        val mode = pixelModeName(actualPixelMode)
        val modeToken = when (actualPixelMode) {
            CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION -> "MAX"
            CameraMetadata.SENSOR_PIXEL_MODE_DEFAULT -> "DEFAULT"
            else -> "UNRESOLVED"
        }

        val out = JSONObject()
            .put(
                "domainId",
                "PHYSICAL_${sanitize(physicalCameraId)}_${modeToken}_RAW_${width}x${height}_${sanitize(cfa)}"
            )
            .put("classificationAuthority", "OBSERVED_CAMERA2_RUNTIME")
            .put("sensorPixelMode", mode)
            .put("rawBinningFactorUsed", rawBinningFactorUsed ?: JSONObject.NULL)
            .put("rawBinningInterpretation", "UNRESOLVED")
            .put("parentMaximumResolutionDomain", JSONObject.NULL)
            .put("derivedLinearScaleFromMaximumResolution", JSONObject.NULL)
            .put("derivedAreaScaleFromMaximumResolution", JSONObject.NULL)

        if (
            maximumWidth != null && maximumHeight != null &&
            maximumWidth > 0 && maximumHeight > 0 && width > 0 && height > 0 &&
            maximumWidth % width == 0 && maximumHeight % height == 0
        ) {
            val sx = maximumWidth / width
            val sy = maximumHeight / height
            if (sx == sy) {
                out.put(
                    "parentMaximumResolutionDomain",
                    "PHYSICAL_${sanitize(physicalCameraId)}_MAX_RAW_${maximumWidth}x${maximumHeight}_${sanitize(cfa)}"
                )
                out.put("derivedLinearScaleFromMaximumResolution", sx)
                out.put("derivedAreaScaleFromMaximumResolution", sx.toLong() * sy.toLong())
            }
        }

        return out
    }

    fun observedClaims(
        logicalCameraId: String,
        physicalCameraId: String,
        width: Int,
        height: Int,
        physicalResultPresent: Boolean,
        timestampIdentityPass: Boolean
    ): JSONArray = JSONArray().apply {
        put("RAW output requested from logical $logicalCameraId bound to physical $physicalCameraId")
        put("RAW output dimensions observed as ${width}x${height}")
        if (physicalResultPresent) put("Physical TotalCaptureResult for camera $physicalCameraId present")
        if (timestampIdentityPass) put("Image timestamp equals sensor timestamp for the captured frame")
    }

    fun claimBoundary(): JSONObject = JSONObject()
        .put("calibrationAuthorityGranted", false)
        .put("c0IdentitySealed", false)
        .put("scientificMasterModified", false)
        .put("sensorModelIdentitySealed", false)
        .put("nativePhotodiodeIdentityClaimed", false)
        .put("upstreamBinningOrRemosaicMechanismResolved", false)

    private fun pixelModeName(mode: Int?): Any = when (mode) {
        CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION -> "MAXIMUM_RESOLUTION"
        CameraMetadata.SENSOR_PIXEL_MODE_DEFAULT -> "DEFAULT"
        null -> JSONObject.NULL
        else -> "UNKNOWN_$mode"
    }

    private fun rawSizes(c: CameraCharacteristics, maximumResolution: Boolean): JSONArray {
        val map = if (maximumResolution) {
            c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION)
        } else {
            c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
        }
        return sizes(try { map?.getOutputSizes(ImageFormat.RAW_SENSOR) } catch (_: Throwable) { null })
    }

    private fun rawHighResolutionSizes(c: CameraCharacteristics): JSONArray {
        val map = c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION)
        return sizes(try { map?.getHighResolutionOutputSizes(ImageFormat.RAW_SENSOR) } catch (_: Throwable) { null })
    }

    private fun sizes(values: Array<Size>?): JSONArray = JSONArray(
        (values ?: emptyArray())
            .sortedByDescending { it.width.toLong() * it.height.toLong() }
            .map { listOf(it.width, it.height) }
    )

    private fun size(value: Size?): Any = value?.let {
        JSONArray(listOf(it.width, it.height))
    } ?: JSONObject.NULL

    private fun rect(value: android.graphics.Rect?): Any = value?.let {
        JSONArray(listOf(it.left, it.top, it.right, it.bottom))
    } ?: JSONObject.NULL

    private fun sanitize(value: String): String = value.replace(Regex("[^A-Za-z0-9]+"), "_")
}
