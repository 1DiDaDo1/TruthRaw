#!/usr/bin/env python3
"""TruthRaw Camera-5 200MP proof-chain topology binder v0.3.

v0.2 proves an internally consistent app-visible 16320x12288 RAW_SENSOR ->
canonical CFA -> DNG CFA identity chain. v0.3 adds a fail-closed topology binding
between Camera2's reported CFA arrangement and the DNG CFA pattern, plus exact
sample-count and Camera-5 result-route checks.

This still does not prove one untouched ADC code per physical photodiode or the
absence of sensor/HAL remosaic/processing. It proves application-visible raster
identity and metadata topology consistency only.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

from camera5_200mp_evidence_bundle_v02 import BUNDLE_PASS, evaluate_bundle

TARGET = (16320, 12288)
TARGET_SAMPLES = TARGET[0] * TARGET[1]
V03_PASS = "CAMERA5_200MP_CAPTURE_CHAIN_CFA_TOPOLOGY_BOUND_APP_VISIBLE"

# Android CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT values for
# Bayer layouts. RGB/MONO/NIR layouts are intentionally not accepted by this
# Bayer-CFA proof path.
CAMERA2_CFA = {
    0: "RGGB",
    1: "GRBG",
    2: "GBRG",
    3: "BGGR",
}


def evaluate_bundle_v03(manifest: dict, runtime_gate: dict, canonical_report: dict, identity_report: dict) -> dict:
    base = evaluate_bundle(manifest, runtime_gate, canonical_report, identity_report)
    cc = manifest.get("camera_characteristics") or {}
    result = manifest.get("capture_result") or {}
    dng_ifd = ((identity_report.get("observed") or {}).get("dng_raw_ifd") or {})

    cfa_code = cc.get("cfa_arrangement")
    try:
        cfa_code_int = int(cfa_code)
    except (TypeError, ValueError):
        cfa_code_int = None
    camera2_cfa_name = CAMERA2_CFA.get(cfa_code_int)
    dng_cfa_name = dng_ifd.get("cfa_pattern_name")

    samples = canonical_report.get("samples")
    try:
        samples_int = int(samples)
    except (TypeError, ValueError):
        samples_int = -1

    raw_binning = result.get("sensor_raw_binning_factor_used")
    checks = {
        "v02_capture_chain_pass": base.get("pass") is True and base.get("classification") == BUNDLE_PASS,
        "camera2_bayer_cfa_reported": camera2_cfa_name is not None,
        "dng_bayer_cfa_reported": dng_cfa_name in set(CAMERA2_CFA.values()),
        "camera2_dng_cfa_match": camera2_cfa_name is not None and camera2_cfa_name == dng_cfa_name,
        "canonical_exact_200mp_sample_count": samples_int == TARGET_SAMPLES,
        "capture_result_camera5": str(result.get("result_camera_id")) == "5",
        "maximum_resolution_result": result.get("sensor_pixel_mode") == "MAXIMUM_RESOLUTION",
        "no_explicit_raw_binning_true": raw_binning is not True,
    }

    passed = all(checks.values())
    classification = V03_PASS if passed else "BLOCKED_CAMERA5_200MP_CFA_TOPOLOGY_NOT_BOUND"
    return {
        "schema": "TruthRawCamera5_200MPEvidenceBundle/0.3",
        "classification": classification,
        "pass": passed,
        "physicalFrameCount": 1,
        "independentEvidenceCount": 1,
        "checks": checks,
        "bindings": {
            "camera2_cfa_arrangement_code": cfa_code_int,
            "camera2_cfa_pattern": camera2_cfa_name,
            "dng_cfa_pattern": dng_cfa_name,
            "canonical_samples": samples_int,
            "expected_samples": TARGET_SAMPLES,
            "sensor_raw_binning_factor_used": raw_binning,
            "source_identity_sha256": manifest.get("source_identity_sha256"),
            "canonical_cfa_sha256": canonical_report.get("canonical_sha256"),
            "dng_file_sha256": (manifest.get("dng_output") or {}).get("sha256"),
        },
        "v02": base,
        "scientific_boundary": (
            "PASS proves the v0.2 one-frame Camera-5 16320x12288 app-visible RAW_SENSOR/DNG CFA identity chain "
            "plus consistency between Camera2's reported Bayer CFA arrangement and the DNG CFA pattern, exact "
            "200,540,160 canonical samples, Camera-5 capture-result identity, and no explicit TRUE raw-binning "
            "result. It does not prove untouched photodiode/ADC output, absence of remosaic or other upstream "
            "sensor/HAL processing, physical pixel pitch, electron calibration, optics, spectral response or colour truth."
        ),
    }


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("manifest")
    ap.add_argument("runtime_gate")
    ap.add_argument("canonical_report")
    ap.add_argument("identity_report")
    ap.add_argument("--out", required=True)
    ns = ap.parse_args()

    def load(path: str) -> dict:
        return json.loads(Path(path).read_text(encoding="utf-8"))

    result = evaluate_bundle_v03(
        load(ns.manifest),
        load(ns.runtime_gate),
        load(ns.canonical_report),
        load(ns.identity_report),
    )
    Path(ns.out).write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps(result, indent=2))
    if not result["pass"]:
        raise SystemExit(2)


if __name__ == "__main__":
    main()
