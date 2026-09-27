#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

def canonical_hash(doc: dict) -> str:
    x = dict(doc)
    x.pop("state_sha256", None)
    return hashlib.sha256(
        json.dumps(x, sort_keys=True, separators=(",", ":"), ensure_ascii=False, allow_nan=False).encode("utf-8")
    ).hexdigest()

def sha_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()

def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("--source-file", required=True)
    p.add_argument("--acquisition-record", required=True)
    p.add_argument("--lens-role", required=True, choices=["MAIN","ULTRA_WIDE","TELEPHOTO","FRONT","EXTERNAL","UNKNOWN"])
    p.add_argument("--lens-role-authority", default="PROJECT_CANONICAL_DEVICE_MAP")
    p.add_argument("--serialized-payload-sha256", required=True)
    p.add_argument("--serialized-payload-bytes", required=True, type=int)
    p.add_argument("--bits-per-sample", required=True, type=int)
    p.add_argument("--compression", required=True)
    p.add_argument("--photometric", required=True)
    p.add_argument("--container", default="DNG")
    p.add_argument("--focal-length-mm", type=float)
    p.add_argument("--output", required=True)
    a = p.parse_args()

    source_path = Path(a.source_file)
    acq_path = Path(a.acquisition_record)
    acq = json.loads(acq_path.read_text(encoding="utf-8"))
    source_hash = sha_file(source_path)
    recorded = (acq.get("source") or {}).get("sha256")
    if source_hash != recorded:
        raise SystemExit("source bytes do not match acquisition-record SHA-256")

    cam = acq.get("camera") or {}
    topo = acq.get("topology") or {}
    result = acq.get("result") or {}
    authority = acq.get("authority") or {}
    missing = set(authority.get("missingBeforeC0Envelope") or [])

    active = ((cam.get("physicalCameraObservation") or {}).get("value"))
    if (cam.get("physicalCameraObservation") or {}).get("status") != "MEASURED_ACTIVE_PHYSICAL_RESULT":
        raise SystemExit("generator currently requires a measured active physical result")

    payload_sha = a.serialized_payload_sha256.lower()
    if len(payload_sha) != 64:
        raise SystemExit("serialized payload SHA-256 must be 64 hex chars")

    capture_sample_unknown = "captureSampleDomainId" in missing
    readout_unknown = "gainReadoutStateId" in missing

    manifest = {
        "schema": "D.RAW/SourcePreAdmissionManifest/0.2",
        "parent_schema": "D.RAW/SourcePreAdmissionManifest/0.1",
        "status": "CANDIDATE_NOT_SCIENTIFICALLY_ADMITTED",
        "source_evidence": {
            "display_name": (acq.get("source") or {}).get("displayName"),
            "sha256": source_hash,
            "byte_length": source_path.stat().st_size,
            "sealed": True,
            "hash_timing": (acq.get("source") or {}).get("hashTiming"),
            "hash_verified_from_source_bytes": True,
        },
        "acquisition_record": {
            "display_name": acq_path.name,
            "sha256": sha_file(acq_path),
            "authority_class": authority.get("recordClass"),
        },
        "instrument": {
            "device_id": (acq.get("device") or {}).get("model"),
            "logical_camera_id": cam.get("logicalCameraId"),
            "active_physical_camera_id": active,
            "physical_camera_authority": "MEASURED_ACTIVE_PHYSICAL_RESULT",
            "lens_role": a.lens_role,
            "lens_role_authority": a.lens_role_authority,
        },
        "source_route": {
            "route_id": f"CAMERA2_LOGICAL_{cam.get('logicalCameraId')}_ACTIVE_PHYSICAL_{active}_{topo.get('format')}_UNFORCED",
            "authority": "SOURCE_OBSERVATION_BOUND",
            "logical_multi_camera": bool(cam.get("logicalMultiCamera")),
            "requested_physical_camera_id": cam.get("requestedPhysicalCameraId"),
            "result_camera_id": cam.get("resultCameraId"),
        },
        "serialized_storage_domain": {
            "authority": "SOURCE_BOUND",
            "container": a.container,
            "source_format": topo.get("format"),
            "width": topo.get("rawWidth"),
            "height": topo.get("rawHeight"),
            "bits_per_sample": a.bits_per_sample,
            "compression": a.compression,
            "photometric": a.photometric,
            "cfa_pattern": topo.get("cfaPattern"),
            "dynamic_white_level": result.get("dynamicWhiteLevel"),
            "dynamic_black_level": result.get("dynamicBlackLevel"),
            "focal_length_mm": a.focal_length_mm,
            "iso": result.get("sensorSensitivityIso"),
            "exposure_time_ns": result.get("exposureTimeNs"),
            "sensor_info_lens_shading_applied": topo.get("sensorInfoLensShadingApplied"),
        },
        "serialized_cfa_payload": {
            "authority": "SOURCE_BOUND",
            "sha256": payload_sha,
            "byte_length": a.serialized_payload_bytes,
            "exact_serialized_strip_bytes": True,
        },
        "capture_sample_domain": {
            "value": None if capture_sample_unknown else "UNSPECIFIED_SOURCE_BOUND_ID_REQUIRED",
            "authority": "UNKNOWN" if capture_sample_unknown else "SOURCE_BOUND",
            "legacy_blocker": "captureSampleDomainId" if capture_sample_unknown else None,
        },
        "readout_domain": {
            "value": None if readout_unknown else "UNSPECIFIED_SOURCE_BOUND_ID_REQUIRED",
            "authority": "UNKNOWN" if readout_unknown else "SOURCE_BOUND",
            "legacy_blocker": "gainReadoutStateId" if readout_unknown else None,
        },
        "sensor_pixel_mode": {
            "value": None,
            "authority": "UNKNOWN",
            "legacy_blocker": "sensorPixelModeIdentityNotRecorded",
        },
        "timing": {
            "sensor_timestamp_ns": result.get("sensorTimestampNs"),
            "image_timestamp_ns": result.get("imageTimestampNs"),
            "timestamp_match": result.get("timestampMatch"),
        },
        "evidence_counts": {
            "physical_frame_count": authority.get("physicalFrameCountForLaterScene"),
            "independent_evidence_count": authority.get("independentEvidenceCountForLaterScene"),
        },
        "initial_capability_placement": {
            "sampling_geometry": f"SOURCE_BOUND_{topo.get('rawWidth')}x{topo.get('rawHeight')}",
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
            "shared_gauge_admitted": False,
            "cross_observation_relation_granted": False,
            "fusion_granted": False,
            "appearance_writeback_allowed": False,
        },
    }
    manifest["state_sha256"] = canonical_hash(manifest)
    Path(a.output).write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print("DRAW_SOURCE_PRE_ADMISSION_V02_CREATED")
    print("state_sha256=" + manifest["state_sha256"])

if __name__ == "__main__":
    main()
