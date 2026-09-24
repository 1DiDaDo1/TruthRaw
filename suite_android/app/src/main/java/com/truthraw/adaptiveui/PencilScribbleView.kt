package com.truthraw.adaptiveui

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.view.View
import kotlin.math.max

/** Small deterministic colored-pencil accent for the D.RAW interface only. */
class PencilScribbleView(context: Context, private val accent: Int) : View(context) {
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = accent
        style = Paint.Style.STROKE
        strokeCap = Paint.Cap.ROUND
        alpha = 135
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val w = width.toFloat(); val h = height.toFloat()
        if (w <= 0f || h <= 0f) return
        paint.strokeWidth = max(2f, resources.displayMetrics.density * 2f)
        canvas.drawLine(w*.08f,h*.72f,w*.54f,h*.20f,paint)
        canvas.drawLine(w*.22f,h*.80f,w*.72f,h*.24f,paint)
        canvas.drawLine(w*.38f,h*.78f,w*.90f,h*.30f,paint)
        canvas.drawLine(w*.16f,h*.58f,w*.66f,h*.08f,paint)
    }
}
