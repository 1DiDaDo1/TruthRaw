package com.truthraw.adaptiveui

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Color
import android.os.Bundle
import android.provider.Settings
import android.view.Gravity
import android.view.ViewGroup
import android.widget.Button
import android.widget.LinearLayout
import android.widget.TextView

/** Permission gate before FotoGraaf v0.4.3 zero-camera diagnostic bootstrap. */
class FotoGraafPermissionGateActivity : Activity() {

    private lateinit var statusView: TextView
    private lateinit var allowButton: Button
    private lateinit var settingsButton: Button
    private var permissionRequestInFlight = false
    private var launched = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        DrawVisualTheme.applyWindow(this)
        launched = savedInstanceState?.getBoolean(STATE_LAUNCHED, false) ?: false
        permissionRequestInFlight = savedInstanceState?.getBoolean(STATE_PERMISSION_IN_FLIGHT, false) ?: false
        setContentView(buildUi())
        continueWhenPermitted()
    }

    override fun onSaveInstanceState(outState: Bundle) {
        outState.putBoolean(STATE_LAUNCHED, launched)
        outState.putBoolean(STATE_PERMISSION_IN_FLIGHT, permissionRequestInFlight)
        super.onSaveInstanceState(outState)
    }

    override fun onResume() {
        super.onResume()
        if (::statusView.isInitialized && hasCameraPermission() && !permissionRequestInFlight) {
            launchDiagnosticOnce()
        }
    }

    private fun continueWhenPermitted() {
        if (hasCameraPermission()) {
            launchDiagnosticOnce()
            return
        }
        statusView.text = "Camera-toestemming is nodig. Na toestemming opent FotoGraaf v0.4.3 eerst een nul-camera diagnostisch scherm; er wordt nog geen CameraManager, HONOR-scan of preview gestart."
        allowButton.isEnabled = false
        settingsButton.visibility = android.view.View.GONE
        if (!permissionRequestInFlight) {
            permissionRequestInFlight = true
            requestPermissions(arrayOf(Manifest.permission.CAMERA), REQUEST_CAMERA_PERMISSION)
        }
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray,
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode != REQUEST_CAMERA_PERMISSION) return
        permissionRequestInFlight = false
        if (grantResults.firstOrNull() == PackageManager.PERMISSION_GRANTED) {
            statusView.text = "Camera-toestemming verleend. Diagnostic Bootstrap wordt geopend…"
            window.decorView.post { launchDiagnosticOnce() }
        } else {
            statusView.text = "Camera-toestemming niet verleend."
            allowButton.isEnabled = true
            settingsButton.visibility = android.view.View.VISIBLE
        }
    }

    private fun launchDiagnosticOnce() {
        if (!hasCameraPermission() || launched || isFinishing || isDestroyed) return
        launched = true
        allowButton.isEnabled = false
        runCatching {
            startActivity(Intent(this, FotoGraafDiagnosticBootstrapActivity::class.java).apply {
                addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP or Intent.FLAG_ACTIVITY_CLEAR_TOP)
            })
        }.onFailure { error ->
            launched = false
            allowButton.isEnabled = true
            statusView.text = "Diagnostic Bootstrap kon niet starten: ${error.javaClass.simpleName}: ${error.message ?: "onbekende fout"}"
            return
        }
        finish()
    }

    private fun hasCameraPermission(): Boolean =
        checkSelfPermission(Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED

    private fun buildUi(): LinearLayout {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(dp(24), dp(32), dp(24), dp(32))
            setBackgroundColor(DrawVisualTheme.PAPER_YELLOW)
            layoutParams = ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT)
        }
        root.addView(TextView(this).apply {
            text = "FotoGraaf · Camera toegang"
            textSize = 28f
            setTextColor(DrawVisualTheme.INK)
            setTypeface(typeface, android.graphics.Typeface.BOLD)
        })
        statusView = TextView(this).apply {
            textSize = 15f
            setTextColor(DrawVisualTheme.INK)
            setPadding(0, dp(16), 0, dp(18))
        }
        root.addView(statusView)
        allowButton = Button(this).apply {
            text = "Camera-toestemming geven"
            isAllCaps = false
            setOnClickListener { continueWhenPermitted() }
        }
        root.addView(allowButton)
        settingsButton = Button(this).apply {
            text = "Open app-instellingen"
            isAllCaps = false
            visibility = android.view.View.GONE
            setOnClickListener {
                startActivity(Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS).apply {
                    data = android.net.Uri.parse("package:$packageName")
                })
            }
        }
        root.addView(settingsButton)
        return root
    }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).toInt()

    companion object {
        private const val REQUEST_CAMERA_PERMISSION = 401
        private const val STATE_LAUNCHED = "truthraw.fotograaf.diagnostic.launched"
        private const val STATE_PERMISSION_IN_FLIGHT = "truthraw.fotograaf.diagnostic.permission_in_flight"
    }
}
