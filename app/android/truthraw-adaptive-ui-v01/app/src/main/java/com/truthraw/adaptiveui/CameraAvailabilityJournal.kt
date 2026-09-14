package com.truthraw.adaptiveui

import java.time.Instant

internal data class CameraAvailabilitySnapshot(
    val logicalStates: Map<String, String>,
    val physicalStates: Map<String, String>,
    val recentEvents: List<String>,
)

/**
 * Process-lifetime diagnostics only. Availability observations help distinguish
 * "cameraIdList returned nothing" from "the camera service reports devices but
 * inventory parsing failed". They never grant capture/calibration authority.
 */
internal object CameraAvailabilityJournal {
    private const val MAX_EVENTS = 48
    private val lock = Any()
    private val logical = linkedMapOf<String, String>()
    private val physical = linkedMapOf<String, String>()
    private val events = ArrayDeque<String>()

    fun cameraAvailable(cameraId: String) = recordLogical(cameraId, "AVAILABLE")
    fun cameraUnavailable(cameraId: String) = recordLogical(cameraId, "UNAVAILABLE")

    fun physicalAvailable(logicalCameraId: String, physicalCameraId: String) =
        recordPhysical(logicalCameraId, physicalCameraId, "AVAILABLE")

    fun physicalUnavailable(logicalCameraId: String, physicalCameraId: String) =
        recordPhysical(logicalCameraId, physicalCameraId, "UNAVAILABLE")

    fun snapshot(): CameraAvailabilitySnapshot = synchronized(lock) {
        CameraAvailabilitySnapshot(
            logicalStates = logical.toMap(),
            physicalStates = physical.toMap(),
            recentEvents = events.toList(),
        )
    }

    private fun recordLogical(cameraId: String, state: String) = synchronized(lock) {
        logical[cameraId] = state
        appendEvent("logical $cameraId -> $state")
    }

    private fun recordPhysical(logicalId: String, physicalId: String, state: String) = synchronized(lock) {
        physical["$logicalId/$physicalId"] = state
        appendEvent("physical $logicalId/$physicalId -> $state")
    }

    private fun appendEvent(message: String) {
        events.addLast("${Instant.now()} · $message")
        while (events.size > MAX_EVENTS) events.removeFirst()
    }
}
