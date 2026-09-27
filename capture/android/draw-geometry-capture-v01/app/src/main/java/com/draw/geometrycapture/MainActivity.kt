package com.draw.geometrycapture

import android.app.Activity
import android.app.AlertDialog
import android.content.Intent
import android.graphics.Typeface
import android.net.Uri
import android.os.Bundle
import android.os.Environment
import android.provider.MediaStore
import android.provider.OpenableColumns
import android.text.Editable
import android.text.TextWatcher
import android.view.Gravity
import android.view.View
import android.widget.Button
import android.widget.CheckBox
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import android.widget.Toast
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.security.MessageDigest
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

class MainActivity : Activity() {

    companion object {
        private const val REQ_IMPORT_MAIN = 1001
        private const val REQ_IMPORT_WIDE = 1002
        private const val REQ_EXPORT_MANIFEST = 1003
        private const val REQ_RECOVER_BATCH = 1004
        private const val TOTAL_POSES = 16
        private const val TRAINING_POSES = 12
        private const val PREFS = "draw_geometry_capture_v01"
    }

    private val prefs by lazy { getSharedPreferences(PREFS, MODE_PRIVATE) }

    private lateinit var scrollView: ScrollView
    private lateinit var transitionBanner: TextView
    private lateinit var poseLabel: TextView
    private lateinit var progressLabel: TextView
    private lateinit var mainStatus: TextView
    private lateinit var wideStatus: TextView
    private lateinit var pairStatus: TextView
    private lateinit var cameraRigid: CheckBox
    private lateinit var targetStatic: CheckBox
    private lateinit var targetFamily: EditText
    private lateinit var targetSpacingMm: EditText
    private lateinit var targetSha256: EditText
    private lateinit var completeButton: Button
    private lateinit var prevButton: Button
    private lateinit var nextButton: Button

    private var currentPoseIndex: Int = 0

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        ensureSession()
        currentPoseIndex = prefs.getInt("currentPoseIndex", 0).coerceIn(0, TOTAL_POSES - 1)
        setContentView(buildUi())
        loadTargetFields()
        renderPose()
    }

    private fun ensureSession() {
        if (!prefs.contains("sessionId")) {
            val id = SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US).format(Date())
            prefs.edit()
                .putString("sessionId", id)
                .putLong("sessionCreatedAtEpochMs", System.currentTimeMillis())
                .apply()
        }
    }

    private fun buildUi(): View {
        scrollView = ScrollView(this)
        val scroll = scrollView
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(18), dp(18), dp(18), dp(32))
        }
        scroll.addView(root)

        root.addView(TextView(this).apply {
            text = "D.RAW Geometry Capture"
            textSize = 26f
            setTypeface(typeface, Typeface.BOLD)
        })
        root.addView(TextView(this).apply {
            text = "Camera2-onafhankelijke capture-assistent • originele RAW/DNG blijft evidence"
            textSize = 14f
            setPadding(0, dp(4), 0, dp(16))
        })

        progressLabel = TextView(this).apply {
            textSize = 15f
            setTypeface(typeface, Typeface.BOLD)
        }
        root.addView(progressLabel)

        poseLabel = TextView(this).apply {
            textSize = 22f
            setTypeface(typeface, Typeface.BOLD)
            setPadding(0, dp(8), 0, dp(8))
        }
        root.addView(poseLabel)

        transitionBanner = TextView(this).apply {
            textSize = 16f
            setTypeface(typeface, Typeface.BOLD)
            setPadding(dp(12), dp(12), dp(12), dp(12))
            setBackgroundColor(0xFFE2F4E8.toInt())
            visibility = View.GONE
        }
        root.addView(transitionBanner)

        root.addView(infoBox(
            "Per pose:\n" +
                "1. Zet telefoon én target stil.\n" +
                "2. Maak MAIN RAW/DNG op 1×.\n" +
                "3. Beweeg niets en maak ULTRA-WIDE RAW/DNG op 0,6×.\n" +
                "4. Importeer beide DNG’s hieronder.\n" +
                "5. Bevestig pas daarna het paar.\n\n" +
                "De APK meet geen geometrie en kent geen relation-authority toe."
        ))

        root.addView(sectionTitle("Universele bron / sessie"))
        root.addView(TextView(this).apply {
            text = "Session ID: " + sessionId() + "\n" +
                "D.RAW leest bronmetadata en de zichtbare voorkant automatisch. " +
                "Handmatige target- of toestelprofielen zijn niet vereist."
            textSize = 13f
        })

        targetFamily = EditText(this).apply {
            hint = "Target family / naam (bv. CHARUCO_8x11)"
            setSingleLine(true)
        }
        targetFamily.visibility = View.GONE

        targetSpacingMm = EditText(this).apply {
            hint = "Fysieke spacing in mm (bv. 20.0)"
            inputType = android.text.InputType.TYPE_CLASS_NUMBER or
                android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL
            setSingleLine(true)
        }
        targetSpacingMm.visibility = View.GONE

        targetSha256 = EditText(this).apply {
            hint = "Target geometry SHA-256 (mag tijdens capture nog leeg zijn)"
            setSingleLine(true)
        }
        targetSha256.visibility = View.GONE

        val watcher = object : TextWatcher {
            override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) {}
            override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) {
                saveTargetFields()
            }
            override fun afterTextChanged(s: Editable?) {}
        }
        targetFamily.addTextChangedListener(watcher)
        targetSpacingMm.addTextChangedListener(watcher)
        targetSha256.addTextChangedListener(watcher)

        root.addView(sectionTitle("Bron A • workflowlabel MAIN/1×"))
        root.addView(Button(this).apply {
            text = "1. Open camera voor MAIN"
            setOnClickListener {
                toast("Maak nu 1× MAIN als originele RAW/DNG. Kom daarna terug zonder telefoon of target te bewegen.")
                openExternalCamera()
            }
        })
        root.addView(Button(this).apply {
            text = "2. Importeer MAIN RAW/DNG"
            setOnClickListener { importDng(REQ_IMPORT_MAIN) }
        })
        mainStatus = TextView(this).apply {
            textSize = 13f
            setPadding(0, dp(4), 0, dp(12))
        }
        root.addView(mainStatus)

        root.addView(sectionTitle("Bron B • workflowlabel ULTRA-WIDE/0,6×"))
        root.addView(Button(this).apply {
            text = "3. Open camera voor ULTRA-WIDE"
            setOnClickListener {
                toast("Beweeg telefoon en target niet. Maak nu 0,6× ULTRA-WIDE als originele RAW/DNG.")
                openExternalCamera()
            }
        })
        root.addView(Button(this).apply {
            text = "4. Importeer ULTRA-WIDE RAW/DNG"
            setOnClickListener { importDng(REQ_IMPORT_WIDE) }
        })
        wideStatus = TextView(this).apply {
            textSize = 13f
            setPadding(0, dp(4), 0, dp(12))
        }
        root.addView(wideStatus)

        root.addView(sectionTitle("Pair-attestatie"))
        cameraRigid = CheckBox(this).apply {
            text = "Optioneel: telefoon/camerasysteem is tussen de twee bronnen niet bewogen"
            setOnCheckedChangeListener { _, _ -> persistAttestation() }
        }
        root.addView(cameraRigid)

        targetStatic = CheckBox(this).apply {
            text = "Optioneel: scène/target is tussen de twee bronnen niet bewogen"
            setOnCheckedChangeListener { _, _ -> persistAttestation() }
        }
        root.addView(targetStatic)

        pairStatus = TextView(this).apply {
            textSize = 14f
            setPadding(0, dp(10), 0, dp(8))
        }
        root.addView(pairStatus)

        completeButton = Button(this).apply {
            text = "5. Bevestig dit pose-paar"
            setOnClickListener { completeCurrentPair() }
        }
        root.addView(completeButton)

        val nav = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER
        }
        prevButton = Button(this).apply {
            text = "← Vorige"
            setOnClickListener { movePose(-1) }
        }
        nextButton = Button(this).apply {
            text = "Volgende →"
            setOnClickListener { movePose(1) }
        }
        nav.addView(prevButton, LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f))
        nav.addView(nextButton, LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f))
        root.addView(nav)

        root.addView(sectionTitle("Sessie"))
        root.addView(Button(this).apply {
            text = "Bestaande 32 RAW/DNG's automatisch herstellen"
            setOnClickListener { launchBatchRecovery() }
        })
        root.addView(TextView(this).apply {
            text = "Voor een gewiste vorige app: selecteer alle 32 originele opnames tegelijk. " +
                "D.RAW leest capturetijd, focal length, bronmetadata en voorkant en bouwt de 16 paren opnieuw op."
            textSize = 13f
            setPadding(0, dp(4), 0, dp(12))
        })
        root.addView(Button(this).apply {
            text = "Exporteer session-manifest JSON"
            setOnClickListener { exportManifest() }
        })
        root.addView(Button(this).apply {
            text = "Nieuwe / lege sessie"
            setOnClickListener { confirmReset() }
        })

        root.addView(infoBox(
            "De app bewaart byte-identieke kopieën van de geselecteerde RAW/DNG-bestanden " +
                "in zijn eigen sessiemap en verifieert SHA-256 na het kopiëren. " +
                "Dit creëert geen nieuwe sensor-evidence en wijzigt de bron niet.\n\n" +
                "Open-world regel: Seal the evidence, not the thinking."
        ))
        return scroll
    }

    private fun sectionTitle(value: String): TextView = TextView(this).apply {
        text = value
        textSize = 18f
        setTypeface(typeface, Typeface.BOLD)
        setPadding(0, dp(18), 0, dp(6))
    }

    private fun infoBox(value: String): TextView = TextView(this).apply {
        text = value
        textSize = 14f
        setPadding(dp(12), dp(12), dp(12), dp(12))
        setBackgroundColor(0xFFEFEFEF.toInt())
    }

    private fun sessionId(): String = prefs.getString("sessionId", "UNKNOWN") ?: "UNKNOWN"

    private fun subsetFor(index: Int): String = if (index < TRAINING_POSES) "TRAINING" else "HOLDOUT"

    private fun poseId(index: Int): String {
        return if (index < TRAINING_POSES) {
            "TRAIN_%02d".format(Locale.US, index + 1)
        } else {
            "HOLD_%02d".format(Locale.US, index - TRAINING_POSES + 1)
        }
    }

    private fun sourceKey(index: Int, role: String): String {
        return "pose_" + index + "_" + role.lowercase(Locale.US)
    }

    private fun completeKey(index: Int): String = "pose_" + index + "_complete"
    private fun rigidKey(index: Int): String = "pose_" + index + "_camera_rigid"
    private fun targetStaticKey(index: Int): String = "pose_" + index + "_target_static"
    private fun recoveryKey(index: Int): String = "pose_" + index + "_recovery_assignment"

    private fun renderPose() {
        val subset = subsetFor(currentPoseIndex)
        val completed = countCompleted()
        progressLabel.text = "Voortgang: " + completed + " / " + TOTAL_POSES + " pose-paren compleet"
        poseLabel.text = "Pose " + (currentPoseIndex + 1) + " / " + TOTAL_POSES +
            " • " + subset + " • " + poseId(currentPoseIndex)

        val transitionMessage = prefs.getString("lastActionMessage", "") ?: ""
        if (transitionMessage.isBlank()) {
            transitionBanner.visibility = View.GONE
        } else {
            transitionBanner.text = transitionMessage
            transitionBanner.visibility = View.VISIBLE
        }

        val main = loadSource(currentPoseIndex, "MAIN")
        val wide = loadSource(currentPoseIndex, "ULTRA_WIDE")
        mainStatus.text = sourceStatus(main)
        wideStatus.text = sourceStatus(wide)

        cameraRigid.setOnCheckedChangeListener(null)
        targetStatic.setOnCheckedChangeListener(null)
        cameraRigid.isChecked = prefs.getBoolean(rigidKey(currentPoseIndex), false)
        targetStatic.isChecked = prefs.getBoolean(targetStaticKey(currentPoseIndex), false)
        cameraRigid.setOnCheckedChangeListener { _, _ -> persistAttestation() }
        targetStatic.setOnCheckedChangeListener { _, _ -> persistAttestation() }

        val isComplete = prefs.getBoolean(completeKey(currentPoseIndex), false)
        pairStatus.text = if (isComplete) {
            "✓ Pose-paar bevestigd. Dit is capture-provenance, nog géén geometry-relation."
        } else {
            "Nog niet bevestigd. Beide originele bestanden zijn vereist; attestaties zijn extra provenance."
        }

        prevButton.isEnabled = currentPoseIndex > 0
        nextButton.isEnabled = currentPoseIndex < TOTAL_POSES - 1
        completeButton.isEnabled = !isComplete
        prefs.edit().putInt("currentPoseIndex", currentPoseIndex).apply()
    }

    private fun sourceStatus(src: ImportedSource?): String {
        if (src == null) return "Nog geen RAW/DNG geïmporteerd."
        val profile = src.universalSourceProfile
        val metadata = profile.optJSONObject("source_metadata") ?: JSONObject()
        val raster = profile.optJSONObject("primary_raw_raster") ?: JSONObject()
        val optics = profile.optJSONObject("optics") ?: JSONObject()
        val scene = profile.optJSONObject("scene_analysis") ?: JSONObject()

        val make = metadata.optString("make", "")
            .takeIf { it.isNotBlank() && it != "null" }
        val model = metadata.optString("model", "")
            .takeIf { it.isNotBlank() && it != "null" }
        val focal = optics.optDouble("focal_length_mm", Double.NaN)
        val width = raster.optLong("width", -1L)
        val height = raster.optLong("height", -1L)
        val frontside = scene.optString("status", "UNKNOWN")

        val auto = buildString {
            append("\nauto: ")
            append(profile.optString("scientific_source_class", "UNKNOWN"))
            if (make != null || model != null) {
                append(" • ")
                append(listOfNotNull(make, model).joinToString(" "))
            }
            if (focal.isFinite()) {
                append(" • ")
                append("%.3f mm".format(Locale.US, focal))
            }
            if (width > 0 && height > 0) {
                append(" • ")
                append(width)
                append("×")
                append(height)
            }
            append("\nvoorkant: ")
            append(frontside)
        }

        return "✓ " + src.displayName + "\n" +
            "bytes: " + src.byteLength + "\n" +
            "SHA-256: " + src.sha256 + "\n" +
            "kopie-hash gelijk: " + src.copyVerified + auto
    }

    private fun persistAttestation() {
        prefs.edit()
            .putBoolean(rigidKey(currentPoseIndex), cameraRigid.isChecked)
            .putBoolean(targetStaticKey(currentPoseIndex), targetStatic.isChecked)
            .apply()
    }

    private fun openExternalCamera() {
        val intent = Intent(MediaStore.INTENT_ACTION_STILL_IMAGE_CAMERA)
        try {
            startActivity(intent)
        } catch (_: Exception) {
            toast("Geen standaard camera-app gevonden. Open je RAW-camera handmatig.")
        }
    }

    private fun launchBatchRecovery() {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "*/*"
            putExtra(Intent.EXTRA_ALLOW_MULTIPLE, true)
            putExtra(
                Intent.EXTRA_MIME_TYPES,
                arrayOf(
                    "image/x-adobe-dng",
                    "image/dng",
                    "application/octet-stream",
                    "image/*"
                )
            )
            addFlags(
                Intent.FLAG_GRANT_READ_URI_PERMISSION or
                    Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION
            )
        }
        startActivityForResult(intent, REQ_RECOVER_BATCH)
    }

    private fun importDng(requestCode: Int) {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "*/*"
            putExtra(
                Intent.EXTRA_MIME_TYPES,
                arrayOf(
                    "image/x-adobe-dng",
                    "image/dng",
                    "application/octet-stream",
                    "image/*"
                )
            )
            addFlags(
                Intent.FLAG_GRANT_READ_URI_PERMISSION or
                    Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION
            )
        }
        startActivityForResult(intent, requestCode)
    }

    @Deprecated("Standalone capture assistant uses the platform result API")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)

        if (requestCode == REQ_EXPORT_MANIFEST && resultCode == RESULT_OK) {
            val uri = data?.data ?: return
            writeManifestToUri(uri)
            return
        }

        if (requestCode == REQ_RECOVER_BATCH && resultCode == RESULT_OK) {
            val uris = mutableListOf<Uri>()
            val clip = data?.clipData
            if (clip != null) {
                for (i in 0 until clip.itemCount) {
                    uris += clip.getItemAt(i).uri
                }
            } else {
                data?.data?.let { uris += it }
            }
            if (uris.isNotEmpty()) {
                recoverExistingShoot(uris)
            }
            return
        }

        if (resultCode != RESULT_OK) return
        val uri = data?.data ?: return

        val role = when (requestCode) {
            REQ_IMPORT_MAIN -> "MAIN"
            REQ_IMPORT_WIDE -> "ULTRA_WIDE"
            else -> return
        }

        try {
            try {
                contentResolver.takePersistableUriPermission(
                    uri,
                    Intent.FLAG_GRANT_READ_URI_PERMISSION
                )
            } catch (_: Exception) {
            }

            val imported = copyAndVerify(uri, role)
            if (hashAlreadyUsedElsewhere(imported.sha256, currentPoseIndex, role)) {
                File(imported.localCopyPath).delete()
                toast("Dit bestand/hash is al in een ander pose-slot gebruikt. Import geweigerd.")
                return
            }

            saveSource(currentPoseIndex, role, imported)
            prefs.edit().putBoolean(completeKey(currentPoseIndex), false).apply()
            renderPose()
            toast(role + " RAW/DNG geïmporteerd en byte-identiek geverifieerd.")
        } catch (e: Exception) {
            toast("Import mislukt: " + (e.message ?: e.javaClass.simpleName))
        }
    }

    private data class RecoveryCandidate(
        val source: ImportedSource,
        val captureEpochMs: Long?,
        val timeAuthority: String,
        val focalLengthMm: Double?,
        val originalSelectionIndex: Int
    )

    private fun recoverExistingShoot(uris: List<Uri>) {
        if (uris.size != 32) {
            AlertDialog.Builder(this)
                .setTitle("Selecteer precies 32 bestanden")
                .setMessage(
                    "Voor herstel van de eerdere 16-pose sessie verwacht D.RAW 32 originele RAW/DNG-bestanden. " +
                        "Geselecteerd: " + uris.size + "."
                )
                .setPositiveButton("OK", null)
                .show()
            return
        }

        transitionBanner.text = "32 bronnen worden gelezen, gehasht en visueel geïnspecteerd…"
        transitionBanner.visibility = View.VISIBLE

        Thread {
            try {
                val recoveryId = SimpleDateFormat(
                    "yyyyMMdd_HHmmss",
                    Locale.US
                ).format(Date())
                val candidates = mutableListOf<RecoveryCandidate>()

                uris.forEachIndexed { index, uri ->
                    try {
                        contentResolver.takePersistableUriPermission(
                            uri,
                            Intent.FLAG_GRANT_READ_URI_PERMISSION
                        )
                    } catch (_: Exception) {
                    }

                    val src = copyAndVerifyRecovery(
                        uri = uri,
                        recoveryId = recoveryId,
                        recoveryIndex = index
                    )
                    val timing = preferredCaptureEpochMs(src)
                    val focal = src.universalSourceProfile
                        .optJSONObject("optics")
                        ?.optDouble("focal_length_mm", Double.NaN)
                        ?.takeIf { it.isFinite() && it > 0.0 }

                    candidates += RecoveryCandidate(
                        source = src,
                        captureEpochMs = timing.first,
                        timeAuthority = timing.second,
                        focalLengthMm = focal,
                        originalSelectionIndex = index
                    )

                    runOnUiThread {
                        transitionBanner.text =
                            "Herstel: " + (index + 1) + " / 32 bronnen geanalyseerd…"
                    }
                }

                val sorted = candidates.sortedWith(
                    compareBy<RecoveryCandidate>(
                        { it.captureEpochMs ?: Long.MAX_VALUE },
                        { it.originalSelectionIndex }
                    )
                )

                val allHaveMeasuredTime = sorted.all { it.captureEpochMs != null }
                val pairs = sorted.chunked(2)
                if (pairs.size != TOTAL_POSES || pairs.any { it.size != 2 }) {
                    throw IllegalStateException("Kon geen 16 bronparen vormen")
                }

                val editor = prefs.edit().clear()
                    .putString("sessionId", recoveryId)
                    .putLong("sessionCreatedAtEpochMs", System.currentTimeMillis())
                    .putString(
                        "sessionOrigin",
                        "RECOVERED_EXISTING_SOURCES_AUTO_PAIRED"
                    )
                    .putString(
                        "recoveryOrderingAuthority",
                        if (allHaveMeasuredTime) {
                            "SOURCE_METADATA_CAPTURE_TIME"
                        } else {
                            "MIXED_CAPTURE_TIME_WITH_SELECTION_ORDER_FALLBACK"
                        }
                    )

                pairs.forEachIndexed { poseIndex, pair ->
                    val first = pair[0]
                    val second = pair[1]
                    val roleAssigned = assignWorkflowRoles(first, second)
                    val main = roleAssigned.first
                    val wide = roleAssigned.second
                    val assignmentAuthority = roleAssigned.third

                    editor.putString(
                        sourceKey(poseIndex, "MAIN"),
                        main.source.copy(role = "MAIN").toJson().toString()
                    )
                    editor.putString(
                        sourceKey(poseIndex, "ULTRA_WIDE"),
                        wide.source.copy(role = "ULTRA_WIDE").toJson().toString()
                    )
                    editor.putBoolean(completeKey(poseIndex), true)
                    editor.putBoolean(rigidKey(poseIndex), false)
                    editor.putBoolean(targetStaticKey(poseIndex), false)

                    val deltaMs = if (
                        first.captureEpochMs != null &&
                        second.captureEpochMs != null
                    ) {
                        kotlin.math.abs(
                            first.captureEpochMs - second.captureEpochMs
                        )
                    } else {
                        null
                    }

                    val recoveryInfo = JSONObject()
                        .put(
                            "schema",
                            "D.RAW/RecoveredPairAssignment/0.1"
                        )
                        .put(
                            "pairing_method",
                            if (
                                first.captureEpochMs != null &&
                                second.captureEpochMs != null
                            ) {
                                "CHRONOLOGICAL_ADJACENT_SOURCE_METADATA"
                            } else {
                                "CHRONOLOGICAL_WITH_SELECTION_ORDER_FALLBACK"
                            }
                        )
                        .put(
                            "pairing_authority",
                            "RECOVERY_INFERENCE_NOT_GEOMETRY_EVIDENCE"
                        )
                        .put(
                            "workflow_role_assignment",
                            assignmentAuthority
                        )
                        .put(
                            "capture_delta_ms",
                            deltaMs ?: JSONObject.NULL
                        )
                        .put(
                            "first_time_authority",
                            first.timeAuthority
                        )
                        .put(
                            "second_time_authority",
                            second.timeAuthority
                        )
                        .put(
                            "first_reported_focal_length_mm",
                            first.focalLengthMm ?: JSONObject.NULL
                        )
                        .put(
                            "second_reported_focal_length_mm",
                            second.focalLengthMm ?: JSONObject.NULL
                        )
                        .put(
                            "workflow_labels_are_scientific_authority",
                            false
                        )
                        .put(
                            "geometry_relation_granted",
                            false
                        )

                    editor.putString(
                        recoveryKey(poseIndex),
                        recoveryInfo.toString()
                    )
                }

                editor.putInt("currentPoseIndex", 0)
                    .putString(
                        "lastActionMessage",
                        "✓ 32 bestaande bronnen automatisch hersteld tot 16 kandidaatparen. Controleer het manifest; er is nog geen geometry-relation admitted."
                    )
                    .apply()

                runOnUiThread {
                    currentPoseIndex = 0
                    loadTargetFields()
                    renderPose()
                    scrollView.post { scrollView.smoothScrollTo(0, 0) }
                    AlertDialog.Builder(this)
                        .setTitle("Herstel voltooid")
                        .setMessage(
                            "32 bronnen zijn automatisch gelezen en opnieuw als 16 pose-paren ingedeeld. " +
                                "De koppeling is als recovery-inference opgeslagen, niet als geometry-evidence. " +
                                "Exporteer nu het session-manifest JSON."
                        )
                        .setPositiveButton("OK", null)
                        .show()
                }
            } catch (e: Exception) {
                runOnUiThread {
                    transitionBanner.text =
                        "Herstel mislukt: " +
                            (e.message ?: e.javaClass.simpleName)
                    transitionBanner.visibility = View.VISIBLE
                    toast(
                        "Herstel mislukt: " +
                            (e.message ?: e.javaClass.simpleName)
                    )
                }
            }
        }.start()
    }

    private fun assignWorkflowRoles(
        first: RecoveryCandidate,
        second: RecoveryCandidate
    ): Triple<RecoveryCandidate, RecoveryCandidate, String> {
        val fa = first.focalLengthMm
        val fb = second.focalLengthMm
        if (
            fa != null &&
            fb != null &&
            fa > 0.0 &&
            fb > 0.0 &&
            kotlin.math.abs(fa - fb) /
                kotlin.math.max(fa, fb) >= 0.05
        ) {
            return if (fa > fb) {
                Triple(
                    first,
                    second,
                    "SOURCE_METADATA_FOCAL_ORDERING_WORKFLOW_HINT"
                )
            } else {
                Triple(
                    second,
                    first,
                    "SOURCE_METADATA_FOCAL_ORDERING_WORKFLOW_HINT"
                )
            }
        }

        return Triple(
            first,
            second,
            "CAPTURE_ORDER_WORKFLOW_FALLBACK"
        )
    }

    private fun preferredCaptureEpochMs(
        source: ImportedSource
    ): Pair<Long?, String> {
        val metadata = source.universalSourceProfile
            .optJSONObject("source_metadata")
            ?: JSONObject()

        val text = metadata
            .optString("capture_time_preferred_text", "")
            .trim()
        val subsec = metadata
            .optString("capture_subsec_preferred_text", "")
            .trim()

        parseExifTime(text, subsec)?.let {
            return it to "SOURCE_METADATA_CAPTURE_TIME"
        }

        parseFilenameTime(source.displayName)?.let {
            return it to "FILENAME_TIME_FALLBACK"
        }

        return null to "SELECTION_ORDER_FALLBACK"
    }

    private fun parseExifTime(
        text: String,
        subsec: String
    ): Long? {
        if (text.isBlank() || text == "null") return null
        return try {
            val parser = SimpleDateFormat(
                "yyyy:MM:dd HH:mm:ss",
                Locale.US
            )
            parser.isLenient = false
            val base = parser.parse(text)?.time ?: return null
            val digits = subsec.filter { it.isDigit() }
            val ms = when {
                digits.isEmpty() -> 0L
                digits.length == 1 -> digits.toLong() * 100L
                digits.length == 2 -> digits.toLong() * 10L
                else -> digits.take(3).toLong()
            }
            base + ms
        } catch (_: Exception) {
            null
        }
    }

    private fun parseFilenameTime(name: String): Long? {
        val candidates = listOf(
            Regex("(20\\d{2})(\\d{2})(\\d{2})[_-]?(\\d{2})(\\d{2})(\\d{2})"),
            Regex("(20\\d{2})[-_](\\d{2})[-_](\\d{2})[ T_-](\\d{2})[:_-]?(\\d{2})[:_-]?(\\d{2})")
        )
        for (regex in candidates) {
            val match = regex.find(name) ?: continue
            val values = match.groupValues
            if (values.size < 7) continue
            val normalized = values.drop(1).take(6).joinToString("")
            try {
                val parser = SimpleDateFormat(
                    "yyyyMMddHHmmss",
                    Locale.US
                )
                parser.isLenient = false
                return parser.parse(normalized)?.time
            } catch (_: Exception) {
            }
        }
        return null
    }

    private fun copyAndVerifyRecovery(
        uri: Uri,
        recoveryId: String,
        recoveryIndex: Int
    ): ImportedSource {
        val display = queryDisplayName(uri)
            ?: ("source_" + recoveryIndex + ".dng")
        val lower = display.lowercase(Locale.US)
        val allowed = listOf(
            ".dng", ".raw", ".nef", ".arw",
            ".cr2", ".cr3", ".rw2", ".orf"
        )
        if (allowed.none { lower.endsWith(it) }) {
            throw IllegalArgumentException(
                "Geen ondersteund RAW/DNG-bestand: " + display
            )
        }

        val base = getExternalFilesDir(
            Environment.DIRECTORY_DOCUMENTS
        ) ?: throw IllegalStateException(
            "Geen externe app-documentmap beschikbaar"
        )
        val dir = File(
            base,
            "D_RAW_Recovery_" + recoveryId + "/RECOVERY"
        )
        if (!dir.exists() && !dir.mkdirs()) {
            throw IllegalStateException(
                "Kan recovery-map niet maken"
            )
        }

        val safe = display.replace(
            Regex("[^A-Za-z0-9._-]"),
            "_"
        )
        val outFile = File(
            dir,
            "%02d_%s".format(
                Locale.US,
                recoveryIndex + 1,
                safe
            )
        )

        val digest = MessageDigest.getInstance("SHA-256")
        var count = 0L
        contentResolver.openInputStream(uri).use { input ->
            requireNotNull(input) {
                "Bronbestand kan niet worden geopend"
            }
            FileOutputStream(outFile, false).use { output ->
                val buffer = ByteArray(1024 * 1024)
                while (true) {
                    val n = input.read(buffer)
                    if (n < 0) break
                    if (n == 0) continue
                    output.write(buffer, 0, n)
                    digest.update(buffer, 0, n)
                    count += n
                }
                output.fd.sync()
            }
        }

        if (count <= 0L) {
            outFile.delete()
            throw IllegalStateException("Leeg bronbestand")
        }

        val sourceHash = digest.digest().toHex()
        val copyHash = sha256(outFile)
        if (sourceHash != copyHash) {
            outFile.delete()
            throw IllegalStateException(
                "Byte-identieke recovery-kopiecontrole mislukt"
            )
        }

        val profile = UniversalSourceProfiler.profile(
            outFile,
            display,
            sourceHash
        )

        return ImportedSource(
            role = "UNASSIGNED_RECOVERY_SOURCE",
            displayName = display,
            sourceUri = uri.toString(),
            localCopyPath = outFile.absolutePath,
            byteLength = count,
            sha256 = sourceHash,
            copiedSha256 = copyHash,
            copyVerified = true,
            importedAtEpochMs = System.currentTimeMillis(),
            universalSourceProfile = profile
        )
    }

    private fun copyAndVerify(uri: Uri, role: String): ImportedSource {
        val display = queryDisplayName(uri) ?: ("source_" + System.currentTimeMillis() + ".dng")
        val lower = display.lowercase(Locale.US)
        val allowed = listOf(".dng", ".raw", ".nef", ".arw", ".cr2", ".cr3", ".rw2", ".orf")
        if (allowed.none { lower.endsWith(it) }) {
            throw IllegalArgumentException("Selecteer een RAW/DNG-bestand; ontvangen: " + display)
        }

        val base = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)
            ?: throw IllegalStateException("Geen externe app-documentmap beschikbaar")
        val dir = File(
            base,
            "D_RAW_Geometry_" + sessionId() + "/" +
                subsetFor(currentPoseIndex) + "/" + poseId(currentPoseIndex)
        )
        if (!dir.exists() && !dir.mkdirs()) {
            throw IllegalStateException("Kan sessiemap niet maken")
        }

        val safe = display.replace(Regex("[^A-Za-z0-9._-]"), "_")
        val outFile = File(dir, role + "_" + safe)

        val digest = MessageDigest.getInstance("SHA-256")
        var count = 0L
        contentResolver.openInputStream(uri).use { input ->
            requireNotNull(input) { "Bronbestand kan niet worden geopend" }
            FileOutputStream(outFile, false).use { output ->
                val buffer = ByteArray(1024 * 1024)
                while (true) {
                    val n = input.read(buffer)
                    if (n < 0) break
                    if (n == 0) continue
                    output.write(buffer, 0, n)
                    digest.update(buffer, 0, n)
                    count += n
                }
                output.fd.sync()
            }
        }

        if (count <= 0L) {
            outFile.delete()
            throw IllegalStateException("Leeg bronbestand")
        }

        val sourceHash = digest.digest().toHex()
        val copyHash = sha256(outFile)
        if (sourceHash != copyHash) {
            outFile.delete()
            throw IllegalStateException("Byte-identieke kopiecontrole mislukt")
        }

        val universalProfile = UniversalSourceProfiler.profile(
            outFile,
            display,
            sourceHash
        )

        return ImportedSource(
            role = role,
            displayName = display,
            sourceUri = uri.toString(),
            localCopyPath = outFile.absolutePath,
            byteLength = count,
            sha256 = sourceHash,
            copiedSha256 = copyHash,
            copyVerified = true,
            importedAtEpochMs = System.currentTimeMillis(),
            universalSourceProfile = universalProfile
        )
    }

    private fun queryDisplayName(uri: Uri): String? {
        contentResolver.query(
            uri,
            arrayOf(OpenableColumns.DISPLAY_NAME),
            null,
            null,
            null
        ).use { cursor ->
            if (cursor != null && cursor.moveToFirst()) {
                val index = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
                if (index >= 0) return cursor.getString(index)
            }
        }
        return uri.lastPathSegment
    }

    private fun sha256(file: File): String {
        val digest = MessageDigest.getInstance("SHA-256")
        FileInputStream(file).use { input ->
            val buffer = ByteArray(1024 * 1024)
            while (true) {
                val n = input.read(buffer)
                if (n < 0) break
                if (n == 0) continue
                digest.update(buffer, 0, n)
            }
        }
        return digest.digest().toHex()
    }

    private fun ByteArray.toHex(): String = joinToString("") { "%02x".format(it) }

    private fun saveSource(index: Int, role: String, src: ImportedSource) {
        prefs.edit()
            .putString(sourceKey(index, role), src.toJson().toString())
            .apply()
    }

    private fun loadSource(index: Int, role: String): ImportedSource? {
        val raw = prefs.getString(sourceKey(index, role), null) ?: return null
        return try {
            ImportedSource.fromJson(JSONObject(raw))
        } catch (_: Exception) {
            null
        }
    }

    private fun hashAlreadyUsedElsewhere(hash: String, current: Int, role: String): Boolean {
        for (i in 0 until TOTAL_POSES) {
            for (r in listOf("MAIN", "ULTRA_WIDE")) {
                if (i == current && r == role) continue
                if (loadSource(i, r)?.sha256 == hash) return true
            }
        }
        return false
    }

    private fun completeCurrentPair() {
        val main = loadSource(currentPoseIndex, "MAIN")
        val wide = loadSource(currentPoseIndex, "ULTRA_WIDE")
        if (main == null || wide == null) {
            toast("Importeer eerst MAIN én ULTRA-WIDE.")
            return
        }
        if (!main.copyVerified || !wide.copyVerified) {
            toast("Beide bestandkopieën moeten byte-identiek geverifieerd zijn.")
            return
        }
        if (main.sha256 == wide.sha256) {
            toast("MAIN en ULTRA-WIDE mogen niet hetzelfde bronbestand zijn.")
            return
        }
        val completedPoseId = poseId(currentPoseIndex)
        val completedPoseIndex = currentPoseIndex

        persistAttestation()
        prefs.edit()
            .putBoolean(completeKey(completedPoseIndex), true)
            .putLong(
                "pose_" + completedPoseIndex + "_completedAtEpochMs",
                System.currentTimeMillis()
            )
            .apply()

        val completionMessage: String
        if (currentPoseIndex < TOTAL_POSES - 1) {
            currentPoseIndex += 1
            completionMessage = "✓ " + completedPoseId + " opgeslagen. Nu: " +
                poseId(currentPoseIndex) + " (" + subsetFor(currentPoseIndex) + ")."
        } else {
            completionMessage = "✓ " + completedPoseId +
                " opgeslagen. Alle 16 pose-paren zijn compleet."
        }

        prefs.edit()
            .putString("lastActionMessage", completionMessage)
            .apply()

        renderPose()
        scrollView.post {
            scrollView.smoothScrollTo(0, 0)
        }
        toast(completionMessage)

        if (allPairsComplete()) {
            AlertDialog.Builder(this)
                .setTitle("16 pose-paren compleet")
                .setMessage(
                    "Alle 12 TRAINING + 4 HOLDOUT paren zijn vastgelegd. " +
                        "Exporteer nu het session-manifest JSON. " +
                        "Dit betekent nog niet dat een geometry-relation is admitted."
                )
                .setPositiveButton("OK", null)
                .show()
        }
    }

    private fun countCompleted(): Int = (0 until TOTAL_POSES).count {
        prefs.getBoolean(completeKey(it), false)
    }

    private fun allPairsComplete(): Boolean = countCompleted() == TOTAL_POSES

    private fun movePose(delta: Int) {
        currentPoseIndex = (currentPoseIndex + delta).coerceIn(0, TOTAL_POSES - 1)
        prefs.edit()
            .putString(
                "lastActionMessage",
                "Geopend: " + poseId(currentPoseIndex) + " (" + subsetFor(currentPoseIndex) + ")."
            )
            .apply()
        renderPose()
        scrollView.post {
            scrollView.smoothScrollTo(0, 0)
        }
    }

    private fun loadTargetFields() {
        targetFamily.setText(prefs.getString("targetFamily", "") ?: "")
        targetSpacingMm.setText(prefs.getString("targetSpacingMm", "") ?: "")
        targetSha256.setText(prefs.getString("targetSha256", "") ?: "")
    }

    private fun saveTargetFields() {
        if (!::targetFamily.isInitialized) return
        prefs.edit()
            .putString("targetFamily", targetFamily.text.toString().trim())
            .putString("targetSpacingMm", targetSpacingMm.text.toString().trim())
            .putString(
                "targetSha256",
                targetSha256.text.toString().trim().lowercase(Locale.US)
            )
            .apply()
    }

    private fun exportManifest() {
        saveTargetFields()
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(
                Intent.EXTRA_TITLE,
                "DRAW_GEOMETRY_CAPTURE_SESSION_" + sessionId() + ".json"
            )
        }
        startActivityForResult(intent, REQ_EXPORT_MANIFEST)
    }

    private fun writeManifestToUri(uri: Uri) {
        try {
            val manifest = buildManifest()
            val bytes = manifest.toString(2).toByteArray(Charsets.UTF_8)
            contentResolver.openOutputStream(uri, "w").use { out ->
                requireNotNull(out) { "Exportbestemming kan niet worden geopend" }
                out.write(bytes)
                out.flush()
            }
            toast("Session-manifest geëxporteerd.")
        } catch (e: Exception) {
            toast("Export mislukt: " + (e.message ?: e.javaClass.simpleName))
        }
    }

    private fun buildManifest(): JSONObject {
        val targetSha = (prefs.getString("targetSha256", "") ?: "")
            .trim()
            .lowercase(Locale.US)
        val targetShaValid = targetSha.matches(Regex("^[0-9a-f]{64}$"))
        val spacingText = prefs.getString("targetSpacingMm", "") ?: ""
        val spacingMm = spacingText.toDoubleOrNull()

        val pairs = JSONArray()
        for (i in 0 until TOTAL_POSES) {
            val main = loadSource(i, "MAIN")
            val wide = loadSource(i, "ULTRA_WIDE")
            val pair = JSONObject()
                .put("pose_id", poseId(i))
                .put("subset", subsetFor(i))
                .put("complete", prefs.getBoolean(completeKey(i), false))
                .put(
                    "operator_attestation",
                    JSONObject()
                        .put("authority", "OPERATOR_ATTESTATION_ONLY")
                        .put(
                            "camera_system_rigid_between_pair",
                            prefs.getBoolean(rigidKey(i), false)
                        )
                        .put(
                            "target_static_between_pair",
                            prefs.getBoolean(targetStaticKey(i), false)
                        )
                        .put("attestation_creates_measured_geometry", false)
                )
                .put("main", main?.toJson() ?: JSONObject.NULL)
                .put("ultra_wide", wide?.toJson() ?: JSONObject.NULL)
                .put(
                    "universal_pair_profile",
                    if (main != null && wide != null) {
                        UniversalSourceProfiler.pairProfile(
                            main.universalSourceProfile,
                            wide.universalSourceProfile
                        )
                    } else {
                        JSONObject.NULL
                    }
                )
                .put(
                    "recovery_assignment",
                    prefs.getString(recoveryKey(i), null)?.let {
                        try {
                            JSONObject(it)
                        } catch (_: Exception) {
                            JSONObject.NULL
                        }
                    } ?: JSONObject.NULL
                )

            val pairIdentity = if (main != null && wide != null) {
                sha256Text(
                    listOf(
                        "D_RAW_GEOMETRY_CAPTURE_PAIR_V0_1",
                        sessionId(),
                        poseId(i),
                        subsetFor(i),
                        main.sha256,
                        wide.sha256,
                        prefs.getBoolean(rigidKey(i), false).toString(),
                        prefs.getBoolean(targetStaticKey(i), false).toString()
                    ).joinToString("|")
                )
            } else {
                null
            }
            pair.put(
                "capture_pair_record_sha256",
                pairIdentity ?: JSONObject.NULL
            )
            pairs.put(pair)
        }

        val status = if (allPairsComplete()) {
            "COMPLETE_16_POSE_CAPTURE_SESSION_AUTO_PROFILED"
        } else {
            "IN_PROGRESS_CAPTURE_SESSION"
        }

        val body = JSONObject()
            .put("schema", "D.RAW/GeometryCaptureSession/0.2")
            .put("status", status)
            .put("session_id", sessionId())
            .put(
                "session_created_at_epoch_ms",
                prefs.getLong("sessionCreatedAtEpochMs", 0L)
            )
            .put("exported_at_epoch_ms", System.currentTimeMillis())
            .put(
                "capture_interface",
                JSONObject()
                    .put("mode", "UNIVERSAL_SOURCE_PROFILED_EXTERNAL_RAW_IMPORT")
                    .put("camera2_required", false)
                    .put("apk_creates_sensor_evidence", false)
                    .put("original_raw_or_dng_is_evidence", true)
                    .put("device_specific_mapping_required", false)
                    .put("workflow_role_labels_are_scientific_authority", false)
            )
            .put(
                "optional_scale_evidence",
                JSONObject()
                    .put(
                        "family",
                        prefs.getString("targetFamily", "") ?: ""
                    )
                    .put(
                        "metric_spacing_mm",
                        spacingMm ?: JSONObject.NULL
                    )
                    .put(
                        "target_geometry_sha256",
                        if (targetShaValid) targetSha else JSONObject.NULL
                    )
                    .put("required_for_relative_scene_geometry", false)
                    .put("required_for_absolute_metric_scale", true)
            )
            .put("training_pose_count_required", 12)
            .put("holdout_pose_count_required", 4)
            .put("completed_pair_count", countCompleted())
            .put(
                "session_recovery",
                JSONObject()
                    .put(
                        "origin",
                        prefs.getString(
                            "sessionOrigin",
                            "LIVE_OR_MANUAL_IMPORT"
                        )
                    )
                    .put(
                        "ordering_authority",
                        prefs.getString(
                            "recoveryOrderingAuthority",
                            "NOT_APPLICABLE"
                        )
                    )
                    .put(
                        "auto_pairing_is_geometry_evidence",
                        false
                    )
                    .put(
                        "workflow_role_labels_are_scientific_authority",
                        false
                    )
            )
            .put(
                "universal_routing",
                JSONObject()
                    .put("source_metadata_drives_initial_capabilities", true)
                    .put("frontside_scene_inspection_enabled", true)
                    .put("device_specific_mapping_required", false)
                    .put("scene_geometry_route", "AUTO_FROM_VISIBLE_IMAGE_CONTENT")
                    .put("indexed_target_required", false)
                    .put("absolute_metric_scale_requires_scale_evidence", true)
                    .put("unknown_fields_remain_unknown", true)
            )
            .put("pairs", pairs)
            .put(
                "relation_authority",
                JSONObject()
                    .put("axis", "GEOMETRY")
                    .put("status", "UNKNOWN")
                    .put("coordinate_transform_allowed", false)
                    .put("fusion_allowed", false)
                    .put("calibration_transfer_allowed", false)
            )
            .put(
                "open_world_evolution",
                JSONObject()
                    .put("law", "SEAL_EVIDENCE_NOT_THINKING")
                    .put("source_bytes_immutable", true)
                    .put("scientific_models_final", false)
                    .put("future_successors_allowed", true)
            )
            .put(
                "invariants",
                JSONObject()
                    .put("source_bytes_mutated", false)
                    .put("creates_new_evidence", false)
                    .put("scientific_writeback_allowed", false)
                    .put("geometry_relation_granted", false)
                    .put("fusion_granted", false)
            )

        body.put(
            "app_serialization_sha256",
            sha256Text(body.toString())
        )
        return body
    }

    private fun sha256Text(value: String): String {
        val digest = MessageDigest.getInstance("SHA-256")
        return digest.digest(value.toByteArray(Charsets.UTF_8)).toHex()
    }

    private fun confirmReset() {
        AlertDialog.Builder(this)
            .setTitle("Nieuwe lege sessie?")
            .setMessage(
                "Dit wist alleen de APK-sessiestatus. " +
                    "Reeds gekopieerde RAW/DNG-bestanden worden niet automatisch verwijderd."
            )
            .setNegativeButton("Annuleren", null)
            .setPositiveButton("Nieuwe sessie") { _, _ ->
                prefs.edit().clear().apply()
                ensureSession()
                currentPoseIndex = 0
                loadTargetFields()
                renderPose()
                toast("Nieuwe sessie gestart.")
            }
            .show()
    }

    private fun dp(value: Int): Int =
        (value * resources.displayMetrics.density).toInt()

    private fun toast(message: String) {
        Toast.makeText(this, message, Toast.LENGTH_LONG).show()
    }

    data class ImportedSource(
        val role: String,
        val displayName: String,
        val sourceUri: String,
        val localCopyPath: String,
        val byteLength: Long,
        val sha256: String,
        val copiedSha256: String,
        val copyVerified: Boolean,
        val importedAtEpochMs: Long,
        val universalSourceProfile: JSONObject
    ) {
        fun toJson(): JSONObject = JSONObject()
            .put("role", role)
            .put("display_name", displayName)
            .put("source_uri", sourceUri)
            .put("local_copy_path", localCopyPath)
            .put("byte_length", byteLength)
            .put("sha256", sha256)
            .put("copied_sha256", copiedSha256)
            .put("copy_verified", copyVerified)
            .put("imported_at_epoch_ms", importedAtEpochMs)
            .put("universal_source_profile", universalSourceProfile)

        companion object {
            fun fromJson(j: JSONObject): ImportedSource = ImportedSource(
                role = j.getString("role"),
                displayName = j.getString("display_name"),
                sourceUri = j.getString("source_uri"),
                localCopyPath = j.getString("local_copy_path"),
                byteLength = j.getLong("byte_length"),
                sha256 = j.getString("sha256"),
                copiedSha256 = j.getString("copied_sha256"),
                copyVerified = j.getBoolean("copy_verified"),
                importedAtEpochMs = j.getLong("imported_at_epoch_ms"),
                universalSourceProfile = j.optJSONObject(
                    "universal_source_profile"
                ) ?: JSONObject()
            )
        }
    }
}
