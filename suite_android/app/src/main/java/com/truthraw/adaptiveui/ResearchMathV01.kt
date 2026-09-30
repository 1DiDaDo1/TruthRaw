package com.truthraw.adaptiveui

import kotlin.math.abs
import kotlin.math.sqrt

/**
 * Small deterministic research-math helpers. No authority is implied by a
 * successful numerical result.
 */
object ResearchMathV01 {
    fun mean(values: List<Double>): Double? {
        if (values.isEmpty() || values.any { !it.isFinite() }) return null
        val out = values.sum() / values.size.toDouble()
        return out.takeIf(Double::isFinite)
    }

    fun median(values: List<Double>): Double? {
        if (values.isEmpty() || values.any { !it.isFinite() }) return null
        val s = values.sorted()
        return if (s.size and 1 == 1) {
            s[s.size / 2]
        } else {
            0.5 * (s[s.size / 2 - 1] + s[s.size / 2])
        }
    }

    data class LinearFit(
        val slope: Double,
        val intercept: Double,
        val rmse: Double,
    )

    fun linearFit(
        x: List<Double>,
        y: List<Double>,
    ): LinearFit? {
        if (
            x.size != y.size ||
            x.size < 2 ||
            x.any { !it.isFinite() } ||
            y.any { !it.isFinite() }
        ) {
            return null
        }
        val mx = mean(x) ?: return null
        val my = mean(y) ?: return null
        var sxx = 0.0
        var sxy = 0.0
        for (i in x.indices) {
            val dx = x[i] - mx
            sxx += dx * dx
            sxy += dx * (y[i] - my)
        }
        if (!sxx.isFinite() || sxx <= 1.0e-18) return null
        val slope = sxy / sxx
        val intercept = my - slope * mx
        if (!slope.isFinite() || !intercept.isFinite()) return null

        var e2 = 0.0
        for (i in x.indices) {
            val e = y[i] - (slope * x[i] + intercept)
            e2 += e * e
        }
        val rmse = sqrt(e2 / x.size.toDouble())
        if (!rmse.isFinite()) return null
        return LinearFit(slope, intercept, rmse)
    }

    fun rmse(
        actual: List<Double>,
        predicted: List<Double>,
    ): Double? {
        if (
            actual.size != predicted.size ||
            actual.isEmpty() ||
            actual.any { !it.isFinite() } ||
            predicted.any { !it.isFinite() }
        ) {
            return null
        }
        var sum = 0.0
        for (i in actual.indices) {
            val d = actual[i] - predicted[i]
            sum += d * d
        }
        return sqrt(sum / actual.size.toDouble()).takeIf(Double::isFinite)
    }

    fun invert3x3(m: Array<DoubleArray>): Array<DoubleArray>? {
        if (
            m.size != 3 ||
            m.any { it.size != 3 || it.any { v -> !v.isFinite() } }
        ) {
            return null
        }
        val a = m[0][0]
        val b = m[0][1]
        val c = m[0][2]
        val d = m[1][0]
        val e = m[1][1]
        val f = m[1][2]
        val g = m[2][0]
        val h = m[2][1]
        val i = m[2][2]

        val det =
            a * (e * i - f * h) -
                b * (d * i - f * g) +
                c * (d * h - e * g)
        if (!det.isFinite() || abs(det) <= 1.0e-15) return null

        val invDet = 1.0 / det
        val out = arrayOf(
            doubleArrayOf(
                (e * i - f * h) * invDet,
                (c * h - b * i) * invDet,
                (b * f - c * e) * invDet,
            ),
            doubleArrayOf(
                (f * g - d * i) * invDet,
                (a * i - c * g) * invDet,
                (c * d - a * f) * invDet,
            ),
            doubleArrayOf(
                (d * h - e * g) * invDet,
                (b * g - a * h) * invDet,
                (a * e - b * d) * invDet,
            ),
        )
        return out.takeIf { matrix ->
            matrix.all { row -> row.all(Double::isFinite) }
        }
    }

    fun multiply3x3(
        a: Array<DoubleArray>,
        b: Array<DoubleArray>,
    ): Array<DoubleArray>? {
        if (
            a.size != 3 || b.size != 3 ||
            a.any { it.size != 3 } ||
            b.any { it.size != 3 }
        ) {
            return null
        }
        val out = Array(3) { DoubleArray(3) }
        for (r in 0..2) {
            for (c in 0..2) {
                var sum = 0.0
                for (k in 0..2) sum += a[r][k] * b[k][c]
                if (!sum.isFinite()) return null
                out[r][c] = sum
            }
        }
        return out
    }

    fun multiply3x3Vector(
        a: Array<DoubleArray>,
        v: DoubleArray,
    ): DoubleArray? {
        if (
            a.size != 3 ||
            a.any { it.size != 3 } ||
            v.size != 3 ||
            v.any { !it.isFinite() }
        ) {
            return null
        }
        val out = DoubleArray(3)
        for (r in 0..2) {
            out[r] = a[r][0] * v[0] + a[r][1] * v[1] + a[r][2] * v[2]
            if (!out[r].isFinite()) return null
        }
        return out
    }

    fun normalize3(v: DoubleArray): DoubleArray? {
        if (v.size != 3 || v.any { !it.isFinite() }) return null
        val n2 = v[0] * v[0] + v[1] * v[1] + v[2] * v[2]
        if (!n2.isFinite() || n2 <= 1.0e-24) return null
        val inv = 1.0 / sqrt(n2)
        return doubleArrayOf(v[0] * inv, v[1] * inv, v[2] * inv)
    }

    fun dot3(a: DoubleArray, b: DoubleArray): Double? {
        if (
            a.size != 3 || b.size != 3 ||
            a.any { !it.isFinite() } || b.any { !it.isFinite() }
        ) {
            return null
        }
        return (a[0] * b[0] + a[1] * b[1] + a[2] * b[2])
            .takeIf(Double::isFinite)
    }
}
