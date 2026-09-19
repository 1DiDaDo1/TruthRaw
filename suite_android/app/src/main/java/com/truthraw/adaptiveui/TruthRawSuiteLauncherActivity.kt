package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.graphics.Color
import android.graphics.Typeface
import android.os.Bundle
import android.view.Gravity
import android.view.ViewGroup
import android.view.WindowInsets
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import io.truthraw.debug.MainActivity as DeviceVerificationActivity
import java.io.File

class TruthRawSuiteLauncherActivity : Activity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setDecorFitsSystemWindows(false)
        setContentView(buildUi())
    }

    private fun buildUi(): ScrollView {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER_VERTICAL
            setBackgroundColor(Color.rgb(15, 17, 20))
            setPadding(dp(24), dp(20), dp(24), dp(24))
            layoutParams = ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT)
            setOnApplyWindowInsetsListener { view, insets ->
                val bars = insets.getInsets(WindowInsets.Type.systemBars())
                view.setPadding(dp(24) + bars.left, dp(20) + bars.top, dp(24) + bars.right, dp(24) + bars.bottom)
                insets
            }
        }
        val scroll = ScrollView(this).apply {
            isFillViewport = true
            setBackgroundColor(Color.rgb(15, 17, 20))
            addView(root)
        }

        root.addView(TextView(this).apply {
            text = "TruthRaw Suite"
            textSize = 31f
            setTextColor(Color.WHITE)
            setTypeface(typeface, Typeface.BOLD)
        })
        root.addView(TextView(this).apply {
            text = "v0.62 · PURE 32-bit Float DNG · self-binding + post-write verificatie"
            textSize = 16f
            setTextColor(Color.rgb(190, 196, 205))
            setPadding(0, dp(5), 0, dp(8))
        })
        root.addView(TextView(this).apply {
            text = "Kies eerst een bestaand RAW-bestand van smartphone of professionele camera. Camera-toegang is de tweede ingang. Beide routes komen bij dezelfde sealed-source RAW-ingang uit. TRUTHRAW PURE blijft 32-bit IEEE Float XYZ-D50 LinearRaw DNG met exact Scientific-Master digest gate. De v0.61 self-binding met Zero-Line/L0, scene-scale en Technical Backplane wordt nu na het schrijven uit het opgeslagen DNG-bestand teruggelezen; zonder die bevestiging meldt de app geen succes. De 16-bit Linear DNG blijft alleen compatibility. Nikon NEF blijft measurement/radiometric-gated totdat volledige scientific admission is bewezen."
            textSize = 14f
            setTextColor(Color.rgb(190, 198, 209))
            setPadding(0, 0, 0, dp(14))
        })

        root.addView(actionButton("1 · RAW-bestand openen · smartphone / professionele camera") {
            startActivity(Intent(this, MainActivity::class.java).apply {
                putExtra(MainActivity.EXTRA_AUTO_OPEN_RAW_PICKER, true)
            })
        })
        root.addView(space())

        root.addView(actionButton("2 · Camera gebruiken · capture → dezelfde RAW-ingang") {
            startActivity(Intent(this, FotoGraafCameraActivity::class.java))
        })
        root.addView(space())

        root.addView(TextView(this).apply {
            text = "Onderzoek & diagnostiek"
            textSize = 18f
            setTextColor(Color.WHITE)
            setTypeface(typeface, Typeface.BOLD)
            setPadding(0, dp(10), 0, dp(8))
        })

        root.addView(actionButton("200MP TEST · crash-isolatie ingang") {
            startActivity(Intent(this, FotoGraaf200MpEntryActivity::class.java))
        })
        root.addView(space())

        root.addView(actionButton("v0.40 · Passieve Honor route observer") {
            startActivity(Intent(this, HonorPassiveRouteObserverActivity::class.java))
        })
        root.addView(space())

        root.addView(actionButton("v0.41 · Honor output-config callback probe") {
            startActivity(Intent(this, HonorOutputConfigCallbackProbeActivity::class.java))
        })
        root.addView(space())

        root.addView(actionButton("v0.42 · Passieve MediaStore + Camera timeline") {
            startActivity(Intent(this, PassiveMediaStoreCameraTimelineActivity::class.java))
        })
        root.addView(space())

        root.addView(actionButton("v0.43 · Passieve mode/shutter timeline") {
            startActivity(Intent(this, PassiveModeShutterTimelineActivity::class.java))
        })
        root.addView(space())

        root.addView(actionButton("v0.44 · HI-RES main → tele state anchors") {
            startActivity(Intent(this, PassiveHiresTeleStateTimelineActivity::class.java))
        })
        root.addView(space())

        root.addView(actionButton("v0.45 · Exported JPEG metadata fingerprint") {
            startActivity(Intent(this, PassiveExportedJpegMetadataFingerprintActivity::class.java))
        })
        root.addView(space())

        root.addView(actionButton("v0.46 · Honor capability route oracle") {
            startActivity(Intent(this, HonorCapabilityRouteOracleActivity::class.java))
        })
        root.addView(space())

        root.addView(actionButton("v0.48 · Target 37 retest · zelfde v0.47 oracle") {
            startActivity(Intent(this, Api37Raw14ExtensionOracleActivity::class.java))
        })
        root.addView(space())

        root.addView(actionButton("v0.49 · Honor Camera .706 package export") {
            startActivity(Intent(this, HonorCameraPackageExportActivity::class.java))
        })
        root.addView(space())

        root.addView(actionButton("v0.50 · Direct typed Honor vendor-key read") {
            startActivity(Intent(this, DirectTypedVendorCharacteristicsOracleActivity::class.java))
        })
        root.addView(space())

        root.addView(actionButton("v0.51 · CameraDeviceSetup RAW14 session query") {
            startActivity(Intent(this, CameraDeviceSetupRaw14SessionOracleActivity::class.java))
        })
        root.addView(space())

        root.addView(actionButton("v0.53 · Android 17 replay van bewezen v0.14 route") {
            startActivity(Intent(this, Android17Camera5PayloadDeltaActivity::class.java))
        })
        root.addView(space())

        root.addView(actionButton("v0.54 · Honor Pro RAW/DNG container fingerprint") {
            startActivity(Intent(this, PassiveHonorProRawDngFingerprintActivity::class.java))
        })
        root.addView(space())

        root.addView(actionButton("v0.55 · Huidige Android-17/HONOR nulmeting") {
            startActivity(Intent(this, CurrentAndroid17HonorCameraBaselineActivity::class.java))
        })
        root.addView(space())

        if (File(filesDir, TruthRawSuiteApplication.CRASH_FILE).exists()) {
            root.addView(actionButton("LAATSTE CRASH BEKIJKEN / OPSLAAN") {
                startActivity(Intent(this, TruthRawCrashReportActivity::class.java))
            })
            root.addView(space())
        }

        root.addView(actionButton("FotoGraaf camera & diagnostics · legacy") {
            startActivity(Intent(this, FotoGraafPermissionGateActivity::class.java))
        })
        root.addView(space())
        root.addView(actionButton("TruthRaw processor") {
            startActivity(Intent(this, MainActivity::class.java))
        })
        root.addView(space())
        root.addView(actionButton("Device verification v0.3 · source + CFA") {
            startActivity(Intent(this, DeviceVerificationActivity::class.java))
        })
        root.addView(space())
        root.addView(TextView(this).apply {
            text = "200MP blijft fail-closed: pas een echte 16320×12288 RAW_SENSOR Image + physical Camera-5 result + timestamp identity + MAXIMUM_RESOLUTION pixel mode kan de capture-gate passeren. Legacy previewactivities zijn nog niet als TextureView-crash-fixed gepromoveerd."
            textSize = 12f
            setTextColor(Color.rgb(145, 153, 165))
        })
        return scroll
    }

    private fun actionButton(label: String, action: () -> Unit): Button = Button(this).apply {
        text = label
        isAllCaps = false
        textSize = 17f
        minHeight = dp(54)
        setOnClickListener { action() }
    }

    private fun space() = android.view.View(this).apply { layoutParams = LinearLayout.LayoutParams(1, dp(10)) }
    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).toInt()
}