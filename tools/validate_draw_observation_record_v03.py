#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile

HEX64 = re.compile(r"^[0-9a-f]{64}$")
NEW_FIELDS = {
    "parent_record_schema", "graph_node", "knowledge_placement",
    "authority_dimensions", "temporal_footprint", "optical_support",
    "calibration_bindings", "knowledge_growth_invariants",
}
REQUIRED_DOMAINS = {
    "SOURCE_EVIDENCE", "SAMPLING_GEOMETRY", "GEOMETRY_POSE", "RADIOMETRY",
    "COLORIMETRY", "SPECTRAL", "OPTICAL_SUPPORT", "NOISE_UNCERTAINTY",
    "TEMPORAL", "PROVENANCE",
}
DIMENSION_TO_DOMAIN = {
    "sampling_geometry": "SAMPLING_GEOMETRY",
    "geometry_pose": "GEOMETRY_POSE",
    "radiometry": "RADIOMETRY",
    "colorimetry": "COLORIMETRY",
    "spectral": "SPECTRAL",
    "optical_support": "OPTICAL_SUPPORT",
    "noise_uncertainty": "NOISE_UNCERTAINTY",
    "temporal": "TEMPORAL",
    "provenance": "PROVENANCE",
}
LENS = {"MAIN":1, "ULTRA_WIDE":2, "TELEPHOTO":3, "FRONT":4, "EXTERNAL":5, "UNKNOWN":6}
TEMPORAL = {"UNKNOWN":1, "CAPTURE_TIMESTAMP_ONLY":2, "EXPOSURE_INTERVAL":3, "GLOBAL_SHUTTER_INTERVAL":4, "ROLLING_SHUTTER_MODEL":5}
CAL_KIND_TO_DIM = {
    "GAIN_NOISE":"noise_uncertainty",
    "OPTICS_PSF_MTF":"optical_support",
    "COLOR":"colorimetry",
    "SPECTRAL_RESPONSE":"spectral",
}

def is_hex64(v):
    return isinstance(v, str) and HEX64.fullmatch(v) is not None

def fail(errors):
    print("DRAW_OBSERVATION_RECORD_V03_FAIL")
    for e in errors:
        print(e)
    raise SystemExit(1)

def hash_string(h, value):
    b=value.encode("utf-8")
    h.update(struct.pack("<Q", len(b)))
    h.update(b)

def graph_node_sha(r):
    tf=r["temporal_footprint"]
    h=hashlib.sha256()
    h.update(b"D_RAW_FREE_WORLD_OBSERVATION_NODE_V0_1")
    hash_string(h,r["observation_id"])
    h.update(bytes.fromhex(r["drawnegative"]["state_sha256"]))
    h.update(bytes.fromhex(r["source_evidence"]["sha256"]))
    h.update(bytes([LENS[r["procedure"]["lens_role"]]]))
    hash_string(h,r["gauge"]["scale_gauge_id"])
    h.update(bytes([TEMPORAL[tf["model"]]]))
    h.update(bytes([1 if tf["exposure_interval_known"] else 0]))
    h.update(bytes([1 if tf["rolling_shutter_model_known"] else 0]))
    h.update(struct.pack("<I",r["evidence_counts"]["physical_frame_count"]))
    h.update(struct.pack("<I",r["evidence_counts"]["independent_evidence_count"]))
    return h.hexdigest()

def validate_parent_v02(r, errors):
    parent={k:v for k,v in r.items() if k not in NEW_FIELDS}
    parent["schema"]="D.RAW/DRAWObservationRecord/0.2"
    validator=Path(__file__).with_name("validate_draw_observation_record_v02.py")
    with tempfile.NamedTemporaryFile("w",suffix=".json",delete=False) as f:
        json.dump(parent,f)
        name=f.name
    try:
        cp=subprocess.run([sys.executable,str(validator),name],capture_output=True,text=True)
        if cp.returncode != 0:
            errors.append("parent_v02_gate:" + (cp.stdout.strip().replace("\n","|") or cp.stderr.strip()))
    finally:
        Path(name).unlink(missing_ok=True)

def validate_temporal(tf, errors):
    model=tf.get("model")
    ts=tf.get("capture_timestamp_identity_sha256")
    interval=tf.get("exposure_interval_known")
    rolling=tf.get("rolling_shutter_model_known")
    if ts is not None and not is_hex64(ts):
        errors.append("temporal.timestamp_identity")
    if model == "UNKNOWN":
        if ts is not None or interval is not False or rolling is not False:
            errors.append("temporal.unknown_must_stay_unknown")
    elif model == "CAPTURE_TIMESTAMP_ONLY":
        if not is_hex64(ts) or interval is not False or rolling is not False:
            errors.append("temporal.timestamp_only")
    elif model in {"EXPOSURE_INTERVAL","GLOBAL_SHUTTER_INTERVAL","ROLLING_SHUTTER_MODEL"}:
        if interval is not True:
            errors.append("temporal.interval_required")
        a,b=tf.get("exposure_start_ns"),tf.get("exposure_end_ns")
        if not isinstance(a,int) or not isinstance(b,int) or b < a:
            errors.append("temporal.interval_values")
        if model == "ROLLING_SHUTTER_MODEL":
            if rolling is not True or not isinstance(tf.get("row_readout_time_ns"),int) or tf["row_readout_time_ns"] <= 0:
                errors.append("temporal.rolling_model")
        elif rolling is not False:
            errors.append("temporal.unexpected_rolling")
    else:
        errors.append("temporal.model")

def main():
    if len(sys.argv) != 2:
        fail(["usage: validate_draw_observation_record_v03.py RECORD.json"])
    try:
        r=json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
    except Exception as exc:
        fail([f"json:{exc}"])
    errors=[]
    if r.get("schema") != "D.RAW/DRAWObservationRecord/0.3":
        errors.append("schema")
    if r.get("parent_record_schema") != "D.RAW/DRAWObservationRecord/0.2":
        errors.append("parent_record_schema")

    validate_parent_v02(r,errors)

    placements={}
    for i,p in enumerate(r.get("knowledge_placement") or []):
        domain=p.get("domain")
        if domain in placements:
            errors.append(f"placement[{i}].duplicate_domain")
            continue
        placements[domain]=p
        if not isinstance(p.get("authority"),str) or not p["authority"]:
            errors.append(f"placement[{i}].authority")
        ident=p.get("identity_sha256")
        if ident is not None and not is_hex64(ident):
            errors.append(f"placement[{i}].identity")
        if p.get("creates_new_evidence") is not False:
            errors.append(f"placement[{i}].creates_new_evidence")
        floor=p.get("floor")
        if domain == "SOURCE_EVIDENCE" and floor != "EVIDENCE":
            errors.append("placement.source_floor")
        elif domain == "APPEARANCE" and floor != "APPEARANCE":
            errors.append("placement.appearance_floor")
        elif domain in {"MATERIAL","ILLUMINATION","RESTORATION"} and floor != "SCENE_MODEL":
            errors.append(f"placement.{domain.lower()}_floor")
        elif domain not in {"SOURCE_EVIDENCE","APPEARANCE","MATERIAL","ILLUMINATION","RESTORATION"} and floor != "SCIENTIFIC_DERIVED":
            errors.append(f"placement.{domain}.floor")
    missing=REQUIRED_DOMAINS-set(placements)
    if missing:
        errors.append("placement.missing:" + ",".join(sorted(missing)))
    source_p=placements.get("SOURCE_EVIDENCE") or {}
    if source_p.get("identity_sha256") != (r.get("source_evidence") or {}).get("sha256"):
        errors.append("placement.source_identity")

    dims=r.get("authority_dimensions") or {}
    for name,domain in DIMENSION_TO_DOMAIN.items():
        d=dims.get(name) or {}
        p=placements.get(domain) or {}
        if d.get("authority") != p.get("authority"):
            errors.append(f"dimension.{name}.authority_mismatch")
        if d.get("identity_sha256") != p.get("identity_sha256"):
            errors.append(f"dimension.{name}.identity_mismatch")
        for k in ("identity_sha256","calibration_binding_sha256","validity_domain_sha256"):
            v=d.get(k)
            if v is not None and not is_hex64(v):
                errors.append(f"dimension.{name}.{k}")
        if d.get("cross_observation_relation_admitted") is not False:
            errors.append(f"dimension.{name}.relation_must_live_in_graph")
        if d.get("implicit_transfer_allowed") is not False:
            errors.append(f"dimension.{name}.implicit_transfer")

    legacy_axes=r.get("authority_axes") or {}
    if legacy_axes.get("geometry_authority") != (dims.get("geometry_pose") or {}).get("authority"):
        errors.append("legacy_geometry_axis_mismatch")
    if legacy_axes.get("radiometric_authority") != (dims.get("radiometry") or {}).get("authority"):
        errors.append("legacy_radiometric_axis_mismatch")

    tf=r.get("temporal_footprint") or {}
    validate_temporal(tf,errors)
    if (r.get("graph_node") or {}).get("temporal_model") != tf.get("model"):
        errors.append("graph_node.temporal_mismatch")

    try:
        expected=graph_node_sha(r)
        if (r.get("graph_node") or {}).get("state_sha256") != expected:
            errors.append("graph_node.digest:" + expected)
    except Exception as exc:
        errors.append("graph_node.compute:" + str(exc))
    graph=r.get("graph_node") or {}
    if graph.get("schema") != "D.RAW/FreeWorldObservationGraphNative/0.1":
        errors.append("graph_node.schema")
    if graph.get("creates_new_evidence") is not False or graph.get("scientific_writeback_allowed") is not False:
        errors.append("graph_node.authority")

    optics=r.get("optical_support") or {}
    if optics.get("authority") != (dims.get("optical_support") or {}).get("authority"):
        errors.append("optics.authority_mismatch")
    if optics.get("numeric_scene_value_changed") is not False or optics.get("authority_upgraded") is not False:
        errors.append("optics.must_not_modify_or_upgrade")
    if str(optics.get("authority","")).startswith("UNKNOWN"):
        if optics.get("model_kind") != "NONE" or optics.get("support_identity_sha256") is not None:
            errors.append("optics.unknown_model_claim")
        for k in ("field_dependent","spatial_frequency_dependent","focus_dependent","spectral_or_channel_dependent"):
            if optics.get(k) is not False:
                errors.append("optics.unknown_dependency_claim:" + k)
    elif optics.get("model_kind") == "NONE" or not is_hex64(optics.get("support_identity_sha256")):
        errors.append("optics.known_requires_model_identity")

    seen=set()
    for i,b in enumerate(r.get("calibration_bindings") or []):
        kind=b.get("kind")
        if kind in seen:
            errors.append(f"calibration[{i}].duplicate_kind")
        seen.add(kind)
        for k in ("calibration_record_sha256","validity_domain_sha256","holdout_report_sha256"):
            if not is_hex64(b.get(k)):
                errors.append(f"calibration[{i}].{k}")
        if b.get("applicable") is not True or b.get("admitted") is not True:
            errors.append(f"calibration[{i}].admission")
        dim_name=CAL_KIND_TO_DIM.get(kind)
        if dim_name:
            d=dims.get(dim_name) or {}
            if d.get("calibration_binding_sha256") != b.get("calibration_record_sha256"):
                errors.append(f"calibration[{i}].dimension_binding")
            if d.get("validity_domain_sha256") != b.get("validity_domain_sha256"):
                errors.append(f"calibration[{i}].validity_binding")

    inv=r.get("knowledge_growth_invariants") or {}
    for k in (
        "source_evidence_mutated","drawnegative_v0_1_mutated",
        "relation_promotes_source_evidence","implicit_calibration_transfer_allowed",
        "cross_observation_fusion_granted_by_record","unknown_upgraded_by_appearance",
    ):
        if inv.get(k) is not False:
            errors.append("invariant." + k)

    if errors:
        fail(errors)

    print("DRAW_OBSERVATION_RECORD_V03_PASS")
    print("parent_v02_gate=PASS")
    print("graph_node_sha256=" + r["graph_node"]["state_sha256"])
    print("knowledge_domains=" + str(len(placements)))
    print("calibration_bindings=" + str(len(r.get("calibration_bindings") or [])))
    print("cross_observation_fusion_granted_by_record=false")
    print("source_evidence_mutated=false")

if __name__ == "__main__":
    main()
