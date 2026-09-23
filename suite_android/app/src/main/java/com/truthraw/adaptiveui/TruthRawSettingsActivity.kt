package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.graphics.Color
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.os.Bundle
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.view.WindowInsets
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import io.truthraw.debug.MainActivity as DeviceVerificationActivity
import java.io.File

class TruthRawSettingsActivity : Activity() {
    private val backgroundColor = Color.rgb(5, 12, 22)
    private val surface = Color.rgb(10, 22, 37)
    private val textPrimary = Color.rgb(244, 248, 255)
    private val textMuted = Color.rgb(158, 178, 205)
    private val blue = Color.rgb(63, 142, 255)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setDecorFitsSystemWindows(false)
        window.statusBarColor = backgroundColor
        window.navigationBarColor = backgroundColor
        setContentView(buildUi())
    }

    private fun buildUi(): ScrollView {
        val root = vertical().apply {
            setBackgroundColor(backgroundColor)
            setPadding(dp(18), dp(12), dp(18), dp(24))
        }

        root.addView(horizontal().apply {
            gravity = Gravity.CENTER_VERTICAL
            addView(TextView(this@TruthRawSettingsActivity).apply {
                text = "‹"
                textSize = 36f
                gravity = Gravity.CENTER
                setTextColor(textPrimary)
                setOnClickListener { finish() }
            }, LinearLayout.LayoutParams(dp(48), dp(48)).apply { marginEnd = dp(8) })
            addView(vertical().apply {
                addView(title("Instellingen", 26f))
                addView(body("TruthRaw v0.84.2 · PURE / ADVANCED / PRO · adaptive compute", 11.5f))
            }, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
        })

        root.addView(space(18))
        root.addView(sectionCard("Interface").apply {
            addView(body(
                "De normale productieroute staat op het hoofdscherm: kies PURE, ADVANCED of PRO en daarna RAW/DNG of Camera. " +
                    "Instellingen bevat daarom geen dubbele productieknoppen meer.",
                12.5f,
            ))
        })

        root.addView(space(14))
        root.addView(sectionCard("Onderzoek & diagnostiek").apply {
            addView(body(
                "Alle historische probes en testgereedschappen staan hier uit de hoofdinterface. Ze promoveren geen claims en veranderen de PURE-route niet.",
                12f,
            ))
            addView(space(10))
            addDiagnostic("200MP TEST · crash-isolatie ingang", FotoGraaf200MpEntryActivity::class.java)
            addDiagnostic("v0.40 · Passieve Honor route observer", HonorPassiveRouteObserverActivity::class.java)
            addDiagnostic("v0.41 · Honor output-config callback probe", HonorOutputConfigCallbackProbeActivity::class.java)
            addDiagnostic("v0.42 · Passieve MediaStore + Camera timeline", PassiveMediaStoreCameraTimelineActivity::class.java)
            addDiagnostic("v0.43 · Passieve mode/shutter timeline", PassiveModeShutterTimelineActivity::class.java)
            addDiagnostic("v0.44 · HI-RES main → tele state anchors", PassiveHiresTeleStateTimelineActivity::class.java)
            addDiagnostic("v0.45 · Exported JPEG metadata fingerprint", PassiveExportedJpegMetadataFingerprintActivity::class.java)
            addDiagnostic("v0.46 · Honor capability route oracle", HonorCapabilityRouteOracleActivity::class.java)
            addDiagnostic("v0.48 · Target 37 RAW14 retest", Api37Raw14ExtensionOracleActivity::class.java)
            addDiagnostic("v0.62 · Android 17 vendor extensions", Api37VendorExtensionOracleActivity::class.java)
            addDiagnostic("v0.49 · Honor Camera package export", HonorCameraPackageExportActivity::class.java)
            addDiagnostic("v0.50 · Direct typed Honor vendor-key read", DirectTypedVendorCharacteristicsOracleActivity::class.java)
            addDiagnostic("v0.61 · MotionCam vendor/session oracle", MotionCamVendorSessionOracleActivity::class.java)
            addDiagnostic("v0.64 · Physical-5 template defaults", Physical5TemplateDefaultsOracleActivity::class.java)
            addDiagnostic("v0.65 · Physical-5 local type oracle", Physical5LocalTypeOracleActivity::class.java)
            addDiagnostic("v0.66 · MasterFilm session acceptance", Physical5MasterFilmSessionAcceptanceActivity::class.java)
            addDiagnostic("v0.67 · aoRunningMode session acceptance", Physical5AoRunningSessionAcceptanceActivity::class.java)
            addDiagnostic("v0.68 · Physical-5 complete session matrix", Physical5SessionAcceptanceMatrixActivity::class.java)
            addDiagnostic("v0.69 · Physical-5 crash-safe matrix", Physical5CrashSafeSessionAcceptanceMatrixActivity::class.java)
            addDiagnostic("v0.51 · CameraDeviceSetup RAW14 query", CameraDeviceSetupRaw14SessionOracleActivity::class.java)
            addDiagnostic("v0.53 · Android 17 replay bewezen route", Android17Camera5PayloadDeltaActivity::class.java)
            addDiagnostic("v0.54 · Honor Pro RAW/DNG fingerprint", PassiveHonorProRawDngFingerprintActivity::class.java)
            addDiagnostic("v0.55 · Android-17/HONOR nulmeting", CurrentAndroid17HonorCameraBaselineActivity::class.java)
            addView(action("FotoGraaf camera & diagnostics · legacy") {
                startActivity(Intent(this@TruthRawSettingsActivity, FotoGraafPermissionGateActivity::class.java))
            })
            addView(space(8))
            addView(action("Device verification v0.3 · source + CFA") {
                startActivity(Intent(this@TruthRawSettingsActivity, DeviceVerificationActivity::class.java))
            })
            if (File(filesDir, TruthRawSuiteApplication.CRASH_FILE).exists()) {
                addView(space(8))
                addView(action("Laatste crash bekijken / opslaan") {
                    startActivity(Intent(this@TruthRawSettingsActivity, TruthRawCrashReportActivity::class.java))
                })
            }
        })

        root.addView(space(14))
        root.addView(sectionCard("Wetenschappelijke grens").apply {
            addView(body(
                "Measured waar measured. Reconstructed waar noodzakelijk. Representation mag de bron overstijgen; kennisclaims niet. physicalFrameCount=1 en independentEvidenceCount=1 blijven onveranderd.",
                12f,
            ))
        })

        return ScrollView(this).apply {
            isFillViewport = true
            clipToPadding = true
            setBackgroundColor(backgroundColor)
            setOnApplyWindowInsetsListener { view, insets ->
                val bars = insets.getInsets(WindowInsets.Type.systemBars())
                view.setPadding(bars.left, bars.top, bars.right, bars.bottom)
                insets
            }
            addView(root)
        }
    }

    private fun LinearLayout.addDiagnostic(label: String, clazz: Class<out Activity>) {
        addView(action(label) {
            startActivity(Intent(this@TruthRawSettingsActivity, clazz))
        })
        addView(space(8))
    }

    private fun sectionCard(heading: String): LinearLayout = vertical().apply {
        setPadding(dp(15), dp(15), dp(15), dp(15))
        background = GradientDrawable().apply {
            shape = GradientDrawable.RECTANGLE
            cornerRadius = dp(18).toFloat()
            setColor(surface)
            setStroke(dp(1), Color.rgb(37, 63, 92))
        }
        addView(title(heading, 18f))
        addView(space(8))
    }

    private fun action(label: String): View = TextView(this).apply {
        text = label
        textSize = 13.5f
        setTextColor(textPrimary)
        gravity = Gravity.CENTER_VERTICAL
        setPadding(dp(13), dp(12), dp(13), dp(12))
        background = GradientDrawable().apply {
            shape = GradientDrawable.RECTANGLE
            cornerRadius = dp(13).toFloat()
            setColor(Color.rgb(14, 30, 49))
            setStroke(dp(1), Color.rgb(44, 75, 111))
        }
        setOnClickListener { }
    }

    private fun action(label: String, onClick: () -> Unit): View =
        (action(label) as TextView).apply { setOnClickListener { onClick() } }

    private fun vertical() = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
    private fun horizontal() = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
    private fun space(height: Int) = View(this).apply {
        layoutParams = LinearLayout.LayoutParams(1, dp(height))
    }
    private fun title(value: String, size: Float) = TextView(this).apply {
        text = value
        textSize = size
        setTextColor(textPrimary)
        setTypeface(typeface, Typeface.BOLD)
    }
    private fun body(value: String, size: Float) = TextView(this).apply {
        text = value
        textSize = size
        setTextColor(textMuted)
        setLineSpacing(0f, 1.12f)
    }
    private fun dp(value: Int): Int = (value * resources.displayMetrics.density + 0.5f).toInt()
}
