package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.content.Intent
import android.net.Uri
import android.provider.OpenableColumns
import java.util.UUID

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
