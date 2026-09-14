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

/**
 * Permission gate in front of FotoGraafCameraActivity.
 *
 * The route inventory must never run before CAMERA permission has been granted
 * on devices that hide/deny Camera2 characteristics without runtime access.
 * This also avoids the v0.3 deadlock where zero routes disabled the only path
 * that could have triggered the permission request.
 */
class FotoGraafPermissionGateActivity : Activity() {

    private lateinit var statusView: TextView
    private lateinit var allowButton: Button
    private lateinit var settingsButton: Button

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(buildUi())
        continueWhenPermitted()
    }

    override fun onResume() {
        super.onResume()
        if (::statusView.isInitialized && hasCameraPermission()) {
            launchFotoGraaf()
        }
    }

    private fun continueWhenPermitted() {
        if (hasCameraPermission()) {
            launchFotoGraaf()
            return
        }
        statusView.text = "Camera-toestemming is nodig vóór de HONOR Camera2 inventory kan worden gelezen."
        allowButton.isEnabled = true
        settingsButton.visibility = android.view.View.GONE
        requestPermissions(arrayOf(Manifest.permission.CAMERA), REQUEST_CAMERA_PERMISSION)
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray,
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode != REQUEST_CAMERA_PERMISSION) return

        if (grantResults.firstOrNull() == PackageManager.PERMISSION_GRANTED) {
            statusView.text = "Camera-toestemming verleend. HONOR inventory wordt geopend…"
            launchFotoGraaf()
        } else {
            statusView.text =
                "Camera-toestemming is niet verleend. Zonder deze toestemming start FotoGraaf geen route-scan."
            allowButton.isEnabled = true
            settingsButton.visibility = android.view.View.VISIBLE
        }
    }

    private fun launchFotoGraaf() {
        if (!hasCameraPermission()) return
        startActivity(Intent(this, FotoGraafCameraActivity::class.java))
        finish()
    }

    private fun hasCameraPermission(): Boolean =
        checkSelfPermission(Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED

    private fun buildUi(): LinearLayout {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(dp(24), dp(32), dp(24), dp(32))
            setBackgroundColor(Color.rgb(18, 20, 24))
            layoutParams = ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT,
            )
        }

        root.addView(TextView(this).apply {
            text = "FotoGraaf · Camera toegang"
            textSize = 28f
            setTextColor(Color.WHITE)
            setTypeface(typeface, android.graphics.Typeface.BOLD)
        })
        root.addView(TextView(this).apply {
            text = "De Camera2/HONOR inventory wordt pas gelezen nadat Android de camera-toestemming heeft verleend."
            textSize = 15f
            setTextColor(Color.rgb(195, 200, 210))
            setPadding(0, dp(12), 0, dp(20))
        })

        statusView = TextView(this).apply {
            textSize = 15f
            setTextColor(Color.WHITE)
            setPadding(0, 0, 0, dp(18))
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
    }
}
