#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

HEX64 = re.compile(r"^[0-9a-f]{64}$")
CAPABILITY_KEYS = (
    "sampling_geometry", "geometry_pose", "radiometry", "colorimetry",
    "spectral", "optical_support", "noise_uncertainty", "temporal",
    "provenance",
)

def fail(errors):
    print("DRAW_SOURCE_CAPABILITY_ENVELOPE_V01_FAIL")
    for e in errors:
        print(e)
    raise SystemExit(1)

def hex64(v):
    return isinstance(v,str) and HEX64.fullmatch(v) is not None

def digest_without_state(e):
    payload=dict(e)
    payload.pop("state_sha256",None)
    b=json.dumps(payload,sort_keys=True,separators=(",",":"),ensure_ascii=False,allow_nan=False).encode("utf-8")
    return hashlib.sha256(b).hexdigest()

def run_observation_gate(path, errors):
    validator=Path(__file__).with_name("validate_draw_observation_record_v03.py")
    cp=subprocess.run([sys.executable,str(validator),str(path)],capture_output=True,text=True)
    if cp.returncode != 0:
        errors.append("observation_v03_gate:"+(cp.stdout.strip().replace("\n","|") or cp.stderr.strip()))

def unknown_authority(value):
    return isinstance(value,str) and value.startswith("UNKNOWN")

def main():
    if len(sys.argv)!=3:
        fail(["usage: validate_draw_source_capability_envelope_v01.py OBSERVATION_V03.json ENVELOPE.json"])
    opath=Path(sys.argv[1]); epath=Path(sys.argv[2])
    errors=[]
    try:
        o=json.loads(opath.read_text(encoding="utf-8"))
        e=json.loads(epath.read_text(encoding="utf-8"))
    except Exception as exc:
        fail([f"json:{exc}"])

    run_observation_gate(opath,errors)

    if e.get("schema")!="D.RAW/SourceCapabilityEnvelope/0.1":
        errors.append("schema")
    if e.get("observation_record_schema")!="D.RAW/DRAWObservationRecord/0.3":
        errors.append("observation_record_schema")
    if e.get("observation_id")!=o.get("observation_id"):
        errors.append("observation_id")
    if e.get("source_evidence_sha256")!=(o.get("source_evidence") or {}).get("sha256"):
        errors.append("source_evidence")

    inst=e.get("instrument") or {}
    proc=o.get("procedure") or {}
    for ek,ok in (
        ("device_id","device_id"),("camera_id","camera_id"),
        ("lens_role","lens_role"),("capture_pipeline_id","capture_pipeline_id"),
    ):
        if inst.get(ek)!=proc.get(ok):
            errors.append("instrument."+ek)

    sd=e.get("source_domain") or {}
    top=o.get("source_topology") or {}
    if sd.get("source_route_id")!=proc.get("capture_pipeline_id"):
        errors.append("source_domain.route")
    if sd.get("sample_domain_id")!=top.get("sample_domain"):
        errors.append("source_domain.sample")
    if sd.get("readout_domain_authority")=="UNKNOWN" and sd.get("readout_domain_id") is not None:
        errors.append("source_domain.invented_readout")
    if sd.get("sensor_pixel_mode_authority")=="UNKNOWN" and sd.get("sensor_pixel_mode") is not None:
        errors.append("source_domain.invented_pixel_mode")

    sampling=e.get("sampling") or {}
    if sampling.get("width")!=top.get("width") or sampling.get("height")!=top.get("height"):
        errors.append("sampling.geometry")
    if sampling.get("cfa_topology")!=top.get("cfa_topology"):
        errors.append("sampling.cfa")

    dims=o.get("authority_dimensions") or {}
    caps=e.get("capabilities") or {}
    if set(caps)!=set(CAPABILITY_KEYS):
        errors.append("capabilities.complete_map_required")
    for key in CAPABILITY_KEYS:
        c=caps.get(key) or {}
        d=dims.get(key) or {}
        for field in ("authority","identity_sha256","calibration_binding_sha256","validity_domain_sha256"):
            if c.get(field)!=d.get(field):
                errors.append(f"capability.{key}.{field}_mismatch")
        for field in ("identity_sha256","calibration_binding_sha256","validity_domain_sha256"):
            v=c.get(field)
            if v is not None and not hex64(v):
                errors.append(f"capability.{key}.{field}_format")
        if unknown_authority(c.get("authority")):
            if any(c.get(f) is not None for f in ("identity_sha256","calibration_binding_sha256","validity_domain_sha256")):
                errors.append(f"capability.{key}.unknown_must_not_invent_identity")

    oldenv=o.get("capability_envelope") or {}
    dr=e.get("dynamic_range_bounds") or {}
    if dr.get("authority")!=oldenv.get("dynamic_range_bounds"):
        errors.append("dynamic_range.authority")
    ident=dr.get("identity_sha256")
    if ident is not None and not hex64(ident):
        errors.append("dynamic_range.identity")

    g=e.get("gauge") or {}
    og=o.get("gauge") or {}
    if g.get("truthrange_formula")!=og.get("truthrange_formula"):
        errors.append("gauge.formula")
    if g.get("scale_gauge_id")!=og.get("scale_gauge_id"):
        errors.append("gauge.id")
    if g.get("relation")!=og.get("gauge_relation"):
        errors.append("gauge.relation")
    if g.get("shared_free_world_gauge_id")!=og.get("shared_free_world_gauge_id"):
        errors.append("gauge.shared")
    if g.get("relation")=="SOURCE_LOCAL_ONLY" and g.get("shared_free_world_gauge_id") is not None:
        errors.append("gauge.source_local_shared")

    if e.get("temporal_footprint")!=o.get("temporal_footprint"):
        errors.append("temporal_footprint")

    eo=e.get("optical_support") or {}
    oo=o.get("optical_support") or {}
    for k in ("authority","model_kind","support_identity_sha256"):
        if eo.get(k)!=oo.get(k):
            errors.append("optical_support."+k)
    if unknown_authority(eo.get("authority")) and (
        eo.get("model_kind")!="NONE" or eo.get("support_identity_sha256") is not None
    ):
        errors.append("optical_support.unknown_model_claim")

    eb=e.get("calibration_bindings") or []
    ob=o.get("calibration_bindings") or []
    if len(eb)!=len(ob):
        errors.append("calibration_bindings.count")
    if eb:
        if sd.get("readout_domain_id") is None or sd.get("sensor_pixel_mode") is None:
            errors.append("calibration_bindings.require_explicit_readout_and_pixel_mode")
        # v0.1 requires each envelope binding to retain the parent v0.3 triplet.
        parent={(x.get("kind"),x.get("calibration_record_sha256"),x.get("validity_domain_sha256"),x.get("holdout_report_sha256")) for x in ob}
        child={(x.get("kind"),x.get("calibration_record_sha256"),x.get("validity_domain_sha256"),x.get("holdout_report_sha256")) for x in eb}
        if child!=parent:
            errors.append("calibration_bindings.parent_mismatch")

    inv=e.get("invariants") or {}
    expected={
        "unknown_is_valid_state":True,
        "lens_identity_upgrades_authority":False,
        "file_format_upgrades_authority":False,
        "implicit_calibration_transfer_allowed":False,
        "cross_observation_relation_granted":False,
        "cross_observation_fusion_granted":False,
        "creates_new_evidence":False,
        "scientific_writeback_allowed":False,
        "appearance_writeback_allowed":False,
    }
    for k,v in expected.items():
        if inv.get(k) is not v:
            errors.append("invariant."+k)

    expected_digest=digest_without_state(e)
    if e.get("state_sha256")!=expected_digest:
        errors.append("state_sha256:"+expected_digest)

    if errors:
        fail(errors)

    print("DRAW_SOURCE_CAPABILITY_ENVELOPE_V01_PASS")
    print("observation_id="+e["observation_id"])
    print("state_sha256="+e["state_sha256"])
    print("capability_positions="+str(len(caps)))
    print("unknown_is_valid_state=true")
    print("cross_observation_relation_granted=false")
    print("cross_observation_fusion_granted=false")
    print("creates_new_evidence=false")

if __name__=="__main__":
    main()
