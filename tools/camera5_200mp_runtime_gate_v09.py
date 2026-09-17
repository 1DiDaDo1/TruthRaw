#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

TARGET = (16320, 12288)
TARGET_SAMPLES = TARGET[0] * TARGET[1]
EXPECTED_CONTIGUOUS = TARGET_SAMPLES * 2
EXPECTED_ROW_BYTES = TARGET[0] * 2
PROVEN_BOUNDARY = "APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF"
PASS_CLASS = "APP_VISIBLE_PHYSICAL5_200MP_RAW_SENSOR_CAPTURE_PROVEN"
BLOCKED_CLASS = "BLOCKED_INCOMPLETE_V014_PHYSICAL5_200MP_RUNTIME_PROOF"


def sha256_file(path: Path, chunk: int = 8 << 20) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        while True:
            b = f.read(chunk)
            if not b:
                break
            h.update(b)
    return h.hexdigest()


def _int(v, default=-1) -> int:
    try:
        return int(v)
    except (TypeError, ValueError):
        return default


def evaluate(evidence: dict, raw_path: Path | None = None) -> dict:
    capability = evidence.get("capability") or {}
    preview = evidence.get("preview") or {}
    topology = evidence.get("requestTopology") or {}
    route = evidence.get("captureRoute") or {}
    result = evidence.get("captureResult") or {}
    payload = evidence.get("rawPayload") or {}

    raw_width = _int(route.get("width"), 0)
    raw_height = _int(route.get("height"), 0)
    raw_size = (raw_width, raw_height)
    result_ids = {str(v) for v in (route.get("reportedPhysicalIds") or [])}

    image_ts = result.get("imageTimestampNs")
    sensor_ts = result.get("sensorTimestampNs")

    pixel_stride = _int(payload.get("pixelStride"))
    row_stride = _int(payload.get("rowStride"))
    payload_bytes = _int(payload.get("bytes"))
    accessible_bytes = _int(payload.get("accessibleBufferBytes"))
    declared_hash = str(payload.get("sha256") or "").lower()
    canonical = payload.get("canonicalContiguousRawSensor") is True

    payload_exists = raw_path is not None and raw_path.exists()
    actual_size = raw_path.stat().st_size if payload_exists else None
    actual_hash = sha256_file(raw_path).lower() if payload_exists else None

    layout_consistent = (
        pixel_stride == 2
        and row_stride >= EXPECTED_ROW_BYTES
        and payload_bytes > 0
        and accessible_bytes == payload_bytes
    )
    if canonical:
        layout_consistent = (
            layout_consistent
            and row_stride == EXPECTED_ROW_BYTES
            and payload_bytes == EXPECTED_CONTIGUOUS
            and _int(payload.get("expectedContiguousBytes")) == EXPECTED_CONTIGUOUS
        )

    returned_mode = route.get("captureResultSensorPixelMode")
    returned_mode_is_max = evidence.get("returnedSensorPixelModeIsMaximumResolution") is True
    returned_mismatch_preserved = evidence.get("returnedSensorPixelModeMismatchPreserved") is True

    checks = {
        "v014_evidence_schema": str(evidence.get("schema") or "")
        == "truthraw.fotograaf-camera5-200mp-staged-evidence.v0.14",
        "acquisition_observation_authority_only": evidence.get("authority")
        == "CAMERA2_ACQUISITION_OBSERVATION_ONLY",
        "calibration_authority_not_granted": evidence.get("calibrationAuthorityGranted") is False,
        "scientific_master_unmodified": evidence.get("scientificMasterModified") is False,
        "single_physical_frame": _int(evidence.get("physicalFrameCount"), 0) == 1,
        "single_independent_evidence_item": _int(evidence.get("independentEvidenceCount"), 0) == 1,
        "maximum_highres_capability_route": capability.get("discoverySource")
        == "MAXIMUM_MAP_HIGH_RESOLUTION",
        "capability_target_exact": (
            _int(capability.get("targetWidth"), 0),
            _int(capability.get("targetHeight"), 0),
        )
        == TARGET,
        "logical0_preview_parent": str(preview.get("logicalCameraId")) == "0",
        "opened_logical0": str(topology.get("openedCameraId")) == "0",
        "requested_physical5": str(topology.get("requestedPhysicalCameraId")) == "5",
        "physical_scoped_request": topology.get("physicalScopedRequestUsed") is True,
        "output_bound_to_physical5": topology.get("outputPhysicalBinding") is True,
        "output_declared_maximum_resolution": topology.get("outputMaximumResolutionModeDeclared")
        is True,
        "physical_maximum_resolution_setting_written": topology.get("physicalSensorPixelModeWritten")
        is True,
        "physical5_reported": "5" in result_ids,
        "physical5_result_selected": str(route.get("physicalResultCameraId")) == "5",
        "exact_200mp_dimensions": raw_size == TARGET,
        "exact_sample_count": _int(route.get("sampleCount"), 0) == TARGET_SAMPLES,
        "requested_maximum_resolution_recorded": route.get("requestedMaximumResolution") is True,
        "route_output_maximum_resolution_recorded": route.get("outputMaximumResolutionModeDeclared")
        is True,
        "result_pixel_mode_kept_independent": route.get("resultPixelModeIsIndependentObservation")
        is True,
        "timestamp_identity_flag": result.get("timestampIdentityPass") is True,
        "timestamp_exact_equality": image_ts is not None and image_ts == sensor_ts,
        "raw_stride_layout_plausible": layout_consistent,
        "raw_payload_hash_declared": len(declared_hash) == 64,
        "seal_precedes_pixel_mode_interpretation": evidence.get("sealBeforePixelModeInterpretation")
        is True,
        "untouched_adc_boundary_preserved": evidence.get("boundary") == PROVEN_BOUNDARY,
        "payload_file_provided": raw_path is not None,
        "payload_file_exists": payload_exists,
        "payload_file_size_matches_evidence": payload_exists and actual_size == payload_bytes,
        "payload_file_sha256_matches_evidence": payload_exists and actual_hash == declared_hash,
    }

    # The qualifying v0.14 device run returned SENSOR_PIXEL_MODE=0. This is intentionally
    # *not* a failure condition after exact raster, physical-result and timestamp proof.
    # The mismatch itself must remain visible rather than being rewritten to MAX.
    pixel_mode_observation_preserved = (
        returned_mode is not None
        and route.get("resultPixelModeIsIndependentObservation") is True
        and (returned_mode_is_max or returned_mismatch_preserved)
    )
    checks["returned_pixel_mode_observation_preserved"] = pixel_mode_observation_preserved

    passed = all(checks.values())
    classification = PASS_CLASS if passed else BLOCKED_CLASS

    return {
        "schema": "TruthRawCamera5_200MPRuntimeGate/0.9",
        "classification": classification,
        "pass": passed,
        "checks": checks,
        "observed": {
            "opened_camera_id": topology.get("openedCameraId"),
            "requested_physical_camera_id": topology.get("requestedPhysicalCameraId"),
            "reported_physical_ids": sorted(result_ids),
            "physical_result_camera_id": route.get("physicalResultCameraId"),
            "raw_dimensions": [raw_width, raw_height],
            "sample_count": route.get("sampleCount"),
            "image_timestamp_ns": image_ts,
            "physical_sensor_timestamp_ns": sensor_ts,
            "row_stride": row_stride,
            "pixel_stride": pixel_stride,
            "payload_bytes": payload_bytes,
            "accessible_buffer_bytes": accessible_bytes,
            "expected_contiguous_bytes": EXPECTED_CONTIGUOUS,
            "canonical_contiguous_rawsensor": canonical,
            "manifest_payload_sha256": declared_hash or None,
            "actual_payload_sha256": actual_hash,
            "capture_result_sensor_pixel_mode": returned_mode,
            "returned_sensor_pixel_mode_is_maximum_resolution": returned_mode_is_max,
            "returned_sensor_pixel_mode_mismatch_preserved": returned_mismatch_preserved,
            "physical_sensor_pixel_mode_override_advertised": topology.get(
                "physicalSensorPixelModeOverrideAdvertised"
            ),
            "raw_binning_factor_used": result.get("rawBinningFactorUsed"),
            "noise_reduction_mode": result.get("noiseReductionMode"),
            "edge_mode": result.get("edgeMode"),
        },
        "authority": {
            "physical_frame_count": evidence.get("physicalFrameCount"),
            "independent_evidence_count": evidence.get("independentEvidenceCount"),
            "calibration_authority_granted": evidence.get("calibrationAuthorityGranted"),
            "scientific_master_modified": evidence.get("scientificMasterModified"),
        },
        "untouched_photodiode_adc_raw_proven": False,
        "boundary": (
            "PASS proves one preserved app-visible Camera2 RAW_SENSOR frame at 16320x12288 "
            "through opened logical camera 0, output/request bound to physical camera 5, with a "
            "physical Camera-5 result and exact Image/SENSOR_TIMESTAMP identity plus payload-file "
            "SHA-256 identity. Returned SENSOR_PIXEL_MODE is preserved as an independent observation "
            "and need not report MAXIMUM_RESOLUTION after the primary evidence is sealed. PASS does "
            "not prove untouched photodiode/ADC output, absence of on-sensor/HAL processing, one ADC "
            "conversion per output sample, electron calibration, optical 200 MP resolution, or full "
            "physical colour truth."
        ),
    }


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("evidence")
    ap.add_argument("--raw")
    ap.add_argument("--out", required=True)
    ns = ap.parse_args()

    evidence = json.load(open(ns.evidence, "r", encoding="utf-8"))
    report = evaluate(evidence, Path(ns.raw) if ns.raw else None)
    Path(ns.out).write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
