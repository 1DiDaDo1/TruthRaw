package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.abs
import kotlin.math.ln
import kotlin.math.sqrt

/**
 * Read-only repeatability audit over multiple independent PR96 optical-field
 * observations.
 *
 * The audit removes only one scalar exposure/scene level per observation and
 * compares the remaining radial/azimuth/CFA-phase shape. It does not identify
 * a camera or lens, does not separate scene illumination from optics/sensor
 * response, and cannot authorize correction.
 */
object FieldResponseRepeatabilityV01 {
    const val SCHEMA = "D.RAW/FieldResponseRepeatability/0.1"

    private const val MIN_OBSERVATIONS = 3
    private const val RADIAL_BINS = 12
    private const val AZIMUTH_BINS = 12
    private const val CFA_PHASES = 4
    private const val EPS = 1.0e-12
    private const val LN2 = 0.6931471805599453

    private data class Prepared(
        val sourceSha256: String,
        val sampleCount: Int,
        val scalarReference: Double,
        val radialRelativeEv: DoubleArray,
        val azimuthRelativeEv: Array<DoubleArray>,
        val cfaPhaseRelativeEv: Array<DoubleArray>,
        val observedOuterToInnerRatio: Double,
    )

    fun evaluate(fieldCharts: List<JSONObject>): JSONObject {
        val unique = linkedMapOf<String, JSONObject>()
        for (chart in fieldCharts) {
            if (
                chart.optString("schema") !=
                "D.RAW/ObservationOpticalFieldChart/0.1" ||
                chart.optString("status") != "FIELD_CHART_AVAILABLE"
            ) {
                continue
            }
            val sha = chart.optString("source_sha256")
            if (sha.isBlank()) continue
            val signal = chart.optJSONObject("measured_composite_field_signal")
            if (
                signal?.optString("status") !=
                "MEASURED_COMPOSITE_FIELD_SIGNAL_AVAILABLE"
            ) {
                continue
            }
            unique.putIfAbsent(sha, chart)
        }

        if (unique.size < MIN_OBSERVATIONS) {
            return unavailable(
                "INSUFFICIENT_DISTINCT_MEASURED_FIELD_OBSERVATIONS",
                unique.keys.toList(),
            )
        }

        val prepared = unique.values.mapNotNull(::prepare)
        if (prepared.size < MIN_OBSERVATIONS) {
            return unavailable(
                "INSUFFICIENT_POSITIVE_COMMON_FIELD_SIGNAL",
                prepared.map { it.sourceSha256 },
            )
        }

        val radialBins = JSONArray()
        val radialMadValues = ArrayList<Double>()
        for (bin in 0 until RADIAL_BINS) {
            val values =
                prepared
                    .map { it.radialRelativeEv[bin] }
                    .filter { it.isFinite() }
                    .sorted()
            val med = median(values)
            val mad = mad(values, med)
            if (mad.isFinite()) radialMadValues += mad
            radialBins.put(
                JSONObject()
                    .put("bin", bin)
                    .put("rho_min", bin.toDouble() / RADIAL_BINS.toDouble())
                    .put("rho_max", (bin + 1).toDouble() / RADIAL_BINS.toDouble())
                    .put("valid_observation_count", values.size)
                    .put(
                        "median_relative_signal_ev",
                        finiteOrNull(med),
                    )
                    .put(
                        "cross_observation_mad_ev",
                        finiteOrNull(mad),
                    ),
            )
        }

        val pairwise = JSONArray()
        val correlationValues = ArrayList<Double>()
        for (i in 0 until prepared.size) {
            for (j in i + 1 until prepared.size) {
                val corr =
                    pearson(
                        prepared[i].radialRelativeEv,
                        prepared[j].radialRelativeEv,
                    )
                if (corr.isFinite()) correlationValues += corr
                pairwise.put(
                    JSONObject()
                        .put("left_source_sha256", prepared[i].sourceSha256)
                        .put("right_source_sha256", prepared[j].sourceSha256)
                        .put(
                            "radial_shape_pearson",
                            finiteOrNull(corr),
                        ),
                )
            }
        }

        val azimuthAnnuli = JSONArray()
        val allAzimuthCellMad = ArrayList<Double>()
        val outerAzimuthCellMad = ArrayList<Double>()
        for (bin in 0 until RADIAL_BINS) {
            val sectors = JSONArray()
            for (sector in 0 until AZIMUTH_BINS) {
                val values =
                    prepared
                        .map { it.azimuthRelativeEv[bin][sector] }
                        .filter { it.isFinite() }
                        .sorted()
                val med = median(values)
                val mad = mad(values, med)
                if (mad.isFinite()) {
                    allAzimuthCellMad += mad
                    if (bin >= RADIAL_BINS / 2) {
                        outerAzimuthCellMad += mad
                    }
                }
                sectors.put(
                    JSONObject()
                        .put("sector", sector)
                        .put("valid_observation_count", values.size)
                        .put("median_relative_to_annulus_ev", finiteOrNull(med))
                        .put("cross_observation_mad_ev", finiteOrNull(mad)),
                )
            }
            azimuthAnnuli.put(
                JSONObject()
                    .put("bin", bin)
                    .put("sectors", sectors),
            )
        }

        val phaseAnnuli = JSONArray()
        val allPhaseMad = ArrayList<Double>()
        for (bin in 0 until RADIAL_BINS) {
            val phases = JSONArray()
            for (phase in 0 until CFA_PHASES) {
                val values =
                    prepared
                        .map { it.cfaPhaseRelativeEv[phase][bin] }
                        .filter { it.isFinite() }
                        .sorted()
                val med = median(values)
                val mad = mad(values, med)
                if (mad.isFinite()) allPhaseMad += mad
                phases.put(
                    JSONObject()
                        .put("phase_index", phase)
                        .put("valid_observation_count", values.size)
                        .put("median_relative_to_annulus_ev", finiteOrNull(med))
                        .put("cross_observation_mad_ev", finiteOrNull(mad)),
                )
            }
            phaseAnnuli.put(
                JSONObject()
                    .put("bin", bin)
                    .put("phases", phases),
            )
        }

        val observations = JSONArray()
        for (p in prepared) {
            observations.put(
                JSONObject()
                    .put("source_sha256", p.sourceSha256)
                    .put("sample_count", p.sampleCount)
                    .put(
                        "per_observation_scalar_reference_p50",
                        p.scalarReference,
                    )
                    .put(
                        "observed_outer_to_inner_p50_ratio",
                        finiteOrNull(p.observedOuterToInnerRatio),
                    ),
            )
        }

        val correlationSorted = correlationValues.sorted()
        val radialMadSorted = radialMadValues.sorted()
        val azimuthMadSorted = allAzimuthCellMad.sorted()
        val outerAzimuthMadSorted = outerAzimuthCellMad.sorted()
        val phaseMadSorted = allPhaseMad.sorted()

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "READ_ONLY_REPEATABILITY_AUDIT_AVAILABLE")
            .put("observation_count", prepared.size)
            .put("minimum_observation_count", MIN_OBSERVATIONS)
            .put("observation_roots", observations)
            .put(
                "relation_context",
                JSONObject()
                    .put(
                        "protocol_schema",
                        UniversalMultiObservationRelationProtocolV01.SCHEMA,
                    )
                    .put(
                        "relation_evidence_class",
                        "USER_GROUPING_HINT_ONLY",
                    )
                    .put(
                        "relation_authority",
                        "NON_AUTHORITY_GROUPING_HINT",
                    )
                    .put("same_physical_camera_proven", false)
                    .put("same_physical_lens_proven", false)
                    .put("camera_model_name_used", false)
                    .put("lens_model_name_used", false)
                    .put("vendor_mapping_used", false),
            )
            .put(
                "normalization",
                JSONObject()
                    .put(
                        "per_observation_scalar",
                        "MEDIAN_OF_POSITIVE_ANNULUS_P50",
                    )
                    .put(
                        "radial_value",
                        "log2(annulus_p50/per_observation_scalar)",
                    )
                    .put(
                        "azimuth_value",
                        "log2(sector_p50/annulus_p50)",
                    )
                    .put(
                        "cfa_phase_value",
                        "log2(phase_p50/annulus_p50)",
                    )
                    .put("exposure_metadata_used", false)
                    .put("white_balance_used", false)
                    .put("gain_map_used", false)
                    .put("scene_content_removed", false),
            )
            .put(
                "radial_repeatability",
                JSONObject()
                    .put("annuli", radialBins)
                    .put("pairwise_radial_shape", pairwise)
                    .put(
                        "median_annulus_cross_observation_mad_ev",
                        finiteOrNull(median(radialMadSorted)),
                    )
                    .put(
                        "max_annulus_cross_observation_mad_ev",
                        finiteOrNull(radialMadSorted.maxOrNull() ?: Double.NaN),
                    )
                    .put(
                        "median_pairwise_radial_shape_pearson",
                        finiteOrNull(median(correlationSorted)),
                    )
                    .put(
                        "min_pairwise_radial_shape_pearson",
                        finiteOrNull(correlationSorted.minOrNull() ?: Double.NaN),
                    ),
            )
            .put(
                "azimuth_repeatability",
                JSONObject()
                    .put("annuli", azimuthAnnuli)
                    .put(
                        "median_cell_cross_observation_mad_ev",
                        finiteOrNull(median(azimuthMadSorted)),
                    )
                    .put(
                        "outer_half_median_cell_cross_observation_mad_ev",
                        finiteOrNull(median(outerAzimuthMadSorted)),
                    ),
            )
            .put(
                "cfa_phase_repeatability",
                JSONObject()
                    .put("annuli", phaseAnnuli)
                    .put(
                        "median_phase_bin_cross_observation_mad_ev",
                        finiteOrNull(median(phaseMadSorted)),
                    ),
            )
            .put(
                "interpretation",
                JSONObject()
                    .put(
                        "authority",
                        "OBSERVED_MULTI_SCENE_FIELD_REPEATABILITY_DESCRIPTIVE_ONLY",
                    )
                    .put("repeatable_camera_system_response_proven", false)
                    .put("lens_only_vignetting_proven", false)
                    .put("scene_illumination_separated", false)
                    .put("sensor_angular_response_separated", false)
                    .put("optical_axis_proven", false)
                    .put("calibration_promoted", false)
                    .put("correction_gain_allowed", false)
                    .put("automatic_threshold_or_winner_used", false)
                    .put(
                        "note",
                        "Low dispersion or high correlation can motivate a later controlled relation experiment, but scene structure can also repeat. This audit therefore reports metrics without promoting a physical cause.",
                    ),
            )
            .put("source_sample_values_modified", false)
            .put("source_sample_positions_modified", false)
            .put("new_measured_samples_created", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun prepare(chart: JSONObject): Prepared? {
        val sourceSha = chart.optString("source_sha256")
        val signal = chart.optJSONObject("measured_composite_field_signal")
            ?: return null
        val annuli = signal.optJSONArray("annuli") ?: return null
        if (
            sourceSha.isBlank() ||
            signal.optInt("radial_bin_count", -1) != RADIAL_BINS ||
            signal.optInt("azimuth_bin_count", -1) != AZIMUTH_BINS ||
            annuli.length() != RADIAL_BINS
        ) {
            return null
        }

        val rawP50 = DoubleArray(RADIAL_BINS) { Double.NaN }
        for (bin in 0 until RADIAL_BINS) {
            val a = annuli.optJSONObject(bin) ?: continue
            rawP50[bin] =
                a.optDouble(
                    "p50_normalized_above_black",
                    Double.NaN,
                )
        }

        val positive = rawP50.filter { it.isFinite() && it > EPS }.sorted()
        val scale = median(positive)
        if (!scale.isFinite() || scale <= EPS || positive.size < 6) {
            return null
        }

        val radialEv =
            DoubleArray(RADIAL_BINS) { bin ->
                safeLog2Ratio(rawP50[bin], scale)
            }

        val azimuth =
            Array(RADIAL_BINS) {
                DoubleArray(AZIMUTH_BINS) { Double.NaN }
            }
        val phase =
            Array(CFA_PHASES) {
                DoubleArray(RADIAL_BINS) { Double.NaN }
            }

        for (bin in 0 until RADIAL_BINS) {
            val a = annuli.optJSONObject(bin) ?: continue
            val annulusP50 = rawP50[bin]
            if (!annulusP50.isFinite() || annulusP50 <= EPS) continue

            val sectors = a.optJSONArray("azimuth_sectors")
            if (sectors != null) {
                for (sector in 0 until minOf(AZIMUTH_BINS, sectors.length())) {
                    val s = sectors.optJSONObject(sector) ?: continue
                    azimuth[bin][sector] =
                        safeLog2Ratio(
                            s.optDouble(
                                "p50_normalized_above_black",
                                Double.NaN,
                            ),
                            annulusP50,
                        )
                }
            }

            val phaseP50 = a.optJSONArray("cfa_phase_p50")
            if (phaseP50 != null) {
                for (p in 0 until minOf(CFA_PHASES, phaseP50.length())) {
                    phase[p][bin] =
                        safeLog2Ratio(
                            phaseP50.optDouble(p, Double.NaN),
                            annulusP50,
                        )
                }
            }
        }

        return Prepared(
            sourceSha256 = sourceSha,
            sampleCount = signal.optInt("sample_count", 0),
            scalarReference = scale,
            radialRelativeEv = radialEv,
            azimuthRelativeEv = azimuth,
            cfaPhaseRelativeEv = phase,
            observedOuterToInnerRatio =
                signal.optDouble(
                    "observed_outer_to_inner_p50_ratio",
                    Double.NaN,
                ),
        )
    }

    private fun safeLog2Ratio(
        value: Double,
        reference: Double,
    ): Double {
        if (
            !value.isFinite() ||
            !reference.isFinite() ||
            value <= EPS ||
            reference <= EPS
        ) {
            return Double.NaN
        }
        return ln(value / reference) / LN2
    }

    private fun pearson(
        a: DoubleArray,
        b: DoubleArray,
    ): Double {
        val pairs = ArrayList<Pair<Double, Double>>()
        for (i in a.indices) {
            if (i >= b.size) break
            if (a[i].isFinite() && b[i].isFinite()) {
                pairs += a[i] to b[i]
            }
        }
        if (pairs.size < 4) return Double.NaN

        val meanA = pairs.sumOf { it.first } / pairs.size.toDouble()
        val meanB = pairs.sumOf { it.second } / pairs.size.toDouble()
        var cov = 0.0
        var va = 0.0
        var vb = 0.0
        for ((x, y) in pairs) {
            val dx = x - meanA
            val dy = y - meanB
            cov += dx * dy
            va += dx * dx
            vb += dy * dy
        }
        val den = sqrt(va * vb)
        return if (den > EPS) cov / den else Double.NaN
    }

    private fun median(values: List<Double>): Double {
        if (values.isEmpty()) return Double.NaN
        val sorted = values.sorted()
        val n = sorted.size
        return if (n % 2 == 1) {
            sorted[n / 2]
        } else {
            (sorted[n / 2 - 1] + sorted[n / 2]) * 0.5
        }
    }

    private fun mad(
        values: List<Double>,
        center: Double,
    ): Double {
        if (values.isEmpty() || !center.isFinite()) return Double.NaN
        return median(values.map { abs(it - center) })
    }

    private fun finiteOrNull(value: Double): Any =
        if (value.isFinite()) value else JSONObject.NULL

    private fun unavailable(
        reason: String,
        roots: List<String>,
    ): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("minimum_observation_count", MIN_OBSERVATIONS)
            .put("available_distinct_observation_count", roots.distinct().size)
            .put("observation_source_sha256", JSONArray(roots.distinct()))
            .put("same_physical_camera_proven", false)
            .put("same_physical_lens_proven", false)
            .put("calibration_promoted", false)
            .put("correction_gain_allowed", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}
