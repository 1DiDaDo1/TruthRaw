package com.truthraw.adaptiveui

import kotlin.math.abs
import kotlin.math.max

/**
 * Deterministic numeric primitives for future scientific uncertainty transport.
 *
 * These helpers transform already-admitted uncertainty only. They do not infer
 * missing covariance, classify residuals as noise, denoise samples, or write
 * Scientific Master.
 */
object ScientificNoiseMathV01 {
    const val METHOD_ID = "D.RAW/ScientificNoiseMath/0.1"
    private const val DIM = 3
    private const val SYMMETRY_TOLERANCE = 1.0e-12
    private const val PSD_RELATIVE_TOLERANCE = 1.0e-12

    fun scaleVariance(
        variance: Double,
        gain: Double,
    ): Double? {
        if (!variance.isFinite() || variance < 0.0 || !gain.isFinite()) {
            return null
        }
        val out = gain * gain * variance
        return out.takeIf { it.isFinite() && it >= 0.0 }
    }

    fun scaleCovariance3x3(
        covariance: Array<DoubleArray>,
        gain: Double,
    ): Array<DoubleArray>? {
        if (!gain.isFinite() || !validCovariance3x3(covariance)) return null
        val factor = gain * gain
        if (!factor.isFinite()) return null
        return Array(DIM) { r ->
            DoubleArray(DIM) { c ->
                covariance[r][c] * factor
            }
        }.takeIf(::validCovariance3x3)
    }

    /**
     * Linear uncertainty transport: C_out = J * C_in * J^T.
     */
    fun transformCovariance3x3(
        covariance: Array<DoubleArray>,
        jacobian: Array<DoubleArray>,
    ): Array<DoubleArray>? {
        if (!validCovariance3x3(covariance) || !validMatrix3x3(jacobian)) {
            return null
        }

        val temp = Array(DIM) { DoubleArray(DIM) }
        for (r in 0 until DIM) {
            for (c in 0 until DIM) {
                var sum = 0.0
                for (k in 0 until DIM) {
                    sum += jacobian[r][k] * covariance[k][c]
                }
                if (!sum.isFinite()) return null
                temp[r][c] = sum
            }
        }

        val out = Array(DIM) { DoubleArray(DIM) }
        for (r in 0 until DIM) {
            for (c in 0 until DIM) {
                var sum = 0.0
                for (k in 0 until DIM) {
                    sum += temp[r][k] * jacobian[c][k]
                }
                if (!sum.isFinite()) return null
                out[r][c] = sum
            }
        }

        // Numeric symmetry is restored only by averaging mirrored results;
        // this does not invent an uncertainty component. The final matrix must
        // still satisfy the full covariance validity/PSD gate below.
        for (r in 0 until DIM) {
            if (out[r][r] < -SYMMETRY_TOLERANCE) return null
            if (out[r][r] < 0.0) out[r][r] = 0.0
            for (c in r + 1 until DIM) {
                val mean = 0.5 * (out[r][c] + out[c][r])
                if (!mean.isFinite()) return null
                out[r][c] = mean
                out[c][r] = mean
            }
        }

        return out.takeIf(::validCovariance3x3)
    }

    fun diagonalCovariance3x3(
        variance0: Double,
        variance1: Double,
        variance2: Double,
    ): Array<DoubleArray>? {
        val values = doubleArrayOf(variance0, variance1, variance2)
        if (values.any { !it.isFinite() || it < 0.0 }) return null
        return Array(DIM) { r ->
            DoubleArray(DIM) { c ->
                if (r == c) values[r] else 0.0
            }
        }.takeIf(::validCovariance3x3)
    }

    /**
     * Returns the noise-gain factor of a literal inverse transfer 1/H only
     * when the transfer magnitude is safely bounded away from zero.
     *
     * This does not authorize deconvolution.
     */
    fun inverseTransferNoiseGain(
        transferMagnitude: Double,
        minimumMagnitude: Double,
    ): Double? {
        if (
            !transferMagnitude.isFinite() ||
            !minimumMagnitude.isFinite() ||
            minimumMagnitude <= 0.0 ||
            transferMagnitude < minimumMagnitude
        ) {
            return null
        }
        val inverse = 1.0 / transferMagnitude
        val gain = inverse * inverse
        return gain.takeIf { it.isFinite() && it >= 0.0 }
    }

    /**
     * A covariance matrix is not valid merely because its diagonal variances
     * are non-negative. It must also be positive semidefinite (PSD), otherwise
     * some real linear combination would have a negative variance.
     *
     * For a real symmetric 3x3 matrix PSD is equivalent to all principal
     * minors being non-negative. We allow only scale-relative floating-point
     * round-off below zero; materially negative minors fail closed. No matrix
     * projection, eigenvalue clipping, or synthetic covariance repair occurs.
     */
    fun positiveSemidefiniteCovariance3x3(
        covariance: Array<DoubleArray>,
    ): Boolean {
        if (!validSymmetricNonnegativeDiagonal3x3(covariance)) return false

        val a = covariance[0][0]
        val b = covariance[1][1]
        val c = covariance[2][2]
        val d = covariance[0][1]
        val e = covariance[0][2]
        val f = covariance[1][2]

        if (!minorNonnegative(a * b - d * d, a * b, d * d)) return false
        if (!minorNonnegative(a * c - e * e, a * c, e * e)) return false
        if (!minorNonnegative(b * c - f * f, b * c, f * f)) return false

        val abc = a * b * c
        val twoDef = 2.0 * d * e * f
        val af2 = a * f * f
        val be2 = b * e * e
        val cd2 = c * d * d
        val determinant = abc + twoDef - af2 - be2 - cd2
        if (!determinant.isFinite()) return false

        val detScale =
            max(
                1.0,
                max(
                    max(abs(abc), abs(twoDef)),
                    max(max(abs(af2), abs(be2)), abs(cd2)),
                ),
            )
        return determinant >= -PSD_RELATIVE_TOLERANCE * detScale
    }

    fun validCovariance3x3(
        covariance: Array<DoubleArray>,
    ): Boolean = positiveSemidefiniteCovariance3x3(covariance)

    private fun validSymmetricNonnegativeDiagonal3x3(
        covariance: Array<DoubleArray>,
    ): Boolean {
        if (!validMatrix3x3(covariance)) return false
        for (r in 0 until DIM) {
            if (covariance[r][r] < 0.0) return false
            for (c in r + 1 until DIM) {
                val x = covariance[r][c]
                val y = covariance[c][r]
                val scale = 1.0 + maxOf(abs(x), abs(y))
                if (abs(x - y) > SYMMETRY_TOLERANCE * scale) return false
            }
        }
        return true
    }

    private fun minorNonnegative(
        minor: Double,
        term0: Double,
        term1: Double,
    ): Boolean {
        if (!minor.isFinite() || !term0.isFinite() || !term1.isFinite()) {
            return false
        }
        val scale = max(1.0, max(abs(term0), abs(term1)))
        return minor >= -PSD_RELATIVE_TOLERANCE * scale
    }

    private fun validMatrix3x3(
        matrix: Array<DoubleArray>,
    ): Boolean =
        matrix.size == DIM &&
            matrix.all { row ->
                row.size == DIM && row.all(Double::isFinite)
            }

    private fun allFinite(
        matrix: Array<DoubleArray>,
    ): Boolean =
        matrix.all { row -> row.all(Double::isFinite) }
}
