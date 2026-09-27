#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import sys

HEX64 = re.compile(r"^[0-9a-f]{64}$")
CAPS = {
    "sampling_geometry", "geometry_pose", "radiometry", "colorimetry",
    "spectral", "optical_support", "noise_uncertainty", "temporal",
    "provenance",
}

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

def fail(errors):
    print("DRAW_SOURCE_PRE_ADMISSION_V02_FAIL")
    for e in errors:
        print(e)
    raise SystemExit(1)

def identity_state(block: dict, name: str, errors: list[str]) -> None:
    authority = block.get("authority")
    value = block.get("value")
    if authority == "UNKNOWN":
        if value is not None:
            errors.append(name + ".unknown_requires_null")
    elif authority == "SOURCE_BOUND":
        if not isinstance(value, str) or not value:
            errors.append(name + ".source_bound_requires_value")
    else:
        errors.append(name + ".authority")

def validate(doc: dict) -> list[str]:
    errors: list[str] = []
    if doc.get("schema") != "D.RAW/SourcePreAdmissionManifest/0.2":
        errors.append("schema")
    if doc.get("parent_schema") != "D.RAW/SourcePreAdmissionManifest/0.1":
        errors.append("parent_schema")
    if doc.get("status") != "CANDIDATE_NOT_SCIENTIFICALLY_ADMITTED":
        errors.append("status")

    src = doc.get("source_evidence") or {}
    if HEX64.fullmatch(str(src.get("sha256", ""))) is None:
        errors.append("source.sha256")
    if src.get("sealed") is not True:
        errors.append("source.sealed")
    if src.get("hash_verified_from_source_bytes") is not True:
        errors.append("source.hash_verification")
    if not isinstance(src.get("byte_length"), int) or src.get("byte_length", 0) <= 0:
        errors.append("source.byte_length")

    acq = doc.get("acquisition_record") or {}
    if HEX64.fullmatch(str(acq.get("sha256", ""))) is None:
        errors.append("acquisition.sha256")
    if acq.get("authority_class") != "CAMERA2_ACQUISITION_OBSERVATION_ONLY":
        errors.append("acquisition.authority_class")

    inst = doc.get("instrument") or {}
    if inst.get("physical_camera_authority") != "MEASURED_ACTIVE_PHYSICAL_RESULT":
        errors.append("instrument.physical_camera_authority")
    if not isinstance(inst.get("active_physical_camera_id"), str) or not inst.get("active_physical_camera_id"):
        errors.append("instrument.active_physical_camera_id")
    if inst.get("lens_role") not in {"MAIN", "ULTRA_WIDE", "TELEPHOTO", "FRONT", "EXTERNAL", "UNKNOWN"}:
        errors.append("instrument.lens_role")

    route = doc.get("source_route") or {}
    if route.get("authority") != "SOURCE_OBSERVATION_BOUND":
        errors.append("route.authority")
    if not isinstance(route.get("route_id"), str) or not route.get("route_id"):
        errors.append("route.id")
    if route.get("requested_physical_camera_id") is not None:
        errors.append("route.unforced_candidate_must_keep_requested_physical_null")

    storage = doc.get("serialized_storage_domain") or {}
    for k in ("container", "source_format", "cfa_pattern", "compression", "photometric"):
        if not isinstance(storage.get(k), str) or not storage.get(k):
            errors.append("storage." + k)
    if not isinstance(storage.get("width"), int) or storage.get("width", 0) <= 0:
        errors.append("storage.width")
    if not isinstance(storage.get("height"), int) or storage.get("height", 0) <= 0:
        errors.append("storage.height")
    if not isinstance(storage.get("bits_per_sample"), int) or storage.get("bits_per_sample", 0) <= 0:
        errors.append("storage.bits_per_sample")
    if storage.get("authority") != "SOURCE_BOUND":
        errors.append("storage.authority")

    payload = doc.get("serialized_cfa_payload") or {}
    if payload.get("authority") != "SOURCE_BOUND":
        errors.append("payload.authority")
    if HEX64.fullmatch(str(payload.get("sha256", ""))) is None:
        errors.append("payload.sha256")
    if not isinstance(payload.get("byte_length"), int) or payload.get("byte_length", 0) <= 0:
        errors.append("payload.byte_length")
    if payload.get("exact_serialized_strip_bytes") is not True:
        errors.append("payload.exact_strip_bytes")

    for name in ("capture_sample_domain", "readout_domain", "sensor_pixel_mode"):
        identity_state(doc.get(name) or {}, name, errors)

    timing = doc.get("timing") or {}
    if timing.get("timestamp_match") is not True:
        errors.append("timing.timestamp_match")
    if timing.get("sensor_timestamp_ns") != timing.get("image_timestamp_ns"):
        errors.append("timing.timestamp_equality")

    counts = doc.get("evidence_counts") or {}
    if counts != {"physical_frame_count": 1, "independent_evidence_count": 1}:
        errors.append("evidence_counts")

    caps = doc.get("initial_capability_placement") or {}
    if set(caps) != CAPS:
        errors.append("capabilities.complete")
    if caps.get("sampling_geometry") != f"SOURCE_BOUND_{storage.get('width')}x{storage.get('height')}":
        errors.append("capabilities.sampling_geometry")
    if caps.get("provenance") != "SOURCE_BOUND":
        errors.append("capabilities.provenance")
    for k in CAPS - {"sampling_geometry", "provenance"}:
        if caps.get(k) != "UNKNOWN":
            errors.append("capabilities.preadmission_science:" + k)

    inv = doc.get("invariants") or {}
    expected = {
        "creates_new_evidence": False,
        "scientific_master_created": False,
        "drawnegative_created": False,
        "calibration_admitted": False,
        "shared_gauge_admitted": False,
        "cross_observation_relation_granted": False,
        "fusion_granted": False,
        "appearance_writeback_allowed": False,
    }
    for k, v in expected.items():
        if inv.get(k) is not v:
            errors.append("invariant." + k)

    if doc.get("state_sha256") != canonical_hash(doc):
        errors.append("state_sha256")
    return errors

def main() -> None:
    if len(sys.argv) != 2:
        fail(["usage: validate_draw_source_pre_admission_v02.py MANIFEST.json"])
    try:
        doc = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
    except Exception as exc:
        fail(["json:" + str(exc)])
    errors = validate(doc)
    if errors:
        fail(errors)
    print("DRAW_SOURCE_PRE_ADMISSION_V02_PASS")
    print("status=CANDIDATE_NOT_SCIENTIFICALLY_ADMITTED")
    print("storage_domain=" + doc["serialized_storage_domain"]["authority"])
    print("capture_sample_domain=" + doc["capture_sample_domain"]["authority"])
    print("readout_domain=" + doc["readout_domain"]["authority"])
    print("sensor_pixel_mode=" + doc["sensor_pixel_mode"]["authority"])
    print("creates_new_evidence=false")

if __name__ == "__main__":
    main()
