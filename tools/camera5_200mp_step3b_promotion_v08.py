#!/usr/bin/env python3
"""TruthRaw Camera-5 200 MP Step-3B promotion gate v0.8.

v0.8 is the final fail-closed promotion contract for a *real* 16320x12288
Camera-5 app-visible RAW_SENSOR capture after the lower-level runtime gate and
CFA/topology bundle have already passed.

It does not inspect/transform image samples itself. It verifies that the proof
chain carries the minimum acquisition/metrology facts required by the current
TruthRaw architecture before the bounded claim
APP_VISIBLE_MAXIMUM_RESOLUTION_RAW_SENSOR_CFA_PROVEN may be emitted.

It never emits UNTOUCHED_NATIVE_200MP_ADC.
"""
from __future__ import annotations

import argparse
import json
from math import isfinite
from pathlib import Path
from typing import Any

TARGET = (16320, 12288)
TARGET_SAMPLES = TARGET[0] * TARGET[1]
RUNTIME_CLASS = "FULL_SENSOR_200MP_APP_VISIBLE_RAW_CAPTURE_PROVEN"
TOPOLOGY_CLASS = "CAMERA5_200MP_CAPTURE_CHAIN_CFA_TOPOLOGY_BOUND_APP_VISIBLE"
PASS = "APP_VISIBLE_MAXIMUM_RESOLUTION_RAW_SENSOR_CFA_PROVEN"
BLOCKED = "BLOCKED_CAMERA5_200MP_STEP3B_PROMOTION_INCOMPLETE"
OPTIONAL_AVAILABILITY = {"PRESENT", "UNAVAILABLE_REPORTED"}


def _positive_number(v: Any) -> bool:
    try:
        return isfinite(float(v)) and float(v) > 0.0
    except (TypeError, ValueError):
        return False


def _nonnegative_number(v: Any) -> bool:
    try:
        return isfinite(float(v)) and float(v) >= 0.0
    except (TypeError, ValueError):
        return False


def _sha(v: Any) -> bool:
    if not isinstance(v, str) or len(v) != 64:
        return False
    return all(c in "0123456789abcdefABCDEF" for c in v)


def evaluate_step3b(manifest: dict, runtime_gate: dict, topology_bundle: dict) -> dict:
    out = manifest.get("raw_output") or {}
    req = manifest.get("requested") or {}
    res = manifest.get("capture_result") or {}
    cc = manifest.get("camera_characteristics") or {}
    availability = manifest.get("metadata_availability") or {}
    bundle_bindings = topology_bundle.get("bindings") or {}

    dims = (int(out.get("width", 0)), int(out.get("height", 0)))
    source_sha = manifest.get("source_identity_sha256")
    black = cc.get("black_level_pattern")
    cfa = cc.get("cfa_arrangement")
    noise_availability = availability.get("noise_profile")
    shading_availability = availability.get("lens_shading_map")

    checks = {
        "real_device_runtime_capture": manifest.get("evidence_class") == "DEVICE_RUNTIME_CAPTURE",
        "physical_camera_5": str(manifest.get("physical_camera_id")) == "5" and str(manifest.get("opened_camera_id")) == "5",
        "raw_sensor_16320x12288": out.get("format") == "RAW_SENSOR" and dims == TARGET,
        "maximum_resolution_requested": req.get("sensor_pixel_mode") == "MAXIMUM_RESOLUTION",
        "maximum_resolution_result": res.get("sensor_pixel_mode") == "MAXIMUM_RESOLUTION",
        "physical_result_camera_5": str(res.get("result_camera_id")) == "5",
        "timestamp_bound": out.get("timestamp_matches_capture_result") is True and out.get("image_timestamp_ns") == res.get("sensor_timestamp_ns"),
        "payload_sha_bound": _sha(source_sha) and source_sha == out.get("payload_sha256"),
        "stride_facts_present": int(out.get("pixel_stride", -1)) == 2 and int(out.get("row_stride", 0)) >= TARGET[0] * 2 and int(out.get("payload_bytes", 0)) > 0,
        "runtime_gate_pass": runtime_gate.get("pass") is True and runtime_gate.get("classification") == RUNTIME_CLASS,
        "topology_bundle_pass": topology_bundle.get("pass") is True and topology_bundle.get("classification") == TOPOLOGY_CLASS,
        "topology_bundle_source_bound": bundle_bindings.get("source_identity_sha256") == source_sha,
        "topology_exact_sample_count": int(bundle_bindings.get("canonical_samples", -1)) == TARGET_SAMPLES,
        "cfa_arrangement_present": str(cfa) in {"0", "1", "2", "3"},
        "black_level_pattern_present": isinstance(black, list) and len(black) == 4 and all(_nonnegative_number(v) for v in black),
        "white_level_present": _positive_number(cc.get("white_level")),
        "exposure_time_present": _positive_number(res.get("sensor_exposure_time_ns")),
        "iso_present": _positive_number(res.get("sensor_sensitivity_iso")),
        "focus_distance_present": _nonnegative_number(res.get("lens_focus_distance_diopters")),
        "stabilization_state_recorded": res.get("lens_optical_stabilization_mode") is not None,
        "noise_profile_availability_recorded": noise_availability in OPTIONAL_AVAILABILITY,
        "lens_shading_availability_recorded": shading_availability in OPTIONAL_AVAILABILITY,
        "noise_profile_present_when_declared": noise_availability != "PRESENT" or isinstance(res.get("noise_profile"), list),
        "lens_shading_present_when_declared": shading_availability != "PRESENT" or res.get("lens_shading_map") is not None,
        "no_explicit_raw_binning_true": res.get("sensor_raw_binning_factor_used") is not True,
    }

    passed = all(checks.values())
    missing = [k for k, ok in checks.items() if not ok]
    return {
        "schema": "TruthRawCamera5Step3BPromotion/0.8",
        "classification": PASS if passed else BLOCKED,
        "pass": passed,
        "physicalFrameCount": 1,
        "independentEvidenceCount": 1,
        "checks": checks,
        "missing_or_failed_requirements": missing,
        "bindings": {
            "source_identity_sha256": source_sha,
            "canonical_cfa_sha256": bundle_bindings.get("canonical_cfa_sha256"),
            "dng_file_sha256": bundle_bindings.get("dng_file_sha256"),
            "dimensions": list(dims),
            "samples": TARGET_SAMPLES,
            "physical_camera_id": str(manifest.get("physical_camera_id")),
            "sensor_pixel_mode_requested": req.get("sensor_pixel_mode"),
            "sensor_pixel_mode_result": res.get("sensor_pixel_mode"),
            "image_timestamp_ns": out.get("image_timestamp_ns"),
            "sensor_timestamp_ns": res.get("sensor_timestamp_ns"),
            "row_stride": out.get("row_stride"),
            "pixel_stride": out.get("pixel_stride"),
            "payload_bytes": out.get("payload_bytes"),
            "cfa_arrangement": cfa,
            "black_level_pattern": black,
            "white_level": cc.get("white_level"),
            "sensor_exposure_time_ns": res.get("sensor_exposure_time_ns"),
            "sensor_sensitivity_iso": res.get("sensor_sensitivity_iso"),
            "lens_focus_distance_diopters": res.get("lens_focus_distance_diopters"),
            "lens_optical_stabilization_mode": res.get("lens_optical_stabilization_mode"),
            "noise_profile_availability": noise_availability,
            "lens_shading_map_availability": shading_availability,
        },
        "primary_evidence_role": "ORIGINAL_APP_VISIBLE_RAW_SENSOR_PAYLOAD",
        "dng_role": "AUXILIARY_DERIVED_CONTAINER_WITH_SEPARATE_CFA_IDENTITY_CHECK",
        "allowed_claim": PASS if passed else None,
        "forbidden_claim": "UNTOUCHED_NATIVE_200MP_ADC",
        "downstream_gates_still_required": [
            "READOUT_DOMAIN_PRECISION_UNCERTAINTY",
            "NOISE_PTC",
            "SHADING",
            "COLOUR_ILLUMINANT",
            "SFR_MTF_OPTICS",
            "HELD_OUT_CALIBRATION",
        ],
        "scientific_boundary": (
            "PASS proves a complete bounded Step-3B application-visible Camera-5 maximum-resolution RAW_SENSOR CFA proof chain, "
            "including physical result/timestamp binding, payload/stride facts, minimum capture metadata and the exact CFA/topology identity bundle. "
            "It remains one physical frame and does not prove untouched ADC/photodiode output, absence of sensor/HAL processing, electron calibration, "
            "noise transferability, physical colour truth or 200 MP optical resolution."
        ),
    }


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("manifest")
    ap.add_argument("runtime_gate")
    ap.add_argument("topology_bundle")
    ap.add_argument("--out", required=True)
    ns = ap.parse_args()

    def load(p: str) -> dict:
        return json.loads(Path(p).read_text(encoding="utf-8"))

    result = evaluate_step3b(load(ns.manifest), load(ns.runtime_gate), load(ns.topology_bundle))
    Path(ns.out).write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))
    if not result["pass"]:
        raise SystemExit(2)


if __name__ == "__main__":
    main()
