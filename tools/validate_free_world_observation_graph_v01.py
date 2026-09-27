#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

HEX64 = re.compile(r"^[0-9a-f]{64}$")

FLOORS = {
    "EVIDENCE", "SCIENTIFIC_DERIVED", "RELATION", "SCENE_MODEL", "APPEARANCE"
}
DOMAINS = {
    "SOURCE_EVIDENCE", "SAMPLING_GEOMETRY", "GEOMETRY_POSE", "RADIOMETRY",
    "COLORIMETRY", "SPECTRAL", "OPTICAL_SUPPORT", "NOISE_UNCERTAINTY",
    "TEMPORAL", "MATERIAL", "ILLUMINATION", "PROVENANCE", "RESTORATION",
    "APPEARANCE"
}
AXES = {
    "GEOMETRY", "RADIOMETRIC_GAUGE", "COLORIMETRIC", "SPECTRAL",
    "OPTICAL_SUPPORT", "UNCERTAINTY_CORRELATION", "TEMPORAL", "PROVENANCE"
}
STATUSES = {
    "UNKNOWN", "HYPOTHESIS", "SOURCE_BOUND", "CALIBRATED", "ADMITTED", "REJECTED"
}
LENS_ROLES = {"MAIN", "ULTRA_WIDE", "TELEPHOTO", "FRONT", "EXTERNAL", "UNKNOWN"}
VIEW_KINDS = {"VIRTUAL_EV", "VIRTUAL_GAIN", "VIRTUAL_VIEW", "COMPATIBILITY_PROJECTION"}

FUSION_REQUIREMENTS = {
    "RADIOMETRIC": {"RADIOMETRIC_GAUGE", "UNCERTAINTY_CORRELATION", "TEMPORAL"},
    "COLOR": {"RADIOMETRIC_GAUGE", "COLORIMETRIC", "UNCERTAINTY_CORRELATION", "TEMPORAL"},
    "SPATIAL_DETAIL": {"GEOMETRY", "OPTICAL_SUPPORT", "UNCERTAINTY_CORRELATION", "TEMPORAL"},
}


def is_hex64(v):
    return isinstance(v, str) and HEX64.fullmatch(v) is not None


def fail(errors):
    print("FREE_WORLD_OBSERVATION_GRAPH_V01_FAIL")
    for e in errors:
        print(e)
    raise SystemExit(1)


def validate(doc):
    errors = []
    if doc.get("schema") != "D.RAW/FreeWorldObservationGraph/0.1":
        errors.append("schema")

    observations = doc.get("observations")
    if not isinstance(observations, list) or not observations:
        errors.append("observations")
        observations = []

    obs_ids = set()
    for i, obs in enumerate(observations):
        oid = obs.get("observation_id")
        if not isinstance(oid, str) or not oid or oid in obs_ids:
            errors.append(f"observation[{i}].id")
        else:
            obs_ids.add(oid)
        if not is_hex64(obs.get("drawnegative_state_sha256")):
            errors.append(f"observation[{i}].drawnegative")
        if not is_hex64(obs.get("source_evidence_sha256")):
            errors.append(f"observation[{i}].source")
        if obs.get("lens_role") not in LENS_ROLES:
            errors.append(f"observation[{i}].lens_role")
        if obs.get("physical_frame_count") != 1:
            errors.append(f"observation[{i}].physical_frame_count")
        if obs.get("independent_evidence_count") != 1:
            errors.append(f"observation[{i}].independent_evidence_count")
        if not isinstance(obs.get("source_local_gauge_id"), str) or not obs.get("source_local_gauge_id"):
            errors.append(f"observation[{i}].gauge")
        tf = obs.get("temporal_footprint") or {}
        if tf.get("model") not in {
            "UNKNOWN", "CAPTURE_TIMESTAMP_ONLY", "EXPOSURE_INTERVAL",
            "GLOBAL_SHUTTER_INTERVAL", "ROLLING_SHUTTER_MODEL"
        }:
            errors.append(f"observation[{i}].temporal_model")
        if tf.get("exposure_interval_known") is not True and (
            "exposure_start" in tf or "exposure_end" in tf
        ):
            errors.append(f"observation[{i}].invented_exposure_interval")

    info_ids = set()
    for i, item in enumerate(doc.get("information") or []):
        iid = item.get("information_id")
        if not isinstance(iid, str) or not iid or iid in info_ids:
            errors.append(f"information[{i}].id")
        else:
            info_ids.add(iid)
        if item.get("observation_id") not in obs_ids:
            errors.append(f"information[{i}].observation")
        if item.get("domain") not in DOMAINS:
            errors.append(f"information[{i}].domain")
        if item.get("floor") not in FLOORS:
            errors.append(f"information[{i}].floor")
        if item.get("creates_new_evidence") is not False:
            errors.append(f"information[{i}].creates_new_evidence")
        ident = item.get("identity_sha256")
        if ident is not None and not is_hex64(ident):
            errors.append(f"information[{i}].identity")
        if item.get("domain") == "APPEARANCE" and item.get("floor") != "APPEARANCE":
            errors.append(f"information[{i}].appearance_floor")

    relations = {}
    for i, rel in enumerate(doc.get("relations") or []):
        rid = rel.get("relation_id")
        if not isinstance(rid, str) or not rid or rid in relations:
            errors.append(f"relation[{i}].id")
            continue
        relations[rid] = rel
        a, b = rel.get("observation_a"), rel.get("observation_b")
        if a not in obs_ids or b not in obs_ids or a == b:
            errors.append(f"relation[{i}].endpoints")
        axis = rel.get("axis")
        status = rel.get("status")
        if axis not in AXES:
            errors.append(f"relation[{i}].axis")
        if status not in STATUSES:
            errors.append(f"relation[{i}].status")
        cert = rel.get("certificate_sha256")
        if status in {"CALIBRATED", "ADMITTED"} and not is_hex64(cert):
            errors.append(f"relation[{i}].certificate")
        if status in {"UNKNOWN", "HYPOTHESIS", "SOURCE_BOUND", "REJECTED"} and cert is not None:
            if not is_hex64(cert):
                errors.append(f"relation[{i}].certificate_format")
        cap = rel.get("capabilities") or {}
        if cap.get("fusion_allowed") is not False:
            errors.append(f"relation[{i}].direct_fusion_forbidden")
        if cap.get("calibration_transfer_allowed") is not False:
            errors.append(f"relation[{i}].implicit_calibration_transfer")
        if status != "ADMITTED" and (
            cap.get("coordinate_transform_allowed") is True or
            cap.get("equality_allowed") is True
        ):
            errors.append(f"relation[{i}].capability_without_admission")
        if cap.get("equality_allowed") is True and axis not in {
            "RADIOMETRIC_GAUGE", "COLORIMETRIC"
        }:
            errors.append(f"relation[{i}].invalid_equality_axis")

    for i, adm in enumerate(doc.get("fusion_admissions") or []):
        kind = adm.get("kind")
        if kind not in FUSION_REQUIREMENTS:
            errors.append(f"fusion[{i}].kind")
            continue
        if adm.get("admitted") is not True or not is_hex64(adm.get("certificate_sha256")):
            errors.append(f"fusion[{i}].certificate")
        ids = adm.get("relation_ids")
        if not isinstance(ids, list) or not ids:
            errors.append(f"fusion[{i}].relations")
            continue
        axes = set()
        endpoints = None
        for rid in ids:
            rel = relations.get(rid)
            if rel is None:
                errors.append(f"fusion[{i}].missing_relation:{rid}")
                continue
            if rel.get("status") != "ADMITTED":
                errors.append(f"fusion[{i}].unadmitted_relation:{rid}")
            axes.add(rel.get("axis"))
            pair = frozenset((rel.get("observation_a"), rel.get("observation_b")))
            if endpoints is None:
                endpoints = pair
            elif pair != endpoints:
                errors.append(f"fusion[{i}].mixed_endpoints")
        missing = FUSION_REQUIREMENTS[kind] - axes
        if missing:
            errors.append(f"fusion[{i}].missing_axes:{','.join(sorted(missing))}")

    for i, view in enumerate(doc.get("virtual_views") or []):
        if view.get("parent_observation_id") not in obs_ids:
            errors.append(f"view[{i}].parent")
        if view.get("kind") not in VIEW_KINDS:
            errors.append(f"view[{i}].kind")
        if view.get("independent_evidence_increment") != 0:
            errors.append(f"view[{i}].evidence_increment")
        if view.get("creates_new_evidence") is not False:
            errors.append(f"view[{i}].creates_new_evidence")

    growth = doc.get("knowledge_growth") or {}
    if growth.get("source_evidence_mutated") is not False:
        errors.append("growth.source_mutation")
    if growth.get("new_information_adds_context_only") is not True:
        errors.append("growth.context_only")
    if growth.get("new_relations_require_certificate_for_admission") is not True:
        errors.append("growth.certificate_gate")

    inv = doc.get("invariants") or {}
    expected = {
        "lens_identity_upgrades_authority": False,
        "relation_promotes_source_evidence": False,
        "appearance_writeback_allowed": False,
        "representation_can_exceed_source": True,
        "knowledge_claims_may_exceed_evidence": False,
    }
    for k, v in expected.items():
        if inv.get(k) is not v:
            errors.append(f"invariant.{k}")

    return errors


def main():
    if len(sys.argv) != 2:
        fail(["usage: validate_free_world_observation_graph_v01.py GRAPH.json"])
    try:
        doc = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
    except Exception as exc:
        fail([f"json:{exc}"])
    errors = validate(doc)
    if errors:
        fail(errors)
    print("FREE_WORLD_OBSERVATION_GRAPH_V01_PASS")
    print(f"graph_id={doc.get('graph_id')}")
    print(f"observations={len(doc.get('observations') or [])}")
    print(f"relations={len(doc.get('relations') or [])}")
    print(f"fusion_admissions={len(doc.get('fusion_admissions') or [])}")
    print("source_evidence_mutated=false")
    print("knowledge_growth=context_and_relations_only")


if __name__ == "__main__":
    main()
