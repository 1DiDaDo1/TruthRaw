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

/** Staged Camera2/HONOR diagnostics. No diagnostic step grants scientific authority. */
class FotoGraafDiagnosticBootstrapActivity : Activity() {

    private lateinit var statusView: TextView
    private lateinit var standardButton: Button
    private lateinit var honorButton: Button
    private lateinit var previewButton: Button
    private var standardPassed = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        DrawVisualTheme.applyWindow(this)
        window.setDecorFitsSystemWindows(false)
        setContentView(buildUi())
        show("BOOT_OK · nog geen CameraManager aangeraakt.\n" +
            "Aanbevolen: Preview-only om lenzen te bekijken; 200MP Tele Test voor de fysieke 200MP-gate.")
    }

    private fun buildUi(): View {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.TOP
            setBackgroundColor(DrawVisualTheme.PAPER_YELLOW)
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

        root.addView(label("FotoGraaf · Camera routes", 26f, true))
        root.addView(label(
            "v0.6 · kies bewust tussen preview-only, 200MP test en legacy diagnostics",
            13f,
            false,
            Color.rgb(185, 191, 202),
        ))
        root.addView(space(12))

        root.addView(button("200MP TELE TEST · physical 5 · 16320×12288") {
            startActivity(Intent(this, FotoGraaf200MpTestActivity::class.java))
        })
        root.addView(space(6))
        root.addView(button("PREVIEW-ONLY · bekijk wide / main / tele veilig") {
            startActivity(Intent(this, FotoGraafSafePreviewActivity::class.java))
        })
        root.addView(space(14))

        statusView = label("Initialiseren…", 13f, false, Color.WHITE)
        root.addView(statusView)
        root.addView(space(14))

        standardButton = button("Diagnose stap 1 · standaard Camera2 inventaris") { runStandardInventory() }
        honorButton = button("Diagnose stap 2 · HONOR uitgebreide scan") { runHonorInventory() }.apply { isEnabled = false }
        previewButton = button("Diagnose stap 3 · preview-only test") {
            startActivity(Intent(this@FotoGraafDiagnosticBootstrapActivity, FotoGraafSafePreviewActivity::class.java))
        }.apply { isEnabled = false }

        root.addView(standardButton)
        root.addView(space(6))
        root.addView(honorButton)
        root.addView(space(6))
        root.addView(previewButton)
        root.addView(space(14))

        root.addView(button("LEGACY Pro v0.5 · preview+RAW gecombineerde session (niet aanbevolen)") {
            startActivity(Intent(this, FotoGraafProCameraActivity::class.java))
        })
        root.addView(space(6))
        root.addView(button("Historische bewezen single-RAW route-capture") {
            startActivity(Intent(this, FotoGraafCameraActivity::class.java))
        })
        root.addView(space(6))
        root.addView(button("Open D.RAW processor") {
            startActivity(Intent(this, MainActivity::class.java))
        })
        root.addView(space(16))

        root.addView(label(
            "Waarom legacy? v0.5 combineert preview en RAW ImageReader in één Camera2-session. De veldtest liet zien dat dit op HONOR een zwarte/afwezige preview kan geven. v0.6 200MP gebruikt daarom preview-only → RAW-only als twee afzonderlijke sessions.",
            11f,
            false,
            Color.rgb(160, 169, 182),
        ))
        root.addView(space(8))
        root.addView(label(
            "Authority: DIAGNOSTIC_ONLY. Discovery en preview verhogen physicalFrameCount of independentEvidenceCount niet. Alleen een werkelijk gekoppelde RAW-capture kan nieuwe capture-evidence vormen.",
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
        show("Stap 1 bezig · alleen Android standaard Camera2…")

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
                    show(text + "\nStap 1 stabiel; stap 2 is nu beschikbaar.")
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
        show("Stap 2 bezig · HONOR runtime inventory…")

        Thread({
            val result = runCatching {
                val manager = getSystemService(CameraManager::class.java)
                HonorCameraProbe.scan(manager)
            }
            runOnUiThread {
                result.onSuccess { report ->
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
                        append("\nGebruik Preview-only voor beeld; gebruik 200MP Tele Test voor 16320×12288.")
                    })
                }.onFailure { error ->
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
