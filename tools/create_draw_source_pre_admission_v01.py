#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re

HEX64 = re.compile(r"^[0-9a-f]{64}$")
LENS = {"MAIN", "ULTRA_WIDE", "TELEPHOTO", "FRONT", "EXTERNAL", "UNKNOWN"}

def canonical_hash(doc: dict) -> str:
    x = dict(doc)
    x.pop("state_sha256", None)
    return hashlib.sha256(
        json.dumps(
            x,
            sort_keys=True,
            separators=(",", ":"),
            ensure_ascii=False,
            allow_nan=False,
        ).encode("utf-8")
    ).hexdigest()

def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("--source-sha256", required=True)
    p.add_argument("--device-id", required=True)
    p.add_argument("--camera-id", required=True)
    p.add_argument("--lens-role", required=True, choices=sorted(LENS))
    p.add_argument("--capture-pipeline-id", required=True)
    p.add_argument("--sample-domain-id", required=True)
    p.add_argument("--readout-domain-id")
    p.add_argument("--sensor-pixel-mode")
    p.add_argument("--width", required=True, type=int)
    p.add_argument("--height", required=True, type=int)
    p.add_argument("--cfa-topology", required=True)
    p.add_argument("--output", required=True)
    a = p.parse_args()

    source = a.source_sha256.lower()
    if HEX64.fullmatch(source) is None:
        raise SystemExit("source SHA-256 must be 64 hexadecimal characters")
    if a.width <= 0 or a.height <= 0:
        raise SystemExit("width/height must be positive")
    for name, value in (
        ("device-id", a.device_id),
        ("camera-id", a.camera_id),
        ("capture-pipeline-id", a.capture_pipeline_id),
        ("sample-domain-id", a.sample_domain_id),
        ("cfa-topology", a.cfa_topology),
    ):
        if not value.strip():
            raise SystemExit(name + " may not be empty")

    manifest = {
        "schema": "D.RAW/SourcePreAdmissionManifest/0.1",
        "status": "CANDIDATE_NOT_SCIENTIFICALLY_ADMITTED",
        "source_evidence": {"sha256": source, "sealed": True},
        "instrument": {
            "device_id": a.device_id,
            "camera_id": a.camera_id,
            "lens_role": a.lens_role,
            "capture_pipeline_id": a.capture_pipeline_id,
        },
        "source_domain": {
            "source_route_id": a.capture_pipeline_id,
            "sample_domain_id": a.sample_domain_id,
            "readout_domain_id": a.readout_domain_id,
            "readout_domain_authority": "SOURCE_BOUND" if a.readout_domain_id else "UNKNOWN",
            "sensor_pixel_mode": a.sensor_pixel_mode,
            "sensor_pixel_mode_authority": "SOURCE_BOUND" if a.sensor_pixel_mode else "UNKNOWN",
        },
        "sampling": {
            "width": a.width,
            "height": a.height,
            "cfa_topology": a.cfa_topology,
        },
        "initial_capability_placement": {
            "sampling_geometry": f"SOURCE_BOUND_{a.width}x{a.height}",
            "geometry_pose": "UNKNOWN",
            "radiometry": "UNKNOWN",
            "colorimetry": "UNKNOWN",
            "spectral": "UNKNOWN",
            "optical_support": "UNKNOWN",
            "noise_uncertainty": "UNKNOWN",
            "temporal": "UNKNOWN",
            "provenance": "SOURCE_BOUND",
        },
        "invariants": {
            "creates_new_evidence": False,
            "scientific_master_created": False,
            "drawnegative_created": False,
            "calibration_admitted": False,
            "cross_observation_relation_granted": False,
            "fusion_granted": False,
            "appearance_writeback_allowed": False,
        },
    }
    manifest["state_sha256"] = canonical_hash(manifest)
    Path(a.output).write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

    print("DRAW_SOURCE_PRE_ADMISSION_V01_CREATED")
    print("state_sha256=" + manifest["state_sha256"])
    print("status=CANDIDATE_NOT_SCIENTIFICALLY_ADMITTED")
    print("unknown_capabilities=7")

if __name__ == "__main__":
    main()
