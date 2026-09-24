package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.graphics.Color
import android.os.Build
import android.os.Bundle
import android.view.View
import android.view.ViewGroup
import android.view.WindowInsets
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import org.json.JSONArray
import org.json.JSONObject
import java.io.BufferedInputStream
import java.io.BufferedOutputStream
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.security.MessageDigest
import java.time.Instant
import java.util.zip.ZipEntry
import java.util.zip.ZipOutputStream

/**
 * v0.49 software-artifact acquisition only.
 *
 * This activity inspects only the installed com.hihonor.camera package files that
 * PackageManager reports for this device. It does not open a camera, submit a capture,
 * invoke Honor Binder services, or treat APK content as sensor/capture evidence.
 */
class HonorCameraPackageExportActivity : Activity() {
    private lateinit var status: TextView
    private lateinit var saveJsonButton: Button
    private lateinit var exportBundleButton: Button

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        DrawVisualTheme.applyWindow(this)
        window.setDecorFitsSystemWindows(false)
        setContentView(buildUi())
        refreshStatus()
    }

    private fun buildUi(): View {
        val body = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(16), dp(12), dp(16), dp(20))
            setBackgroundColor(Color.rgb(12, 14, 18))
        }

        body.addView(label("TruthRaw v0.49 · Honor Camera package export", 22f, true))
        body.addView(label(
            "Leest en hasht uitsluitend de geïnstalleerde com.hihonor.camera APK-bestanden. " +
                "Doel: de Android-17 Honor Camera .706 software vergelijken met de bewaarde Android-16 .452 APK. " +
                "Geen camera-open, capture, pixeldata, Binder-call of vendor write.",
            12f, false, Color.rgb(190, 198, 210),
        ))

        body.addView(space(10))
        body.addView(button("1 · Inspecteer + hash Honor Camera package") { inspectPackage() })
        saveJsonButton = button("2 · JSON opslaan") { saveJson() }.apply { isEnabled = false }
        body.addView(saveJsonButton)
        exportBundleButton = button("3 · Bouw + sla APK-bundle ZIP op") { buildAndSaveBundle() }.apply { isEnabled = false }
        body.addView(exportBundleButton)

        body.addView(space(10))
        body.addView(label(
            "De ZIP bevat alleen base.apk en eventuele split APKs van com.hihonor.camera plus een manifest met hashes. " +
                "APK-inhoud blijft SOFTWARE_ARTIFACT_ONLY en krijgt geen TruthRaw sensor-/kalibratie-authority.",
            11f, false, Color.rgb(155, 165, 180),
        ))

        body.addView(space(10))
        status = label("Nog geen v0.49 package report.", 10f, false)
        body.addView(status)

        return ScrollView(this).apply {
            isFillViewport = true
            setBackgroundColor(Color.rgb(12, 14, 18))
            addView(body, ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT,
            ))
            setOnApplyWindowInsetsListener { view, insets ->
                val bars = insets.getInsets(WindowInsets.Type.systemBars())
                view.setPadding(bars.left, bars.top, bars.right, bars.bottom)
                insets
            }
        }
    }

    private fun inspectPackage() {
        saveJsonButton.isEnabled = false
        exportBundleButton.isEnabled = false
        status.text = "Inspecteren en SHA-256 berekenen…"

        Thread {
            val report = runCatching { buildPackageReport() }.getOrElse { e ->
                JSONObject()
                    .put("schema", SCHEMA)
                    .put("createdAtUtc", Instant.now().toString())
                    .put("authority", "SOFTWARE_PACKAGE_ARTIFACT_ONLY")
                    .put("cameraOpenedByTruthRaw", false)
                    .put("captureSubmittedByTruthRaw", false)
                    .put("honorBinderInvokedByTruthRaw", false)
                    .put("errorClass", e.javaClass.name)
                    .put("errorMessage", e.message ?: JSONObject.NULL)
            }
            reportFile().writeText(report.toString(2))
            runOnUiThread { refreshStatus() }
        }.start()
    }

    @Suppress("DEPRECATION")
    private fun buildPackageReport(): JSONObject {
        val info = packageManager.getPackageInfo(TARGET_PACKAGE, 0)
        val app = info.applicationInfo ?: error("applicationInfo ontbreekt")

        val files = mutableListOf<PackageFile>()
        files += PackageFile(
            role = "base",
            splitName = null,
            path = app.sourceDir,
        )

        val splitPaths = app.splitSourceDirs ?: emptyArray()
        val splitNames = info.splitNames ?: emptyArray()
        splitPaths.forEachIndexed { index, path ->
            files += PackageFile(
                role = "split",
                splitName = splitNames.getOrNull(index),
                path = path,
            )
        }

        val fileResults = JSONArray()
        var allReadable = true
        var allHashed = true

        files.forEachIndexed { index, item ->
            val f = File(item.path)
            val out = JSONObject()
                .put("index", index)
                .put("role", item.role)
                .put("splitName", item.splitName ?: JSONObject.NULL)
                .put("sourcePath", item.path)
                .put("fileName", f.name)
                .put("exists", f.exists())
                .put("canRead", f.canRead())
                .put("bytes", if (f.exists()) f.length() else JSONObject.NULL)

            if (!f.exists() || !f.canRead()) {
                allReadable = false
                allHashed = false
                out.put("sha256", JSONObject.NULL)
                out.put("hashError", "FILE_NOT_READABLE")
            } else {
                runCatching { sha256(f) }
                    .onSuccess { out.put("sha256", it) }
                    .onFailure { e ->
                        allHashed = false
                        out.put("sha256", JSONObject.NULL)
                        out.put("hashError", "${e.javaClass.name}: ${e.message}")
                    }
            }
            fileResults.put(out)
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", "SOFTWARE_PACKAGE_ARTIFACT_ONLY")
            .put("captureEvidenceGranted", false)
            .put("calibrationAuthorityGranted", false)
            .put("scientificMasterModified", false)
            .put("cameraOpenedByTruthRaw", false)
            .put("captureSubmittedByTruthRaw", false)
            .put("imageBufferAccessedByTruthRaw", false)
            .put("vendorRequestWrittenByTruthRaw", false)
            .put("honorBinderInvokedByTruthRaw", false)
            .put("physicalFrameCount", 0)
            .put("independentEvidenceCount", 0)
            .put("device", JSONObject()
                .put("manufacturer", Build.MANUFACTURER)
                .put("model", Build.MODEL)
                .put("sdkInt", Build.VERSION.SDK_INT)
                .put("release", Build.VERSION.RELEASE)
                .put("fingerprint", Build.FINGERPRINT)
                .put("truthRawTargetSdk", applicationInfo.targetSdkVersion))
            .put("package", JSONObject()
                .put("packageName", info.packageName)
                .put("versionName", info.versionName ?: JSONObject.NULL)
                .put("longVersionCode", info.longVersionCode)
                .put("targetSdkVersion", app.targetSdkVersion)
                .put("minSdkVersion", app.minSdkVersion)
                .put("compileSdkVersion", app.compileSdkVersion)
                .put("systemApp", (app.flags and android.content.pm.ApplicationInfo.FLAG_SYSTEM) != 0)
                .put("updatedSystemApp", (app.flags and android.content.pm.ApplicationInfo.FLAG_UPDATED_SYSTEM_APP) != 0)
                .put("splitCount", splitPaths.size))
            .put("packageFiles", fileResults)
            .put("allPackageFilesReadable", allReadable)
            .put("allPackageFilesHashed", allHashed)
            .put("bundleExportEligible", allReadable && allHashed)
            .put("android16Reference", JSONObject()
                .put("versionName", "171.0.10.452")
                .put("sourceApkSha256", "3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52"))
            .put("classification", "INSTALLED_HONOR_CAMERA_PACKAGE_SOFTWARE_ARTIFACT_FINGERPRINT_ONLY")
            .put("boundary", "APK_BYTES_AND_STATIC_CODE_ARE_SOFTWARE_ROUTE_EVIDENCE_ONLY__NEVER_DIRECT_CFA_CAPTURE_OR_CALIBRATION_EVIDENCE")
    }

    private data class PackageFile(
        val role: String,
        val splitName: String?,
        val path: String,
    )

    private fun sha256(file: File): String {
        val digest = MessageDigest.getInstance("SHA-256")
        BufferedInputStream(FileInputStream(file), 1024 * 1024).use { input ->
            val buffer = ByteArray(1024 * 1024)
            while (true) {
                val n = input.read(buffer)
                if (n <= 0) break
                digest.update(buffer, 0, n)
            }
        }
        return digest.digest().joinToString("") { "%02x".format(it) }
    }

    private fun buildBundle(report: JSONObject): File {
        val packageObj = report.optJSONObject("package") ?: error("package ontbreekt")
        val version = packageObj.optString("versionName", "unknown").replace(Regex("[^A-Za-z0-9._-]"), "_")
        val output = File(filesDir, "TRUTHRAW_HONOR_CAMERA_PACKAGE_${version}_v049.zip")

        val fileArray = report.optJSONArray("packageFiles") ?: error("packageFiles ontbreekt")
        ZipOutputStream(BufferedOutputStream(FileOutputStream(output), 1024 * 1024)).use { zip ->
            for (i in 0 until fileArray.length()) {
                val item = fileArray.getJSONObject(i)
                val path = item.getString("sourcePath")
                val role = item.getString("role")
                val splitName = item.optString("splitName", "")
                val source = File(path)
                if (!source.exists() || !source.canRead()) {
                    error("Package file niet leesbaar: $path")
                }

                val safeSplit = splitName.replace(Regex("[^A-Za-z0-9._-]"), "_")
                val entryName = if (role == "base") {
                    "base.apk"
                } else {
                    "splits/${i.toString().padStart(2, '0')}_${if (safeSplit.isBlank()) source.name else safeSplit}.apk"
                }

                zip.putNextEntry(ZipEntry(entryName))
                BufferedInputStream(FileInputStream(source), 1024 * 1024).use { input ->
                    val buffer = ByteArray(1024 * 1024)
                    while (true) {
                        val n = input.read(buffer)
                        if (n <= 0) break
                        zip.write(buffer, 0, n)
                    }
                }
                zip.closeEntry()
            }

            zip.putNextEntry(ZipEntry("truthraw_package_manifest.json"))
            zip.write(report.toString(2).toByteArray(Charsets.UTF_8))
            zip.closeEntry()
        }
        return output
    }

    private fun buildAndSaveBundle() {
        val report = loadReport() ?: return
        if (!report.optBoolean("bundleExportEligible", false)) {
            status.text = "Bundle-export niet mogelijk: minstens één packagebestand is niet leesbaar/hashbaar."
            return
        }

        exportBundleButton.isEnabled = false
        status.text = "APK-bundle wordt lokaal opgebouwd…"
        Thread {
            runCatching { buildBundle(report) }
                .onSuccess { bundle ->
                    runOnUiThread {
                        exportBundleButton.isEnabled = true
                        pendingBundle = bundle
                        startActivityForResult(
                            Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
                                addCategory(Intent.CATEGORY_OPENABLE)
                                type = "application/zip"
                                putExtra(Intent.EXTRA_TITLE, bundle.name)
                            },
                            REQUEST_SAVE_BUNDLE,
                        )
                    }
                }
                .onFailure { e ->
                    runOnUiThread {
                        exportBundleButton.isEnabled = true
                        status.text = "Bundle bouwen faalde: ${e.javaClass.simpleName}: ${e.message}"
                    }
                }
        }.start()
    }

    @Suppress("DEPRECATION")
    private fun saveJson() {
        val source = reportFile()
        if (!source.exists()) return
        startActivityForResult(
            Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
                addCategory(Intent.CATEGORY_OPENABLE)
                type = "application/json"
                putExtra(Intent.EXTRA_TITLE, source.name)
            },
            REQUEST_SAVE_JSON,
        )
    }

    @Deprecated("Document export bridge")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (resultCode != RESULT_OK) return
        val uri = data?.data ?: return

        val source = when (requestCode) {
            REQUEST_SAVE_JSON -> reportFile()
            REQUEST_SAVE_BUNDLE -> pendingBundle
            else -> null
        } ?: return

        runCatching {
            contentResolver.openOutputStream(uri)?.use { out ->
                BufferedInputStream(FileInputStream(source), 1024 * 1024).use { input ->
                    val buffer = ByteArray(1024 * 1024)
                    while (true) {
                        val n = input.read(buffer)
                        if (n <= 0) break
                        out.write(buffer, 0, n)
                    }
                }
            } ?: error("Geen output stream")
        }.onSuccess {
            status.text = if (requestCode == REQUEST_SAVE_BUNDLE) {
                "Honor Camera APK-bundle opgeslagen. Upload de ZIP naar deze chat."
            } else {
                "v0.49 JSON opgeslagen."
            }
        }.onFailure {
            status.text = "Opslaan faalde: ${it.javaClass.simpleName}: ${it.message}"
        }
    }

    private fun loadReport(): JSONObject? =
        runCatching { JSONObject(reportFile().readText()) }.getOrNull()

    private fun refreshStatus() {
        val report = loadReport()
        if (report == null) {
            saveJsonButton.isEnabled = false
            exportBundleButton.isEnabled = false
            status.text = "Nog geen v0.49 package report."
            return
        }

        saveJsonButton.isEnabled = true
        exportBundleButton.isEnabled = report.optBoolean("bundleExportEligible", false)

        val pkg = report.optJSONObject("package")
        val files = report.optJSONArray("packageFiles")
        status.text = buildString {
            append("Honor Camera=").append(pkg?.optString("versionName", "?")).append('\n')
            append("targetSdk=").append(pkg?.optInt("targetSdkVersion", -1)).append('\n')
            append("packageFiles=").append(files?.length() ?: 0).append('\n')
            append("allReadable=").append(report.optBoolean("allPackageFilesReadable", false)).append('\n')
            append("allHashed=").append(report.optBoolean("allPackageFilesHashed", false)).append('\n')
            append("authority=").append(report.optString("authority", "?"))
        }
    }

    private fun reportFile(): File = File(filesDir, REPORT_FILENAME)

    private fun button(text: String, action: () -> Unit): Button =
        Button(this).apply {
            this.text = text
            isAllCaps = false
            minHeight = dp(52)
            setOnClickListener { action() }
        }

    private fun label(text: String, size: Float, bold: Boolean, color: Int = Color.WHITE): TextView =
        TextView(this).apply {
            this.text = text
            textSize = size
            setTextColor(color)
            if (bold) setTypeface(typeface, android.graphics.Typeface.BOLD)
        }

    private fun space(height: Int): View =
        View(this).apply { layoutParams = LinearLayout.LayoutParams(1, dp(height)) }

    private fun dp(v: Int): Int = (v * resources.displayMetrics.density).toInt()

    companion object {
        private const val TARGET_PACKAGE = "com.hihonor.camera"
        private const val REPORT_FILENAME = "TRUTHRAW_HONOR_CAMERA_PACKAGE_FINGERPRINT_v049.json"
        private const val SCHEMA = "truthraw.honor-camera-package-export.v0.49"
        private const val REQUEST_SAVE_JSON = 64901
        private const val REQUEST_SAVE_BUNDLE = 64902
        private var pendingBundle: File? = null
    }
}
