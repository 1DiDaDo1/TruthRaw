#!/usr/bin/env python3
"""TruthRaw Camera 5 native-200MP evidence bundle v0.2.

This tool composes the already separated Camera2 runtime gate, canonical RAW
normalization report and RAW<->DNG CFA identity result into one fail-closed
proof-chain classification.

It does not create a second exposure and does not upgrade app-visible RAW into
untouched photodiode/ADC truth. Its purpose is to prove that one Camera-5
16320x12288 application-visible RAW_SENSOR observation is consistently bound
through capture manifest -> runtime gate -> canonical CFA raster -> DNG CFA IFD.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Optional

from camera5_200mp_runtime_gate_v07 import evaluate as evaluate_runtime
from normalize_camera5_200mp_raw_v07 import normalize as normalize_raw
from camera5_200mp_cfa_identity_v01 import evaluate_identity

TARGET = (16320, 12288)
RUNTIME_PASS = "FULL_SENSOR_200MP_APP_VISIBLE_RAW_CAPTURE_PROVEN"
CANONICAL_CLASS = "FULL_SENSOR_200MP_CANONICAL_APP_VISIBLE_CFA_MIRROR"
IDENTITY_PASS = "CAMERA5_200MP_RAW_DNG_CFA_IDENTITY_PROVEN"
BUNDLE_PASS = "CAMERA5_200MP_CAPTURE_CHAIN_PROVEN_APP_VISIBLE_CFA"


def evaluate_bundle(manifest: dict, runtime_gate: dict, canonical_report: dict, identity_report: dict) -> dict:
    raw_out = manifest.get("raw_output") or {}
    dng_out = manifest.get("dng_output") or {}
    runtime_observed = runtime_gate.get("observed") or {}
    identity_observed = identity_report.get("observed") or {}
    identity_inner = identity_observed.get("identity") or {}

    dims = (int(raw_out.get("width", 0)), int(raw_out.get("height", 0)))
    source_hash = manifest.get("source_identity_sha256")
    payload_hash = raw_out.get("payload_sha256")
    canonical_hash = canonical_report.get("canonical_sha256")
    dng_hash = dng_out.get("sha256")

    checks = {
        "single_device_runtime_capture": manifest.get("evidence_class") == "DEVICE_RUNTIME_CAPTURE",
        "camera5_route": str(manifest.get("physical_camera_id")) == "5",
        "target_16320x12288": dims == TARGET,
        "raw_sensor_format": raw_out.get("format") == "RAW_SENSOR",
        "source_identity_bound": bool(source_hash) and source_hash == payload_hash,
        "runtime_gate_pass": runtime_gate.get("pass") is True and runtime_gate.get("classification") == RUNTIME_PASS,
        "runtime_gate_dimensions_match": tuple(runtime_observed.get("raw_dimensions") or ()) == dims,
        "runtime_gate_source_hash_match": runtime_observed.get("manifest_payload_sha256") == source_hash,
        "canonical_report_class": canonical_report.get("classification") == CANONICAL_CLASS,
        "canonical_dimensions_match": (
            int(canonical_report.get("width", -1)), int(canonical_report.get("height", -1))
        ) == dims,
        "canonical_source_bound": canonical_report.get("source_buffer_sha256") == source_hash,
        "canonical_hash_present": isinstance(canonical_hash, str) and len(canonical_hash) == 64,
        "dng_hash_present": isinstance(dng_hash, str) and len(dng_hash) == 64,
        "identity_gate_pass": identity_report.get("pass") is True and identity_report.get("classification") == IDENTITY_PASS,
        "identity_dimensions_match": tuple(identity_observed.get("manifest_dimensions") or ()) == dims,
        "identity_canonical_hash_match": identity_observed.get("canonical_raw_sha256") == canonical_hash,
        "identity_dng_hash_match": identity_observed.get("dng_sha256") == dng_hash,
        "identity_stream_hash_match": identity_inner.get("canonical_raw_sha256_stream") == canonical_hash,
        "identity_dng_cfa_hash_match": identity_inner.get("dng_cfa_canonical_sha256") == canonical_hash,
        "sample_identity": identity_inner.get("sample_identity") is True,
        "exact_sample_count": int(identity_inner.get("samples_compared", -1)) == TARGET[0] * TARGET[1],
        "no_first_mismatch": identity_inner.get("first_mismatch_sample") is None,
    }

    passed = all(checks.values())
    classification = BUNDLE_PASS if passed else "BLOCKED_CAMERA5_200MP_CAPTURE_CHAIN_NOT_PROVEN"
    return {
        "schema": "TruthRawCamera5_200MPEvidenceBundle/0.2",
        "classification": classification,
        "pass": passed,
        "physicalFrameCount": 1,
        "independentEvidenceCount": 1,
        "checks": checks,
        "bindings": {
            "source_identity_sha256": source_hash,
            "canonical_cfa_sha256": canonical_hash,
            "dng_file_sha256": dng_hash,
            "dimensions": list(dims),
            "physical_camera_id": str(manifest.get("physical_camera_id")),
            "sensor_pixel_mode_requested": (manifest.get("requested") or {}).get("sensor_pixel_mode"),
            "sensor_pixel_mode_result": (manifest.get("capture_result") or {}).get("sensor_pixel_mode"),
        },
        "scientific_boundary": (
            "PASS proves one internally consistent application-visible Camera2 Camera-5 16320x12288 RAW_SENSOR "
            "capture chain, including exact canonical-CFA identity with its DNG RAW IFD. It remains one physical "
            "frame/one independent evidence item and does not prove absence of on-sensor/HAL processing, one ADC "
            "conversion per physical photodiode, electron-domain calibration, optical truth, spectral truth or colour truth."
        ),
    }


def prove_from_files(manifest_path: Path, source_raw: Path, dng_path: Path, workdir: Path) -> dict:
    """Run the whole existing proof chain on one real capture, streaming where available."""
    workdir.mkdir(parents=True, exist_ok=True)
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))

    runtime_gate = evaluate_runtime(manifest, source_raw)
    runtime_path = workdir / "camera5_200mp_runtime_gate_v07.json"
    runtime_path.write_text(json.dumps(runtime_gate, indent=2), encoding="utf-8")

    canonical_raw = workdir / "camera5_200mp_canonical.rawsensor"
    canonical_report = normalize_raw(manifest, source_raw, canonical_raw)
    canonical_report_path = workdir / "camera5_200mp_canonical_report_v07.json"
    canonical_report_path.write_text(json.dumps(canonical_report, indent=2), encoding="utf-8")

    identity_report = evaluate_identity(manifest, canonical_report, canonical_raw, dng_path, require_200mp=True)
    identity_path = workdir / "camera5_200mp_cfa_identity_v01.json"
    identity_path.write_text(json.dumps(identity_report, indent=2), encoding="utf-8")

    bundle = evaluate_bundle(manifest, runtime_gate, canonical_report, identity_report)
    bundle["artefacts"] = {
        "runtime_gate": runtime_path.name,
        "canonical_raw": canonical_raw.name,
        "canonical_report": canonical_report_path.name,
        "cfa_identity": identity_path.name,
    }
    return bundle


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("manifest")
    ap.add_argument("source_raw")
    ap.add_argument("dng")
    ap.add_argument("--workdir", required=True)
    ap.add_argument("--out", required=True)
    ns = ap.parse_args()

    result = prove_from_files(Path(ns.manifest), Path(ns.source_raw), Path(ns.dng), Path(ns.workdir))
    Path(ns.out).write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps(result, indent=2))
    if not result["pass"]:
        raise SystemExit(2)


if __name__ == "__main__":
    main()
