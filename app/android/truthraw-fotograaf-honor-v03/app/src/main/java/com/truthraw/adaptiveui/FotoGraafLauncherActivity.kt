package com.truthraw.adaptiveui

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Bundle
import android.widget.Toast

/**
 * Small permission gate in front of FotoGraaf.
 *
 * The runtime inventory must not run before CAMERA permission has been
 * resolved. Otherwise some device builds can expose an empty/filtered camera
 * inventory and the user cannot reach the capture button that previously
 * triggered the permission request.
 */
class FotoGraafLauncherActivity : Activity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (checkSelfPermission(Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) {
            openFotoGraaf()
        } else {
            requestPermissions(arrayOf(Manifest.permission.CAMERA), REQUEST_CAMERA_PERMISSION)
        }
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray,
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (
            requestCode == REQUEST_CAMERA_PERMISSION &&
            grantResults.firstOrNull() == PackageManager.PERMISSION_GRANTED
        ) {
            openFotoGraaf()
        } else {
            Toast.makeText(
                this,
                "Camera-toestemming is nodig om de HONOR Camera2 routes te inventariseren.",
                Toast.LENGTH_LONG,
            ).show()
            finish()
        }
    }

    private fun openFotoGraaf() {
        startActivity(Intent(this, FotoGraafCameraActivity::class.java))
        finish()
    }

    companion object {
        private const val REQUEST_CAMERA_PERMISSION = 401
    }
}
