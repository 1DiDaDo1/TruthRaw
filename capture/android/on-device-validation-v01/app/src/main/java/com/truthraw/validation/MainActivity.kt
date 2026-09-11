package com.truthraw.validation

import android.app.Activity
import android.os.Bundle
import android.os.PowerManager
import android.widget.TextView

class MainActivity : Activity() {
    companion object {
        init {
            System.loadLibrary("truthraw_validation_bridge")
        }

        @JvmStatic
        external fun nativeProbe(thermalState: Int, foreground: Boolean): String
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val power = getSystemService(POWER_SERVICE) as PowerManager
        val mappedThermal = when (power.currentThermalStatus) {
            PowerManager.THERMAL_STATUS_NONE -> 0
            PowerManager.THERMAL_STATUS_LIGHT,
            PowerManager.THERMAL_STATUS_MODERATE -> 1
            PowerManager.THERMAL_STATUS_SEVERE -> 2
            PowerManager.THERMAL_STATUS_CRITICAL,
            PowerManager.THERMAL_STATUS_EMERGENCY,
            PowerManager.THERMAL_STATUS_SHUTDOWN -> 3
            else -> 3
        }

        val result = nativeProbe(mappedThermal, true)
        setContentView(TextView(this).apply {
            text = result
            textSize = 14f
            setPadding(24, 24, 24, 24)
        })
    }
}
