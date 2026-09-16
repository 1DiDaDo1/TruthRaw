package com.truthraw.adaptiveui

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Color
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraManager
import android.os.Bundle
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.view.WindowInsets
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView

/**
 * FotoGraaf diagnostic bootstrap.
 *
 * Important safety property: onCreate() touches NO CameraManager and opens NO
 * camera. Camera2 is entered only after explicit user actions, one layer at a
 * time. This keeps HONOR/HAL failures observable instead of collapsing startup.
 * No step in this Activity grants evidence or calibration authority.
 */
class FotoGraafDiagnosticBootstrapActivity : Activity() {

    private lateinit var statusView: TextView
    private lateinit var standardButton: Button
    private lateinit var honorButton: Button
    private lateinit var previewButton: Button
    private var standardPassed = false
    private var honorPassed = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setDecorFitsSystemWindows(false)
        setContentView(buildUi())
        show("BOOT_OK · geïntegreerde suite · nog geen CameraManager aangeraakt.\n" +
            "Launcher + permission gate + Activity-start zijn los van de camera-HAL gehouden.")
    }

    private fun buildUi(): View {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.TOP
            setBackgroundColor(Color.rgb(12, 14, 17))
            setPadding(dp(16), dp(16), dp(16), dp(24))
            setOnApplyWindowInsetsListener { view, insets ->
                val bars = insets.getInsets(WindowInsets.Type.systemBars() or WindowInsets.Type.displayCutout())
                view.setPadding(dp(16) + bars.left, dp(16) + bars.top, dp(16) + bars.right, dp(24) + bars.bottom)
                insets
            }
        }
        val scroll = ScrollView(this).apply {
            isFillViewport = true
            addView(root, ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))
        }

        root.addView(label("FotoGraaf · Diagnostic Bootstrap", 26f, true))
        root.addView(label(
            "nul-camera startup → standaard Camera2 → HONOR scan → preview → Pro RAW",
            13f,
            false,
            Color.rgb(185, 191, 202),
        ))
        root.addView(space(12))

        statusView = label("Initialiseren…", 13f, false, Color.WHITE)
        root.addView(statusView)
        root.addView(space(14))

        standardButton = button("Stap 1 · standaard Camera2 inventaris") { runStandardInventory() }
        honorButton = button("Stap 2 · HONOR uitgebreide scan") { runHonorInventory() }.apply { isEnabled = false }
        previewButton = button("Stap 3 · preview-only test openen") {
            startActivity(Intent(this@FotoGraafDiagnosticBootstrapActivity, FotoGraafSafePreviewActivity::class.java))
        }.apply { isEnabled = false }

        root.addView(standardButton)
        root.addView(space(6))
        root.addView(honorButton)
        root.addView(space(6))
        root.addView(previewButton)
        root.addView(space(14))

        root.addView(button("Open FotoGraaf Pro · live RAW + ISO/tijd/EV/AF/MF/OIS") {
            startActivity(Intent(this, FotoGraafProCameraActivity::class.java))
        })
        root.addView(space(6))
        root.addView(button("Open historische bewezen single-RAW route-capture") {
            startActivity(Intent(this, FotoGraafCameraActivity::class.java))
        })
        root.addView(space(6))
        root.addView(button("Open TruthRaw processor") {
            startActivity(Intent(this, MainActivity::class.java))
        })
        root.addView(space(16))

        root.addView(label(
            "Authority: DIAGNOSTIC_ONLY. Discovery/preview/vendor metadata verhogen physicalFrameCount of independentEvidenceCount niet. Alleen een werkelijk gekoppelde RAW-capture kan nieuwe capture-evidence vormen.",
            11f,
            false,
            Color.rgb(145, 153, 165),
        ))
        return scroll
    }

    private fun runStandardInventory() {
        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            show("Stap 1 geblokkeerd: CAMERA permission ontbreekt.")
            return
        }
        standardButton.isEnabled = false
        honorButton.isEnabled = false
        previewButton.isEnabled = false
        show("Stap 1 bezig · alleen Android standaard Camera2; geen com.hihonor.* request writes…")

        Thread({
            val report = runCatching {
                val manager = getSystemService(CameraManager::class.java)
                val rawIds = manager.cameraIdList.toList()
                val rows = mutableListOf<String>()
                rawIds.forEach { id ->
                    val c = manager.getCameraCharacteristics(id)
                    val physical = c.physicalCameraIds.toList().sorted()
                    val caps = c.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES)?.toList().orEmpty()
                    val rawCap = caps.contains(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_RAW)
                    rows += "logical=$id physical=$physical RAWcap=$rawCap"
                }
                buildString {
                    append("STANDARD_CAMERA2_OK\n")
                    append("cameraIdList=$rawIds\n")
                    if (rows.isEmpty()) append("geen logical rows\n") else rows.forEach { append(it).append('\n') }
                }
            }
            runOnUiThread {
                report.onSuccess { text ->
                    standardPassed = true
                    honorButton.isEnabled = true
                    standardButton.isEnabled = true
                    show(text + "\nStap 1 is stabiel. Je kunt nu Stap 2 starten.")
                }.onFailure { error ->
                    standardPassed = false
                    standardButton.isEnabled = true
                    show("STANDARD_CAMERA2_FAIL · ${error.javaClass.name}: ${error.message}")
                }
            }
        }, "truthraw-standard-camera2-inventory").start()
    }

    private fun runHonorInventory() {
        if (!standardPassed) {
            show("Stap 2 geblokkeerd: voer eerst Stap 1 uit.")
            return
        }
        honorButton.isEnabled = false
        previewButton.isEnabled = false
        show("Stap 2 bezig · bestaande HONOR runtime inventory wordt nu pas aangeroepen…")

        Thread({
            val result = runCatching {
                val manager = getSystemService(CameraManager::class.java)
                HonorCameraProbe.scan(manager)
            }
            runOnUiThread {
                result.onSuccess { report ->
                    honorPassed = true
                    honorButton.isEnabled = true
                    previewButton.isEnabled = report.routes.isNotEmpty()
                    show(buildString {
                        append("HONOR_SCAN_OK\n")
                        append("rawCameraIds=${report.rawCameraIds}\n")
                        append("physical=${report.discoveredPhysicalIds}\n")
                        append("routes=${report.routes.size}\n")
                        report.routes.take(12).forEachIndexed { index, route ->
                            append(index).append(": ").append(route.label).append('\n')
                        }
                        append("\nStap 2 is stabiel. Preview-only en Pro-camera kunnen apart worden getest.")
                    })
                }.onFailure { error ->
                    honorPassed = false
                    honorButton.isEnabled = true
                    show("HONOR_SCAN_FAIL · ${error.javaClass.name}: ${error.message}")
                }
            }
        }, "truthraw-honor-inventory-explicit").start()
    }

    private fun show(message: String) { statusView.text = message }

    private fun label(value: String, size: Float, bold: Boolean, color: Int = Color.WHITE): TextView =
        TextView(this).apply {
            text = value
            textSize = size
            setTextColor(color)
            if (bold) setTypeface(typeface, android.graphics.Typeface.BOLD)
        }

    private fun button(value: String, action: () -> Unit): Button = Button(this).apply {
        text = value
        isAllCaps = false
        minHeight = dp(52)
        setOnClickListener { action() }
    }

    private fun space(height: Int): View = View(this).apply {
        layoutParams = LinearLayout.LayoutParams(1, dp(height))
    }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).toInt()
}
