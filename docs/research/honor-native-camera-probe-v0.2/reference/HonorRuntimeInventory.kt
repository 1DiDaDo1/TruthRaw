@file:Suppress("UNCHECKED_CAST")

package truthraw.fotograaf.honorprobe.v02.reference

import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraManager
import android.os.Build

/**
 * Reference-only source for HONOR Native RAW Probe v0.2.
 *
 * This deliberately reads vendor characteristics through the Key objects that
 * the runtime itself exposes. It does not construct guessed vendor keys and it
 * does not set vendor requests.
 *
 * This file is not yet wired into the historical FotoGraaf APK source tree.
 */
object HonorRuntimeInventory {

    data class RuntimeKeyValue(
        val name: String,
        val valueClass: String?,
        val renderedValue: String,
    )

    data class CameraInventory(
        val cameraId: String,
        val physicalCameraIds: List<String>,
        val honorCharacteristics: List<RuntimeKeyValue>,
        val availableHonorRequestKeys: List<String>,
        val availableHonorResultKeys: List<String>,
        val availableHonorPhysicalRequestKeys: List<String>,
        val professionalTeleRawLogicalCameraId: Int?,
        val rawSensorResolution: IntArray?,
        val rawCaptureSize: IntArray?,
        val rawStreamConfigurations: IntArray?,
    )

    private const val HONOR_PREFIX = "com.hihonor."

    private const val KEY_PRO_TELE_RAW_LOGICAL_ID =
        "com.hihonor.device.capabilities.professionalTeleRawLogicalCameraID"
    private const val KEY_RAW_SENSOR_RESOLUTION =
        "com.hihonor.device.capabilities.rawSensorResolution"
    private const val KEY_RAW_CAPTURE_SIZE =
        "com.hihonor.device.capabilities.rawCaptureSize"
    private const val KEY_RAW_STREAM_CONFIG =
        "com.hihonor.device.capabilities.hwCaptureRawStreamConfigurations"

    fun scan(manager: CameraManager): List<CameraInventory> =
        manager.cameraIdList.map { cameraId -> scanOne(manager, cameraId) }

    fun scanOne(manager: CameraManager, cameraId: String): CameraInventory {
        val c = manager.getCameraCharacteristics(cameraId)

        val vendorValues = c.keys
            .asSequence()
            .filter { it.name.startsWith(HONOR_PREFIX) }
            .map { key ->
                val value = runCatching {
                    c.get(key as CameraCharacteristics.Key<Any>)
                }.getOrNull()
                RuntimeKeyValue(
                    name = key.name,
                    valueClass = value?.javaClass?.name,
                    renderedValue = render(value),
                )
            }
            .sortedBy { it.name }
            .toList()

        val requestKeys = runCatching { c.availableCaptureRequestKeys }
            .getOrNull().orEmpty()
            .map { it.name }
            .filter { it.startsWith(HONOR_PREFIX) }
            .sorted()

        val resultKeys = runCatching { c.availableCaptureResultKeys }
            .getOrNull().orEmpty()
            .map { it.name }
            .filter { it.startsWith(HONOR_PREFIX) }
            .sorted()

        val physicalRequestKeys = if (Build.VERSION.SDK_INT >= 28) {
            runCatching { c.availablePhysicalCameraRequestKeys }
                .getOrNull().orEmpty()
                .map { it.name }
                .filter { it.startsWith(HONOR_PREFIX) }
                .sorted()
        } else {
            emptyList()
        }

        val physicalIds = if (Build.VERSION.SDK_INT >= 28) {
            runCatching { c.physicalCameraIds.toList().sorted() }.getOrDefault(emptyList())
        } else {
            emptyList()
        }

        return CameraInventory(
            cameraId = cameraId,
            physicalCameraIds = physicalIds,
            honorCharacteristics = vendorValues,
            availableHonorRequestKeys = requestKeys,
            availableHonorResultKeys = resultKeys,
            availableHonorPhysicalRequestKeys = physicalRequestKeys,
            professionalTeleRawLogicalCameraId = readByName(c, KEY_PRO_TELE_RAW_LOGICAL_ID) as? Int,
            rawSensorResolution = readByName(c, KEY_RAW_SENSOR_RESOLUTION) as? IntArray,
            rawCaptureSize = readByName(c, KEY_RAW_CAPTURE_SIZE) as? IntArray,
            rawStreamConfigurations = readByName(c, KEY_RAW_STREAM_CONFIG) as? IntArray,
        )
    }

    fun readByName(c: CameraCharacteristics, name: String): Any? {
        val key = c.keys.firstOrNull { it.name == name } ?: return null
        return runCatching {
            c.get(key as CameraCharacteristics.Key<Any>)
        }.getOrNull()
    }

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
