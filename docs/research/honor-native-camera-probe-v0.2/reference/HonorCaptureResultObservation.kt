@file:Suppress("UNCHECKED_CAST")

package truthraw.fotograaf.honorprobe.v02.reference

import android.hardware.camera2.CaptureResult
import android.hardware.camera2.TotalCaptureResult
import android.os.Build

/**
 * Reference-only capture-result reader.
 *
 * It observes runtime result keys; it does not use a stock-APK hint as proof.
 */
object HonorCaptureResultObservation {

    data class VendorResult(
        val name: String,
        val valueClass: String?,
        val renderedValue: String,
    )

    data class Observation(
        val sensorTimestampNs: Long?,
        val physicalResultCameraIds: List<String>,
        val honorResults: List<VendorResult>,
        val previewCameraPhysicalId: String?,
        val opticalSwitchStatus: String?,
    )

    private const val HONOR_PREFIX = "com.hihonor."
    private const val KEY_PREVIEW_PHYSICAL_ID =
        "com.hihonor.capture.metadata.previewCameraPhysicalId"
    private const val KEY_OPTICAL_SWITCH_STATUS =
        "com.hihonor.capture.metadata.opticalSwitchStatus"

    fun observe(result: TotalCaptureResult): Observation {
        val vendor = result.keys
            .asSequence()
            .filter { it.name.startsWith(HONOR_PREFIX) }
            .map { key ->
                val value = runCatching {
                    result.get(key as CaptureResult.Key<Any>)
                }.getOrNull()
                VendorResult(
                    name = key.name,
                    valueClass = value?.javaClass?.name,
                    renderedValue = render(value),
                )
            }
            .sortedBy { it.name }
            .toList()

        val physicalIds = if (Build.VERSION.SDK_INT >= 28) {
            runCatching { result.physicalCameraResults.keys.sorted() }
                .getOrDefault(emptyList())
        } else {
            emptyList()
        }

        return Observation(
            sensorTimestampNs = result.get(CaptureResult.SENSOR_TIMESTAMP),
            physicalResultCameraIds = physicalIds,
            honorResults = vendor,
            previewCameraPhysicalId = vendor
                .firstOrNull { it.name == KEY_PREVIEW_PHYSICAL_ID }
                ?.renderedValue,
            opticalSwitchStatus = vendor
                .firstOrNull { it.name == KEY_OPTICAL_SWITCH_STATUS }
                ?.renderedValue,
        )
    }

    fun rawTimestampIdentityPass(imageTimestampNs: Long, result: Observation): Boolean =
        result.sensorTimestampNs != null && imageTimestampNs == result.sensorTimestampNs

    private fun render(value: Any?): String = when (value) {
        null -> "null"
        is ByteArray -> value.contentToString()
        is ShortArray -> value.contentToString()
        is IntArray -> value.contentToString()
        is LongArray -> value.contentToString()
        is FloatArray -> value.contentToString()
        is DoubleArray -> value.contentToString()
        is BooleanArray -> value.contentToString()
        is Array<*> -> value.contentDeepToString()
        else -> value.toString()
    }
}
