package com.truthraw.adaptiveui

import android.content.Context
import android.util.AttributeSet
import android.view.TextureView
import kotlin.math.roundToInt

/**
 * TextureView that preserves a requested camera-preview aspect ratio.
 *
 * ratioWidth/ratioHeight describe the display-space ratio, not evidence or
 * capture geometry. This class changes presentation only.
 */
class AutoFitTextureView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0,
) : TextureView(context, attrs, defStyleAttr) {

    private var ratioWidth = 0
    private var ratioHeight = 0

    fun setAspectRatio(width: Int, height: Int) {
        require(width > 0 && height > 0) { "preview aspect ratio must be positive" }
        ratioWidth = width
        ratioHeight = height
        requestLayout()
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec)
        if (ratioWidth == 0 || ratioHeight == 0) return

        val availableWidth = MeasureSpec.getSize(widthMeasureSpec)
        val availableHeight = MeasureSpec.getSize(heightMeasureSpec)
        if (availableWidth <= 0 || availableHeight <= 0) return

        val widthFromHeight =
            (availableHeight.toDouble() * ratioWidth.toDouble() / ratioHeight.toDouble()).roundToInt()
        val heightFromWidth =
            (availableWidth.toDouble() * ratioHeight.toDouble() / ratioWidth.toDouble()).roundToInt()

        if (widthFromHeight <= availableWidth) {
            setMeasuredDimension(widthFromHeight, availableHeight)
        } else {
            setMeasuredDimension(availableWidth, heightFromWidth)
        }
    }
}
