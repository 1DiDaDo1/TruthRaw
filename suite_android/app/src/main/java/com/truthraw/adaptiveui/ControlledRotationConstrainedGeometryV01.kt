package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.abs
import kotlin.math.atan2
import kotlin.math.cos
import kotlin.math.max
import kotlin.math.min
import kotlin.math.sin
import kotlin.math.sqrt

/**
 * Deterministic rotation-constrained geometry diagnostic for explicit
 * controlled 0/90/180/270 FIELD_RESPONSE relation records.
 *
 * The relation record supplies the nominal quarter-turn. Appearance features
 * may estimate only residual similarity terms around that nominal transform:
 * small residual rotation, translation and uniform scale. Shear, anisotropic
 * scale, reflection and projective terms are forbidden by construction.
 *
 * This is a diagnostic candidate only. It never changes the field solver,
 * never resamples source pixels, never promotes registration/calibration and
 * never writes to the Scientific Master.
 */
object ControlledRotationConstrainedGeometryV01 {
    const val SCHEMA =
        "D.RAW/ControlledRotationConstrainedGeometry/0.1"

    private const val MAX_DESCRIPTOR_DISTANCE = 64
    private const val TOP_DESCRIPTOR_CANDIDATES_PER_LEFT = 6
    private const val MAX_TRANSLATION_HYPOTHESIS = 0.22
    private const val TRANSLATION_SUPPORT_RADIUS = 0.045
    private const val MIN_MATCHES = 4
    private const val MAD_SCALE = 1.4826
    private const val MIN_ROBUST_RESIDUAL = 0.006

    // Broad diagnostic bounds only. They do not authorize promotion.
    private const val MAX_ABS_RESIDUAL_ROTATION_DEG = 15.0
    private const val MIN_UNIFORM_SCALE = 0.80
    private const val MAX_UNIFORM_SCALE = 1.20
    private const val MAX_CENTER_DISPLACEMENT = 0.15
    private const val MAX_RMS_RESIDUAL = 0.035

    fun evaluate(
        profiles: List<JSONObject>,
        records: List<JSONObject>,
    ): JSONObject {
        val reports = JSONArray()
        var admittedCount = 0

        for ((index, record) in records.withIndex()) {
            if (record.optString("axis_scope") != "FIELD_RESPONSE") continue
            val payload = record.optJSONObject("axis_payload") ?: continue
            if (!payload.optBoolean("controlled_rotation_relation", false)) continue

            val admission =
                CalibrationObservationAdmissionV01.admitForNumericCandidate(
                    record = record,
                    axis = "FIELD_RESPONSE",
                )
            if (
                admission.optString("status") !=
                "NUMERIC_CANDIDATE_RELATION_ADMITTED"
            ) {
                continue
            }

            admittedCount++
            val report =
                evaluate(
                    profiles = profiles,
                    record = record,
                )
            report.put("record_index", index)
            reports.put(report)
        }

        if (admittedCount == 0) {
            return unavailable(
                "NO_ADMITTED_CONTROLLED_ROTATION_RELATION_RECORD",
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                "CONTROLLED_ROTATION_CONSTRAINED_GEOMETRY_AUDIT_SET_AVAILABLE",
            )
            .put("admitted_record_count", admittedCount)
            .put("record_reports", reports)
            .put(
                "authority_boundary",
                JSONObject()
                    .put(
                        "nominal_relation_used_as_geometry_constraint",
                        true,
                    )
                    .put(
                        "unconstrained_affine_may_override_nominal_relation",
                        false,
                    )
                    .put(
                        "registration_adjusted_field_solver_executed",
                        false,
                    )
                    .put("world_registration_promoted", false)
                    .put(
                        "field_response_calibration_promoted",
                        false,
                    )
                    .put("correction_authorized", false),
            )
            .put("image_transform_applied", false)
            .put("creates_sensor_evidence", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private data class Feature(
        val index: Int,
        val xIso: Double,
        val yIso: Double,
        val orientationDeg: Double,
        val descriptor: ByteArray,
    )

    private data class PairCandidate(
        val left: Feature,
        val right: Feature,
        val distance: Int,
        val rotatedLeftX: Double,
        val rotatedLeftY: Double,
        val tx: Double,
        val ty: Double,
    )

    private data class Match(
        val left: Feature,
        val right: Feature,
        val descriptorDistance: Int,
        val nominalX: Double,
        val nominalY: Double,
    )

    private data class Similarity(
        val a: Double,
        val b: Double,
        val tx: Double,
        val ty: Double,
    ) {
        val scale: Double
            get() = sqrt(a * a + b * b)
        val residualRotationDeg: Double
            get() = Math.toDegrees(atan2(b, a))
    }

    fun evaluate(
        profiles: List<JSONObject>,
        record: JSONObject,
    ): JSONObject {
        val payload =
            record.optJSONObject("axis_payload")
                ?: return unavailable("AXIS_PAYLOAD_MISSING")
        if (
            record.optString("axis_scope") != "FIELD_RESPONSE" ||
            !payload.optBoolean("controlled_rotation_relation", false)
        ) {
            return unavailable(
                "CONTROLLED_FIELD_ROTATION_RELATION_REQUIRED",
            )
        }

        val admission =
            CalibrationObservationAdmissionV01.admitForNumericCandidate(
                record = record,
                axis = "FIELD_RESPONSE",
            )
        if (
            admission.optString("status") !=
            "NUMERIC_CANDIDATE_RELATION_ADMITTED"
        ) {
            return unavailable(
                "CONTROLLED_ROTATION_RECORD_NOT_NUMERICALLY_ADMITTED",
            )
        }

        val selectedShiftSign =
            payload.optJSONObject("rotation_mapping")
                ?.optInt("selected_shift_sign", 0)
                ?: 0
        if (
            selectedShiftSign != 1 &&
            selectedShiftSign != -1
        ) {
            return unavailable(
                "SELECTED_QUARTER_TURN_SHIFT_SIGN_REQUIRED",
            )
        }

        val sourceRoles = sourceRoles(payload)
        val anchorSource =
            sourceRoles.entries
                .firstOrNull { it.value.second == 0 }
                ?.key
                ?: return unavailable(
                    "ROTATION_0_DEG_ANCHOR_REQUIRED",
                )

        val bySource =
            profiles
                .mapNotNull { profile ->
                    val sha =
                        profile.optString("source_sha256")
                            .trim()
                            .lowercase()
                    if (sha.isBlank()) null else sha to profile
                }
                .toMap()

        val anchorProfile =
            bySource[anchorSource]
                ?: return unavailable(
                    "ROTATION_0_DEG_PROFILE_MISSING",
                )
        val anchorFront =
            anchorProfile.optJSONObject("scene_analysis")
                ?: return unavailable(
                    "ROTATION_0_DEG_SCENE_ANALYSIS_MISSING",
                )
        val anchorFeatures =
            readFeatures(anchorFront)
        val anchorFeatureSupportSource =
            featureSupportSource(anchorFront)
        if (anchorFeatures.size < MIN_MATCHES) {
            return unavailable(
                "ROTATION_0_DEG_FEATURE_SUPPORT_TOO_LOW",
            )
        }

        val observations = JSONArray()
        observations.put(
            JSONObject()
                .put("source_sha256", anchorSource)
                .put("observation_role", sourceRoles[anchorSource]?.first)
                .put("quarter_turn_index", 0)
                .put("nominal_rotation_degrees", 0.0)
                .put(
                    "status",
                    "ROTATION_CONSTRAINED_ANCHOR_IDENTITY",
                )
                .put("residual_rotation_degrees", 0.0)
                .put("fitted_total_rotation_degrees", 0.0)
                .put("uniform_scale", 1.0)
                .put("translation_x_isotropic", 0.0)
                .put("translation_y_isotropic", 0.0)
                .put("center_displacement_isotropic", 0.0)
                .put("rms_residual_isotropic", 0.0)
                .put("robust_match_count", anchorFeatures.size)
                .put(
                    "feature_support_source",
                    anchorFeatureSupportSource,
                )
                .put(
                    "feature_support_count",
                    anchorFeatures.size,
                )
                .put("diagnostic_bounds_pass", true)
                .put("candidate_applied", false)
                .put("scientific_writeback_allowed", false),
        )

        var availableCount = 0
        var rejectedCount = 0
        var unavailableCount = 0

        for (
            entry in sourceRoles.entries
                .filter { it.value.second > 0 }
                .sortedBy { it.value.second }
        ) {
            val source = entry.key
            val role = entry.value.first
            val q = entry.value.second
            val profile = bySource[source]
            if (profile == null) {
                observations.put(
                    observationUnavailable(
                        source,
                        role,
                        q,
                        "ACTIVE_PROFILE_MISSING",
                    ),
                )
                unavailableCount++
                continue
            }
            val rightFront =
                profile.optJSONObject("scene_analysis")
                    ?: JSONObject()
            val nominalRotationDeg =
                -selectedShiftSign *
                    q.toDouble() *
                    90.0
            val report =
                evaluatePair(
                    leftFrontside = anchorFront,
                    rightFrontside = rightFront,
                    leftSource = anchorSource,
                    rightSource = source,
                    role = role,
                    quarterTurnIndex = q,
                    nominalRotationDeg = nominalRotationDeg,
                )
            observations.put(report)
            when (report.optString("status")) {
                "ROTATION_CONSTRAINED_SIMILARITY_CANDIDATE_AVAILABLE" ->
                    availableCount++
                "ROTATION_CONSTRAINED_DIAGNOSTIC_OUTSIDE_BOUNDS" ->
                    rejectedCount++
                else -> unavailableCount++
            }
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                "CONTROLLED_ROTATION_CONSTRAINED_GEOMETRY_AUDIT_AVAILABLE",
            )
            .put("anchor_source_sha256", anchorSource)
            .put("selected_shift_sign", selectedShiftSign)
            .put("observation_diagnostics", observations)
            .put(
                "candidate_available_observation_count",
                availableCount,
            )
            .put(
                "candidate_outside_bounds_observation_count",
                rejectedCount,
            )
            .put(
                "candidate_unavailable_observation_count",
                unavailableCount,
            )
            .put(
                "constraint_contract",
                JSONObject()
                    .put(
                        "nominal_relation_source",
                        "EXPLICIT_CONTROLLED_ROTATION_RELATION_RECORD",
                    )
                    .put(
                        "nominal_relation_used_as_geometry_constraint",
                        true,
                    )
                    .put(
                        "residual_rotation_estimation_allowed",
                        true,
                    )
                    .put("translation_estimation_allowed", true)
                    .put("uniform_scale_estimation_allowed", true)
                    .put("shear_allowed", false)
                    .put("anisotropic_scale_allowed", false)
                    .put("reflection_allowed", false)
                    .put("projective_terms_allowed", false)
                    .put(
                        "unconstrained_affine_may_override_nominal_relation",
                        false,
                    )
                    .put(
                        "rotation_support_keypoints_preferred",
                        true,
                    )
                    .put(
                        "rotation_support_changes_primary_pair_geometry",
                        false,
                    ),
            )
            .put(
                "diagnostic_bounds",
                JSONObject()
                    .put(
                        "minimum_robust_match_count",
                        MIN_MATCHES,
                    )
                    .put(
                        "maximum_abs_residual_rotation_degrees",
                        MAX_ABS_RESIDUAL_ROTATION_DEG,
                    )
                    .put(
                        "minimum_uniform_scale",
                        MIN_UNIFORM_SCALE,
                    )
                    .put(
                        "maximum_uniform_scale",
                        MAX_UNIFORM_SCALE,
                    )
                    .put(
                        "maximum_center_displacement_isotropic",
                        MAX_CENTER_DISPLACEMENT,
                    )
                    .put(
                        "maximum_rms_residual_isotropic",
                        MAX_RMS_RESIDUAL,
                    )
                    .put(
                        "bounds_are_scientific_promotion_thresholds",
                        false,
                    ),
            )
            .put(
                "authority_boundary",
                JSONObject()
                    .put(
                        "appearance_geometry_is_world_registration_proof",
                        false,
                    )
                    .put(
                        "controlled_rotation_label_is_sensor_evidence",
                        false,
                    )
                    .put(
                        "geometry_deviation_explains_field_error_proven",
                        false,
                    )
                    .put(
                        "registration_adjusted_field_solver_executed",
                        false,
                    )
                    .put("world_registration_promoted", false)
                    .put(
                        "field_response_calibration_promoted",
                        false,
                    )
                    .put("correction_authorized", false),
            )
            .put("image_transform_applied", false)
            .put("source_sample_values_modified", false)
            .put("source_sample_positions_modified", false)
            .put("creates_sensor_evidence", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun evaluatePair(
        leftFrontside: JSONObject,
        rightFrontside: JSONObject,
        leftSource: String,
        rightSource: String,
        role: String,
        quarterTurnIndex: Int,
        nominalRotationDeg: Double,
    ): JSONObject {
        val left = readFeatures(leftFrontside)
        val right = readFeatures(rightFrontside)
        val leftFeatureSupportSource =
            featureSupportSource(leftFrontside)
        val rightFeatureSupportSource =
            featureSupportSource(rightFrontside)
        if (
            left.size < MIN_MATCHES ||
            right.size < MIN_MATCHES
        ) {
            return observationUnavailable(
                rightSource,
                role,
                quarterTurnIndex,
                "INSUFFICIENT_LOCAL_FEATURES",
                nominalRotationDeg,
            )
        }

        val nominalRadians =
            Math.toRadians(nominalRotationDeg)
        val c = cos(nominalRadians)
        val s = sin(nominalRadians)

        val candidates = ArrayList<PairCandidate>()
        for (l in left) {
            val rx = c * l.xIso - s * l.yIso
            val ry = s * l.xIso + c * l.yIso
            val ranked =
                right
                    .map { r ->
                        r to hamming(
                            l.descriptor,
                            r.descriptor,
                        )
                    }
                    .filter {
                        it.second <= MAX_DESCRIPTOR_DISTANCE
                    }
                    .sortedWith(
                        compareBy<Pair<Feature, Int>> {
                            it.second
                        }.thenBy {
                            it.first.index
                        },
                    )
                    .take(
                        TOP_DESCRIPTOR_CANDIDATES_PER_LEFT,
                    )
            for ((r, distance) in ranked) {
                val tx = r.xIso - rx
                val ty = r.yIso - ry
                if (
                    sqrt(tx * tx + ty * ty) >
                    MAX_TRANSLATION_HYPOTHESIS
                ) {
                    continue
                }
                candidates +=
                    PairCandidate(
                        left = l,
                        right = r,
                        distance = distance,
                        rotatedLeftX = rx,
                        rotatedLeftY = ry,
                        tx = tx,
                        ty = ty,
                    )
            }
        }

        if (candidates.isEmpty()) {
            return observationUnavailable(
                rightSource,
                role,
                quarterTurnIndex,
                "NO_DESCRIPTOR_CANDIDATES_WITHIN_ROTATION_CONSTRAINT",
                nominalRotationDeg,
            )
        }

        var best: List<Match> = emptyList()
        var bestRms = Double.POSITIVE_INFINITY
        var bestDescriptorSum = Int.MAX_VALUE
        var bestTranslationMagnitude =
            Double.POSITIVE_INFINITY

        for (hypothesis in candidates) {
            val proposed = ArrayList<PairCandidate>()
            for (l in left) {
                val perLeft =
                    candidates
                        .asSequence()
                        .filter {
                            it.left.index == l.index
                        }
                        .map { candidate ->
                            val dx =
                                candidate.tx -
                                    hypothesis.tx
                            val dy =
                                candidate.ty -
                                    hypothesis.ty
                            val residual =
                                sqrt(dx * dx + dy * dy)
                            candidate to residual
                        }
                        .filter {
                            it.second <=
                                TRANSLATION_SUPPORT_RADIUS
                        }
                        .sortedWith(
                            compareBy<Pair<PairCandidate, Double>> {
                                it.second
                            }.thenBy {
                                it.first.distance
                            }.thenBy {
                                it.first.right.index
                            },
                        )
                        .firstOrNull()
                if (perLeft != null) {
                    proposed += perLeft.first
                }
            }

            val unique =
                proposed
                    .sortedWith(
                        compareBy<PairCandidate> {
                            val dx = it.tx - hypothesis.tx
                            val dy = it.ty - hypothesis.ty
                            sqrt(dx * dx + dy * dy)
                        }.thenBy {
                            it.distance
                        }.thenBy {
                            it.left.index
                        }.thenBy {
                            it.right.index
                        },
                    )
                    .fold(
                        Pair(
                            linkedSetOf<Int>(),
                            ArrayList<Match>(),
                        ),
                    ) { acc, candidate ->
                        if (
                            candidate.right.index !in
                            acc.first
                        ) {
                            acc.first += candidate.right.index
                            acc.second +=
                                Match(
                                    left = candidate.left,
                                    right = candidate.right,
                                    descriptorDistance =
                                        candidate.distance,
                                    nominalX =
                                        candidate.rotatedLeftX,
                                    nominalY =
                                        candidate.rotatedLeftY,
                                )
                        }
                        acc
                    }
                    .second

            if (unique.size < MIN_MATCHES) continue

            val rms =
                sqrt(
                    unique.sumOf { match ->
                        val dx =
                            match.right.xIso -
                                (match.nominalX +
                                    hypothesis.tx)
                        val dy =
                            match.right.yIso -
                                (match.nominalY +
                                    hypothesis.ty)
                        dx * dx + dy * dy
                    } / unique.size.toDouble(),
                )
            val descriptorSum =
                unique.sumOf {
                    it.descriptorDistance
                }
            val translationMagnitude =
                sqrt(
                    hypothesis.tx * hypothesis.tx +
                        hypothesis.ty * hypothesis.ty,
                )

            val better =
                unique.size > best.size ||
                    (
                        unique.size == best.size &&
                            rms < bestRms - 1.0e-12
                        ) ||
                    (
                        unique.size == best.size &&
                            abs(rms - bestRms) <= 1.0e-12 &&
                            descriptorSum < bestDescriptorSum
                        ) ||
                    (
                        unique.size == best.size &&
                            abs(rms - bestRms) <= 1.0e-12 &&
                            descriptorSum == bestDescriptorSum &&
                            translationMagnitude <
                            bestTranslationMagnitude
                        )
            if (better) {
                best = unique
                bestRms = rms
                bestDescriptorSum = descriptorSum
                bestTranslationMagnitude =
                    translationMagnitude
            }
        }

        if (best.size < MIN_MATCHES) {
            return observationUnavailable(
                rightSource,
                role,
                quarterTurnIndex,
                "ROTATION_CONSTRAINED_TRANSLATION_CLUSTER_TOO_SMALL",
                nominalRotationDeg,
            )
        }

        val firstFit =
            fitResidualSimilarity(best)
                ?: return observationUnavailable(
                    rightSource,
                    role,
                    quarterTurnIndex,
                    "ROTATION_CONSTRAINED_SIMILARITY_SINGULAR",
                    nominalRotationDeg,
                )
        val firstResiduals =
            best.map {
                residual(firstFit, it)
            }
        val med0 = median(firstResiduals)
        val mad0 =
            median(
                firstResiduals.map {
                    abs(it - med0)
                },
            )
        val robustThreshold =
            max(
                MIN_ROBUST_RESIDUAL,
                med0 + 3.0 * MAD_SCALE * mad0,
            )
        val inliers =
            best.filter {
                residual(firstFit, it) <=
                    robustThreshold
            }
        if (inliers.size < MIN_MATCHES) {
            return observationUnavailable(
                rightSource,
                role,
                quarterTurnIndex,
                "ROTATION_CONSTRAINED_ROBUST_INLIER_COUNT_TOO_LOW",
                nominalRotationDeg,
            )
        }

        val fit =
            fitResidualSimilarity(inliers)
                ?: return observationUnavailable(
                    rightSource,
                    role,
                    quarterTurnIndex,
                    "ROTATION_CONSTRAINED_ROBUST_REFIT_SINGULAR",
                    nominalRotationDeg,
                )
        val residuals =
            inliers
                .map { residual(fit, it) }
                .sorted()
        val rms =
            sqrt(
                residuals.sumOf { it * it } /
                    residuals.size.toDouble(),
            )
        val fittedTotalRotation =
            wrap180(
                nominalRotationDeg +
                    fit.residualRotationDeg,
            )
        val centerDisplacement =
            sqrt(fit.tx * fit.tx + fit.ty * fit.ty)
        val orientationResiduals =
            inliers.map { match ->
                abs(
                    wrap180(
                        match.right.orientationDeg -
                            match.left.orientationDeg -
                            fittedTotalRotation,
                    ),
                )
            }.sorted()
        val descriptorDistances =
            inliers.map {
                it.descriptorDistance.toDouble()
            }.sorted()

        val withinBounds =
            abs(fit.residualRotationDeg) <=
                MAX_ABS_RESIDUAL_ROTATION_DEG &&
                fit.scale in
                MIN_UNIFORM_SCALE..MAX_UNIFORM_SCALE &&
                centerDisplacement <=
                MAX_CENTER_DISPLACEMENT &&
                rms <= MAX_RMS_RESIDUAL

        return JSONObject()
            .put("source_sha256", rightSource)
            .put("observation_role", role)
            .put("quarter_turn_index", quarterTurnIndex)
            .put(
                "status",
                if (withinBounds) {
                    "ROTATION_CONSTRAINED_SIMILARITY_CANDIDATE_AVAILABLE"
                } else {
                    "ROTATION_CONSTRAINED_DIAGNOSTIC_OUTSIDE_BOUNDS"
                },
            )
            .put(
                "nominal_rotation_degrees",
                nominalRotationDeg,
            )
            .put(
                "residual_rotation_degrees",
                fit.residualRotationDeg,
            )
            .put(
                "fitted_total_rotation_degrees",
                fittedTotalRotation,
            )
            .put("uniform_scale", fit.scale)
            .put("translation_x_isotropic", fit.tx)
            .put("translation_y_isotropic", fit.ty)
            .put(
                "center_displacement_isotropic",
                centerDisplacement,
            )
            .put(
                "left_feature_support_source",
                leftFeatureSupportSource,
            )
            .put(
                "right_feature_support_source",
                rightFeatureSupportSource,
            )
            .put("left_feature_support_count", left.size)
            .put("right_feature_support_count", right.size)
            .put(
                "descriptor_candidate_count",
                candidates.size,
            )
            .put(
                "translation_cluster_match_count",
                best.size,
            )
            .put("robust_match_count", inliers.size)
            .put(
                "robust_residual_threshold_isotropic",
                robustThreshold,
            )
            .put("rms_residual_isotropic", rms)
            .put(
                "median_residual_isotropic",
                median(residuals),
            )
            .put(
                "p95_residual_isotropic",
                percentile(residuals, 0.95),
            )
            .put(
                "median_descriptor_hamming",
                median(descriptorDistances),
            )
            .put(
                "median_abs_feature_orientation_residual_degrees",
                median(orientationResiduals),
            )
            .put(
                "p95_abs_feature_orientation_residual_degrees",
                percentile(
                    orientationResiduals,
                    0.95,
                ),
            )
            .put(
                "diagnostic_bounds_pass",
                withinBounds,
            )
            .put(
                "forbidden_model_terms",
                JSONArray()
                    .put("SHEAR")
                    .put("ANISOTROPIC_SCALE")
                    .put("REFLECTION")
                    .put("PROJECTIVE"),
            )
            .put(
                "unconstrained_affine_used_for_fit",
                false,
            )
            .put(
                "nominal_relation_used_as_constraint",
                true,
            )
            .put(
                "same_world_structure_proven",
                false,
            )
            .put("world_registration_promoted", false)
            .put(
                "field_response_calibration_promoted",
                false,
            )
            .put("correction_authorized", false)
            .put("candidate_applied", false)
            .put("image_transform_applied", false)
            .put("creates_sensor_evidence", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun sourceRoles(
        payload: JSONObject,
    ): Map<String, Pair<String, Int>> {
        val samples =
            payload.optJSONArray("samples")
                ?: return emptyMap()
        val out =
            linkedMapOf<String, Pair<String, Int>>()
        for (i in 0 until samples.length()) {
            val sample =
                samples.optJSONObject(i)
                    ?: continue
            val source =
                sample.optString("source_sha256")
                    .trim()
                    .lowercase()
            val role =
                sample.optString("observation_role")
            val q = quarterTurn(role) ?: continue
            if (
                source.matches(
                    Regex("[0-9a-f]{64}"),
                )
            ) {
                out.putIfAbsent(
                    source,
                    role to q,
                )
            }
        }
        return out
    }

    private fun readFeatures(
        frontside: JSONObject,
    ): List<Feature> {
        val geometry =
            frontside.optJSONObject(
                "deterministic_local_feature_geometry_v0_1",
            ) ?: return emptyList()
        if (
            geometry.optString("status") !=
            "DETERMINISTIC_LOCAL_FEATURES_AVAILABLE"
        ) {
            return emptyList()
        }

        val width =
            geometry.optInt("analysis_width", 0)
        val height =
            geometry.optInt("analysis_height", 0)
        if (width < 2 || height < 2) {
            return emptyList()
        }
        val scale =
            max(width - 1, height - 1)
                .toDouble()
        val cx = (width - 1) * 0.5
        val cy = (height - 1) * 0.5
        val rotationSupport =
            geometry.optJSONArray(
                "rotation_support_keypoints",
            )
        val array =
            if (
                rotationSupport != null &&
                rotationSupport.length() >= MIN_MATCHES
            ) {
                rotationSupport
            } else {
                geometry.optJSONArray("keypoints")
                    ?: return emptyList()
            }
        val out = ArrayList<Feature>()

        for (i in 0 until array.length()) {
            val item =
                array.optJSONObject(i)
                    ?: continue
            val descriptor =
                hexToBytes(
                    item.optString(
                        "descriptor_hex_128bit",
                    ),
                ) ?: continue
            val xPx =
                item.optDouble(
                    "x_analysis_px",
                    Double.NaN,
                )
            val yPx =
                item.optDouble(
                    "y_analysis_px",
                    Double.NaN,
                )
            val orientation =
                item.optDouble(
                    "orientation_degrees",
                    Double.NaN,
                )
            if (
                !xPx.isFinite() ||
                !yPx.isFinite() ||
                !orientation.isFinite()
            ) {
                continue
            }
            out +=
                Feature(
                    index =
                        item.optInt("index", i),
                    xIso = (xPx - cx) / scale,
                    yIso = (yPx - cy) / scale,
                    orientationDeg = orientation,
                    descriptor = descriptor,
                )
        }
        return out
    }

    private fun featureSupportSource(
        frontside: JSONObject,
    ): String {
        val geometry =
            frontside.optJSONObject(
                "deterministic_local_feature_geometry_v0_1",
            ) ?: return "UNAVAILABLE"
        val rotationSupport =
            geometry.optJSONArray(
                "rotation_support_keypoints",
            )
        return if (
            rotationSupport != null &&
            rotationSupport.length() >= MIN_MATCHES
        ) {
            "ROTATION_SUPPORT_KEYPOINTS"
        } else {
            "PRIMARY_KEYPOINTS"
        }
    }

    private fun fitResidualSimilarity(
        matches: List<Match>,
    ): Similarity? {
        if (matches.size < 2) return null

        val meanX =
            matches.sumOf { it.nominalX } /
                matches.size.toDouble()
        val meanY =
            matches.sumOf { it.nominalY } /
                matches.size.toDouble()
        val meanU =
            matches.sumOf { it.right.xIso } /
                matches.size.toDouble()
        val meanV =
            matches.sumOf { it.right.yIso } /
                matches.size.toDouble()

        var real = 0.0
        var imag = 0.0
        var norm = 0.0
        for (m in matches) {
            val x = m.nominalX - meanX
            val y = m.nominalY - meanY
            val u = m.right.xIso - meanU
            val v = m.right.yIso - meanV
            real += x * u + y * v
            imag += x * v - y * u
            norm += x * x + y * y
        }
        if (
            !norm.isFinite() ||
            norm < 1.0e-12
        ) {
            return null
        }

        val a = real / norm
        val b = imag / norm
        val tx =
            meanU -
                (a * meanX - b * meanY)
        val ty =
            meanV -
                (b * meanX + a * meanY)

        if (
            !a.isFinite() ||
            !b.isFinite() ||
            !tx.isFinite() ||
            !ty.isFinite()
        ) {
            return null
        }
        return Similarity(a, b, tx, ty)
    }

    private fun residual(
        fit: Similarity,
        match: Match,
    ): Double {
        val px =
            fit.a * match.nominalX -
                fit.b * match.nominalY +
                fit.tx
        val py =
            fit.b * match.nominalX +
                fit.a * match.nominalY +
                fit.ty
        val dx = px - match.right.xIso
        val dy = py - match.right.yIso
        return sqrt(dx * dx + dy * dy)
    }

    private fun quarterTurn(
        role: String,
    ): Int? =
        when {
            role.contains("ROTATION_0_DEG") -> 0
            role.contains("ROTATION_90_DEG") -> 1
            role.contains("ROTATION_180_DEG") -> 2
            role.contains("ROTATION_270_DEG") -> 3
            else -> null
        }

    private fun hamming(
        a: ByteArray,
        b: ByteArray,
    ): Int {
        val n = min(a.size, b.size)
        var distance = 0
        for (i in 0 until n) {
            distance +=
                Integer.bitCount(
                    (a[i].toInt() xor
                        b[i].toInt()) and 0xff,
                )
        }
        distance +=
            abs(a.size - b.size) * 8
        return distance
    }

    private fun hexToBytes(
        hex: String,
    ): ByteArray? {
        if (
            hex.length != 32 ||
            hex.any {
                !it.isDigit() &&
                    it.lowercaseChar() !in 'a'..'f'
            }
        ) {
            return null
        }
        return ByteArray(16) { index ->
            val start = index * 2
            hex.substring(start, start + 2)
                .toInt(16)
                .toByte()
        }
    }

    private fun wrap180(
        value: Double,
    ): Double {
        if (!value.isFinite()) {
            return Double.NaN
        }
        var x = value % 360.0
        if (x > 180.0) x -= 360.0
        if (x <= -180.0) x += 360.0
        return x
    }

    private fun median(
        values: List<Double>,
    ): Double {
        val finite =
            values.filter {
                it.isFinite()
            }.sorted()
        if (finite.isEmpty()) {
            return Double.NaN
        }
        val n = finite.size
        return if (n % 2 == 1) {
            finite[n / 2]
        } else {
            0.5 * (
                finite[n / 2 - 1] +
                    finite[n / 2]
                )
        }
    }

    private fun percentile(
        values: List<Double>,
        q: Double,
    ): Double {
        val finite =
            values.filter {
                it.isFinite()
            }.sorted()
        if (finite.isEmpty()) {
            return Double.NaN
        }
        val index =
            ((finite.size - 1) *
                q.coerceIn(0.0, 1.0))
                .toInt()
        return finite[index]
    }

    private fun observationUnavailable(
        source: String,
        role: String,
        quarterTurnIndex: Int,
        reason: String,
        nominalRotationDeg: Double =
            quarterTurnIndex * 90.0,
    ): JSONObject =
        JSONObject()
            .put("source_sha256", source)
            .put("observation_role", role)
            .put(
                "quarter_turn_index",
                quarterTurnIndex,
            )
            .put(
                "nominal_rotation_degrees",
                nominalRotationDeg,
            )
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put(
                "nominal_relation_used_as_constraint",
                true,
            )
            .put(
                "unconstrained_affine_used_for_fit",
                false,
            )
            .put(
                "same_world_structure_proven",
                false,
            )
            .put("world_registration_promoted", false)
            .put(
                "field_response_calibration_promoted",
                false,
            )
            .put("correction_authorized", false)
            .put("candidate_applied", false)
            .put("image_transform_applied", false)
            .put("creates_sensor_evidence", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)

    private fun unavailable(
        reason: String,
    ): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put(
                "nominal_relation_used_as_geometry_constraint",
                false,
            )
            .put(
                "registration_adjusted_field_solver_executed",
                false,
            )
            .put("world_registration_promoted", false)
            .put(
                "field_response_calibration_promoted",
                false,
            )
            .put("correction_authorized", false)
            .put("image_transform_applied", false)
            .put("creates_sensor_evidence", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}
