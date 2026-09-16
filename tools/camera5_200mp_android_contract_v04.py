#!/usr/bin/env python3
"""TruthRaw Camera-5 200MP Android-contract guard v0.4.

This module does NOT prove a physical 200 MP capture. It formalizes the bounded
interpretation of the current Camera-5 static capability evidence against the
Android Camera2 maximum-resolution / ultra-high-resolution contract.

Key rule: static capability and host validation may prove that a route is
advertised and prepared, but may not promote it to a real capture. The real
device gate remains open until a qualifying 16320x12288 RAW_SENSOR payload is
bound to physical Camera-5 TotalCaptureResult metadata.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

TARGET = (16320, 12288)
TARGET_SAMPLES = TARGET[0] * TARGET[1]
PASS = "CAMERA5_200MP_ANDROID_CONTRACT_STATIC_READY_RUNTIME_OPEN"
BLOCKED = "BLOCKED_CAMERA5_200MP_ANDROID_CONTRACT"
EXPECTED_CAPTURE_GATE = "OPEN_NEEDS_REAL_16320x12288_RAW_PAYLOAD_AND_TOTALCAPTURERESULT_BINDING"


def _has_raw_route(routes: object, width: int, height: int) -> bool:
    if not isinstance(routes, list):
        return False
    return any(
        isinstance(r, dict)
        and r.get("format") == "RAW_SENSOR"
        and int(r.get("width", -1)) == width
        and int(r.get("height", -1)) == height
        for r in routes
    )


def evaluate_contract(capability: dict, host_validation: dict | None = None) -> dict:
    geometry = capability.get("geometry") or {}
    raw_routes = capability.get("raw_routes") or {}
    camera = capability.get("camera") or {}

    max_array = tuple(geometry.get("maximum_pixel_array") or ())
    binning = capability.get("sensor_info_binning_factor")
    uhr = capability.get("ultra_high_resolution_sensor") is True
    remosaic = capability.get("remosaic_reprocessing") is True

    # Android's documented UHR contract says RAW_SENSOR is already regular
    # Bayer when REMOSAIC_REPROCESSING is not advertised. A simultaneously
    # reported binning-factor key is therefore retained as vendor-metadata
    # tension, not promoted to app-visible same-colour CFA topology.
    regular_bayer_expected = uhr and not remosaic
    vendor_metadata_tension = regular_bayer_expected and binning is not None

    checks = {
        "static_capability_gate_pass": capability.get("pass") is True,
        "physical_camera_5": str(camera.get("camera_id")) == "5",
        "ultra_high_resolution_sensor": uhr,
        "maximum_pixel_array_exact": max_array == TARGET,
        "advertised_raw_sensor_16320x12288": _has_raw_route(
            raw_routes.get("high_resolution"), *TARGET
        ),
        "remosaic_reprocessing_not_advertised": not remosaic,
        "android_contract_regular_bayer_expected": regular_bayer_expected,
        "bounded_topology_interpretation": capability.get(
            "app_visible_raw_topology_interpretation"
        ) == "APP_VISIBLE_RAW_SENSOR_REGULAR_BAYER_BY_ANDROID_CONTRACT",
        "runtime_capture_gate_still_open": capability.get("capture_gate")
        == EXPECTED_CAPTURE_GATE,
    }

    host_checks: dict[str, bool] = {}
    if host_validation is not None:
        h = host_validation.get("checks") or {}
        host_checks = {
            "host_validation_pass": host_validation.get("status") == "PASS",
            "host_exact_target": h.get("exact_16320x12288_target") is True,
            "host_raw_sensor_only_candidate": h.get("raw_sensor_only_candidate") is True,
            "host_explicit_maximum_pixel_mode": h.get("explicit_maximum_pixel_mode") is True,
            "host_device_execution_pending": host_validation.get("device_execution")
            == "PENDING_REAL_BKQ_N49_RUN",
        }

    passed = all(checks.values()) and all(host_checks.values())
    return {
        "schema": "TruthRawCamera5AndroidContractGuard/0.4",
        "classification": PASS if passed else BLOCKED,
        "pass": passed,
        "target": {
            "width": TARGET[0],
            "height": TARGET[1],
            "samples": TARGET_SAMPLES,
        },
        "checks": checks,
        "host_checks": host_checks,
        "observations": {
            "sensor_info_binning_factor": binning,
            "vendor_metadata_tension": vendor_metadata_tension,
            "android_contract_regular_bayer_expected": regular_bayer_expected,
        },
        "runtime_gate": EXPECTED_CAPTURE_GATE,
        "permitted_current_claim": "STATIC_CAPABILITY_AND_HOST_PREPARATION_ONLY",
        "permitted_future_capture_claim": "APP_VISIBLE_MAXIMUM_RESOLUTION_RAW_SENSOR_CFA_PROVEN",
        "forbidden_without_separate_evidence": "UNTOUCHED_NATIVE_200MP_ADC",
        "scientific_boundary": (
            "PASS validates only the bounded interpretation of static Camera2 capability and, when supplied, "
            "host-side probe preparation. It does not prove a delivered 16320x12288 frame, payload identity, "
            "sensor/HAL processing absence, optical resolution, noise/PTC, colour calibration or scientific "
            "transferability from 4080x3072/8160x6144."
        ),
    }


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("capability_gate")
    ap.add_argument("--host-validation")
    ap.add_argument("--out")
    ns = ap.parse_args()

    capability = json.loads(Path(ns.capability_gate).read_text(encoding="utf-8"))
    host = None
    if ns.host_validation:
        host = json.loads(Path(ns.host_validation).read_text(encoding="utf-8"))
    result = evaluate_contract(capability, host)
    text = json.dumps(result, indent=2)
    if ns.out:
        Path(ns.out).write_text(text + "\n", encoding="utf-8")
    print(text)
    if not result["pass"]:
        raise SystemExit(2)


if __name__ == "__main__":
    main()
