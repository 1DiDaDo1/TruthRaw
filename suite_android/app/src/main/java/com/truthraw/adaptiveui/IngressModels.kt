package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.content.Intent
import android.net.Uri
import android.provider.OpenableColumns
import java.io.File
import java.security.MessageDigest
import java.util.UUID
import org.json.JSONObject

/**
 * Ingress holds only document handles and lightweight metadata.
 * It deliberately never reads RAW payload bytes into an in-memory payload buffer.
 */
enum class SourceIngressRoute {
    IMPORTED_FILE,
    CAMERA_CAPTURE,
}

data class RawHandle(
    val uri: Uri,
    val displayName: String,
    val declaredSizeBytes: Long?,
    val mimeType: String? = null,
    val format: RawFormatProfile = RawFormatRegistry.classify(displayName, mimeType),
    val sourceRoute: SourceIngressRoute = SourceIngressRoute.IMPORTED_FILE,
    // Optional acquisition ancestry. For camera-origin DNG this points back to
    // the separately sealed app-visible RAW_SENSOR evidence; it does not make
    // the DNG byte-identical to that upstream evidence.
    val acquisitionEvidencePath: String? = null,
    val upstreamSealedSourceSha256: String? = null,
    val upstreamSourceRole: String? = null,
    // True only when the app-internal Camera-5 v0.53 evidence binds this exact
    // processing DNG to the sealed 16320x12288 envelope and admitted 4080x3072
    // payload. This authorizes the TruthNegative target geometry only; it does
    // not upgrade any target pixel to measured authority.
    val verifiedCamera5TruthNegative200MpEnvelope: Boolean = false,
    val acquisitionEvidenceSha256: String? = null,
)

enum class JobState {
    QUEUED,
    READY,
    PROCESSING,
    COMPLETE,
    BLOCKED,
}

data class RawJob(
    val id: String = UUID.randomUUID().toString(),
    val source: RawHandle,
    val state: JobState = JobState.READY,
)

enum class InputRoute {
    SINGLE_ONE_OUTPUT,
    SINGLE_MULTIPLE_OUTPUTS,
    BATCH_INDEPENDENT,
    MULTI_CAPTURE_ENHANCED,
    MULTI_CAPTURE_HDR,
}

data class BatchSession(
    val jobs: List<RawJob> = emptyList(),
    val route: InputRoute = InputRoute.BATCH_INDEPENDENT,
) {
    val selectedCount: Int get() = jobs.size

    fun withJobs(newJobs: List<RawJob>): BatchSession {
        val defaultRoute = when (newJobs.size) {
            0 -> InputRoute.BATCH_INDEPENDENT
            1 -> InputRoute.SINGLE_ONE_OUTPUT
            else -> InputRoute.BATCH_INDEPENDENT
        }
        return copy(jobs = newJobs, route = defaultRoute)
    }
}

object RawIngress {
    private data class Camera5Qualification(
        val verified: Boolean = false,
        val evidenceSha256: String? = null,
    )

    private fun sha256(file: File): String {
        val digest = MessageDigest.getInstance("SHA-256")
        file.inputStream().use { input ->
            val buffer = ByteArray(1024 * 1024)
            while (true) {
                val read = input.read(buffer)
                if (read <= 0) break
                digest.update(buffer, 0, read)
            }
        }
        return digest.digest().joinToString("") { "%02x".format(it) }
    }

    private fun qualifyCamera5TruthNegative200Mp(
        processingDng: File,
        evidenceFile: File?,
        upstreamSha256: String?,
    ): Camera5Qualification {
        if (evidenceFile == null || upstreamSha256 == null) {
            return Camera5Qualification()
        }
        return runCatching {
            val root = JSONObject(evidenceFile.readText())
            val topology = root.getJSONObject("topologyAdmission")
            val request = root.getJSONObject("requestTopology")
            val capture = root.getJSONObject("captureRoute")
            val raw = root.getJSONObject("rawPayload")

            val processingSha = sha256(processingDng)
            val evidenceSha = sha256(evidenceFile)
            val reportedIds = capture.getJSONArray("reportedPhysicalIds")
            val reportedPhysical5 =
                (0 until reportedIds.length()).any { reportedIds.optString(it) == "5" }

            val verified =
                root.optString("schema") ==
                    "truthraw.fotograaf-camera5-200mp-v014-route-replay.v0.53" &&
                root.optInt("physicalFrameCount", -1) == 1 &&
                root.optInt("independentEvidenceCount", -1) == 1 &&
                request.optString("requestedPhysicalCameraId") == "5" &&
                request.optBoolean("outputPhysicalBinding", false) &&
                capture.optString("physicalResultCameraId") == "5" &&
                reportedPhysical5 &&
                capture.optInt("width", -1) == 16320 &&
                capture.optInt("height", -1) == 12288 &&
                raw.optString("sha256").equals(upstreamSha256, ignoreCase = true) &&
                raw.optLong("bytes", -1L) == 401080320L &&
                topology.optString("status") ==
                    "ADMITTED_EXACT_STANDARD_RAW_PREFIX_TO_DERIVED_DNG" &&
                topology.optInt("sourceEnvelopeWidth", -1) == 16320 &&
                topology.optInt("sourceEnvelopeHeight", -1) == 12288 &&
                topology.optString("sourceEnvelopeSha256")
                    .equals(upstreamSha256, ignoreCase = true) &&
                !topology.optBoolean(
                    "sourceEnvelopePromotedTo200MpScientificMaster",
                    true,
                ) &&
                topology.optInt("admittedWidth", -1) == 4080 &&
                topology.optInt("admittedHeight", -1) == 3072 &&
                topology.optLong("admittedPayloadBytes", -1L) == 25067520L &&
                topology.optString("processingDngFile") == processingDng.name &&
                topology.optString("processingDngSha256")
                    .equals(processingSha, ignoreCase = true) &&
                topology.optBoolean("mainHouseMustResealDng", false) &&
                topology.optBoolean("scientificMasterCreationAllowed", false) &&
                topology.optBoolean(
                    "truthNegativeAllowedOnlyAfterMainHouseAdmission",
                    false,
                ) &&
                !topology.optBoolean("nativeAdcGeometryProven", true) &&
                !topology.optBoolean("optical200MpIndependenceProven", true)

            Camera5Qualification(
                verified = verified,
                evidenceSha256 = evidenceSha.takeIf { verified },
            )
        }.getOrElse { Camera5Qualification() }
    }

    fun readInternalCameraFile(
        file: File,
        acquisitionEvidenceFile: File? = null,
        upstreamSealedSourceSha256: String? = null,
    ): RawJob {
        require(file.isFile && file.canRead()) { "Camera source file is not readable." }
        if (acquisitionEvidenceFile != null) {
            require(acquisitionEvidenceFile.isFile && acquisitionEvidenceFile.canRead()) {
                "Camera acquisition evidence file is not readable."
            }
        }
        val upstreamSha = upstreamSealedSourceSha256?.lowercase()
        if (upstreamSha != null) {
            require(upstreamSha.matches(Regex("[0-9a-f]{64}"))) {
                "Camera upstream sealed source SHA-256 is invalid."
            }
        }
        val mimeType = when (file.extension.lowercase()) {
            "dng" -> "image/x-adobe-dng"
            else -> "application/octet-stream"
        }
        val format = RawFormatRegistry.classify(file.name, mimeType)
        val camera5Qualification = qualifyCamera5TruthNegative200Mp(
            processingDng = file,
            evidenceFile = acquisitionEvidenceFile,
            upstreamSha256 = upstreamSha,
        )
        return RawJob(
            source = RawHandle(
                uri = Uri.fromFile(file),
                displayName = file.name,
                declaredSizeBytes = file.length(),
                mimeType = mimeType,
                format = format,
                sourceRoute = SourceIngressRoute.CAMERA_CAPTURE,
                acquisitionEvidencePath = acquisitionEvidenceFile?.absolutePath,
                upstreamSealedSourceSha256 = upstreamSha,
                upstreamSourceRole = if (upstreamSha != null) {
                    "APP_VISIBLE_CAMERA2_RAW_SENSOR_SOURCE_FIRST_SEALED"
                } else null,
                verifiedCamera5TruthNegative200MpEnvelope =
                    camera5Qualification.verified,
                acquisitionEvidenceSha256 =
                    camera5Qualification.evidenceSha256,
            ),
        )
    }

    fun readHandlesOnly(
        resolver: ContentResolver,
        uris: List<Uri>,
        resultIntentFlags: Int,
    ): List<RawJob> = uris.distinct().map { uri ->
        persistReadPermissionIfAvailable(resolver, uri, resultIntentFlags)
        RawJob(source = queryMetadata(resolver, uri))
    }

    private fun queryMetadata(resolver: ContentResolver, uri: Uri): RawHandle {
        var name = uri.lastPathSegment ?: "RAW"
        var size: Long? = null
        val mimeType = resolver.getType(uri)
        resolver.query(
            uri,
            arrayOf(OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE),
            null,
            null,
            null,
        )?.use { cursor ->
            if (cursor.moveToFirst()) {
                val nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
                val sizeIndex = cursor.getColumnIndex(OpenableColumns.SIZE)
                if (nameIndex >= 0 && !cursor.isNull(nameIndex)) name = cursor.getString(nameIndex)
                if (sizeIndex >= 0 && !cursor.isNull(sizeIndex)) size = cursor.getLong(sizeIndex)
            }
        }
        return RawHandle(
            uri = uri,
            displayName = name,
            declaredSizeBytes = size,
            mimeType = mimeType,
            format = RawFormatRegistry.classify(name, mimeType),
            sourceRoute = SourceIngressRoute.IMPORTED_FILE,
        )
    }

    private fun persistReadPermissionIfAvailable(
        resolver: ContentResolver,
        uri: Uri,
        resultIntentFlags: Int,
    ) {
        val hasRead = resultIntentFlags and Intent.FLAG_GRANT_READ_URI_PERMISSION != 0
        val hasPersistable = resultIntentFlags and Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION != 0
        if (!hasRead || !hasPersistable) return
        try {
            resolver.takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION)
        } catch (_: SecurityException) {
            // The URI remains valid for the current grant even if the provider declines persistence.
        }
    }
}
