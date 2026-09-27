#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

HEX64 = re.compile(r"^[0-9a-f]{64}$")
CAPS = {
    "sampling_geometry", "geometry_pose", "radiometry", "colorimetry",
    "spectral", "optical_support", "noise_uncertainty", "temporal",
    "provenance",
}

def hashdoc(doc: dict) -> str:
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

def canon(doc: dict) -> str:
    return hashlib.sha256(
        json.dumps(
            doc,
            sort_keys=True,
            separators=(",", ":"),
            ensure_ascii=False,
            allow_nan=False,
        ).encode("utf-8")
    ).hexdigest()

def fail(errors, tag):
    print(tag + "_FAIL")
    for e in errors:
        print(e)
    raise SystemExit(1)

def validate_pre(path):
    try:
        d = json.loads(Path(path).read_text(encoding="utf-8"))
    except Exception as exc:
        fail(["json:" + str(exc)], "DRAW_SOURCE_PRE_ADMISSION_V01")

    errors = []
    if d.get("schema") != "D.RAW/SourcePreAdmissionManifest/0.1":
        errors.append("schema")
    if d.get("status") != "CANDIDATE_NOT_SCIENTIFICALLY_ADMITTED":
        errors.append("status")

    src = d.get("source_evidence") or {}
    if HEX64.fullmatch(str(src.get("sha256", ""))) is None or src.get("sealed") is not True:
        errors.append("source")

    inst = d.get("instrument") or {}
    sd = d.get("source_domain") or {}
    samp = d.get("sampling") or {}
    for k in ("device_id", "camera_id", "capture_pipeline_id"):
        if not isinstance(inst.get(k), str) or not inst[k]:
            errors.append("instrument." + k)
    if sd.get("source_route_id") != inst.get("capture_pipeline_id"):
        errors.append("route_mismatch")
    if not isinstance(sd.get("sample_domain_id"), str) or not sd.get("sample_domain_id"):
        errors.append("sample_domain")
    if sd.get("readout_domain_authority") == "UNKNOWN" and sd.get("readout_domain_id") is not None:
        errors.append("invented_readout")
    if sd.get("sensor_pixel_mode_authority") == "UNKNOWN" and sd.get("sensor_pixel_mode") is not None:
        errors.append("invented_pixel_mode")
    if sd.get("readout_domain_authority") == "SOURCE_BOUND" and not sd.get("readout_domain_id"):
        errors.append("missing_readout_identity")
    if sd.get("sensor_pixel_mode_authority") == "SOURCE_BOUND" and not sd.get("sensor_pixel_mode"):
        errors.append("missing_pixel_mode_identity")

    if (
        not isinstance(samp.get("width"), int)
        or samp.get("width", 0) <= 0
        or not isinstance(samp.get("height"), int)
        or samp.get("height", 0) <= 0
        or not isinstance(samp.get("cfa_topology"), str)
        or not samp.get("cfa_topology")
    ):
        errors.append("sampling")

    caps = d.get("initial_capability_placement") or {}
    if set(caps) != CAPS:
        errors.append("capability_map_complete")
    for k, v in caps.items():
        if not isinstance(v, str) or not v:
            errors.append("capability." + k)

    # Pre-admission may only assert source-bound sampling/provenance.
    expected_unknown = {
        "geometry_pose", "radiometry", "colorimetry", "spectral",
        "optical_support", "noise_uncertainty", "temporal",
    }
    for k in expected_unknown:
        if caps.get(k) != "UNKNOWN":
            errors.append("pre_admission_scientific_claim:" + k)
    if caps.get("provenance") != "SOURCE_BOUND":
        errors.append("pre_admission_provenance")
    if caps.get("sampling_geometry") != f"SOURCE_BOUND_{samp.get('width')}x{samp.get('height')}":
        errors.append("pre_admission_sampling_authority")

    inv = d.get("invariants") or {}
    expected = {
        "creates_new_evidence": False,
        "scientific_master_created": False,
        "drawnegative_created": False,
        "calibration_admitted": False,
        "cross_observation_relation_granted": False,
        "fusion_granted": False,
        "appearance_writeback_allowed": False,
    }
    for k, v in expected.items():
        if inv.get(k) is not v:
            errors.append("invariant." + k)

    if d.get("state_sha256") != hashdoc(d):
        errors.append("state_sha256")

    if errors:
        fail(errors, "DRAW_SOURCE_PRE_ADMISSION_V01")

    print("DRAW_SOURCE_PRE_ADMISSION_V01_PASS")
    print("status=CANDIDATE_NOT_SCIENTIFICALLY_ADMITTED")
    return d

def run(cmd, label, errors):
    cp = subprocess.run(cmd, capture_output=True, text=True)
    if cp.returncode != 0:
        errors.append(
            label + ":" + (
                cp.stdout.strip().replace("\n", "|")
                or cp.stderr.strip().replace("\n", "|")
            )
        )

def validate_final(obs_path, env_path, pkg_path):
    errors = []
    here = Path(__file__).resolve().parent
    run(
        [sys.executable, str(here / "validate_draw_observation_record_v03.py"), str(obs_path)],
        "observation_v03",
        errors,
    )
    run(
        [
            sys.executable,
            str(here / "validate_draw_source_capability_envelope_v01.py"),
            str(obs_path),
            str(env_path),
        ],
        "capability_envelope_v01",
        errors,
    )

    try:
        o = json.loads(Path(obs_path).read_text(encoding="utf-8"))
        e = json.loads(Path(env_path).read_text(encoding="utf-8"))
        p = json.loads(Path(pkg_path).read_text(encoding="utf-8"))
    except Exception as exc:
        fail(["json:" + str(exc)], "DRAW_SOURCE_ADMISSION_PACKAGE_V01")

    if p.get("schema") != "D.RAW/SourceAdmissionPackage/0.1":
        errors.append("schema")
    if p.get("status") != "ADMITTED_SOURCE_LOCAL":
        errors.append("status")

    oid = o.get("observation_id")
    src = (o.get("source_evidence") or {}).get("sha256")
    if p.get("observation_id") != oid or e.get("observation_id") != oid:
        errors.append("observation_id")
    if p.get("source_evidence_sha256") != src or e.get("source_evidence_sha256") != src:
        errors.append("source_evidence")

    proc = o.get("procedure") or {}
    inst = e.get("instrument") or {}
    for k in ("device_id", "camera_id", "lens_role", "capture_pipeline_id"):
        if proc.get(k) != inst.get(k):
            errors.append("instrument_binding." + k)

    orc = p.get("observation_record") or {}
    if orc.get("schema") != "D.RAW/DRAWObservationRecord/0.3":
        errors.append("observation_record.schema")
    if orc.get("canonical_sha256") != canon(o):
        errors.append("observation_record.hash")
    if orc.get("graph_node_sha256") != (o.get("graph_node") or {}).get("state_sha256"):
        errors.append("graph_node")

    ce = p.get("capability_envelope") or {}
    if ce.get("schema") != "D.RAW/SourceCapabilityEnvelope/0.1":
        errors.append("envelope.schema")
    if ce.get("state_sha256") != e.get("state_sha256"):
        errors.append("envelope.state")

    pg = p.get("gauge") or {}
    og = o.get("gauge") or {}
    eg = e.get("gauge") or {}
    if (
        pg.get("relation") != "SOURCE_LOCAL_ONLY"
        or og.get("gauge_relation") != "SOURCE_LOCAL_ONLY"
        or eg.get("relation") != "SOURCE_LOCAL_ONLY"
    ):
        errors.append("gauge.not_source_local")
    if pg.get("scale_gauge_id") != og.get("scale_gauge_id") or pg.get("scale_gauge_id") != eg.get("scale_gauge_id"):
        errors.append("gauge.id")
    if (
        pg.get("shared_free_world_gauge_id") is not None
        or og.get("shared_free_world_gauge_id") is not None
        or eg.get("shared_free_world_gauge_id") is not None
    ):
        errors.append("gauge.shared")

    counts = p.get("evidence_counts") or {}
    oc = o.get("evidence_counts") or {}
    if counts != {"physical_frame_count": 1, "independent_evidence_count": 1} or oc != counts:
        errors.append("counts")

    if (
        p.get("calibration_binding_count") != len(o.get("calibration_bindings") or [])
        or p.get("calibration_binding_count") != len(e.get("calibration_bindings") or [])
    ):
        errors.append("calibration_count")

    if p.get("graph_relations") != []:
        errors.append("relations_must_be_empty")
    if p.get("fusion_admissions") != []:
        errors.append("fusion_must_be_empty")

    inv = p.get("invariants") or {}
    expected = {
        "source_evidence_mutated": False,
        "scientific_master_mutated_by_admission": False,
        "drawnegative_mutated_by_admission": False,
        "calibration_transfer_implied": False,
        "cross_observation_relation_granted": False,
        "cross_observation_fusion_granted": False,
        "appearance_writeback_allowed": False,
    }
    for k, v in expected.items():
        if inv.get(k) is not v:
            errors.append("invariant." + k)

    if p.get("state_sha256") != hashdoc(p):
        errors.append("state_sha256")

    if errors:
        fail(errors, "DRAW_SOURCE_ADMISSION_PACKAGE_V01")

    print("DRAW_SOURCE_ADMISSION_PACKAGE_V01_PASS")
    print("status=ADMITTED_SOURCE_LOCAL")
    print("observation_id=" + str(oid))
    print("source_evidence_sha256=" + str(src))
    print("graph_relations=0")
    print("fusion_admissions=0")

def main():
    if len(sys.argv) < 3:
        raise SystemExit(
            "usage: validate_draw_source_admission_package_v01.py "
            "pre PRE.json | final OBS.json ENVELOPE.json PACKAGE.json"
        )
    if sys.argv[1] == "pre" and len(sys.argv) == 3:
        validate_pre(sys.argv[2])
        return
    if sys.argv[1] == "final" and len(sys.argv) == 5:
        validate_final(sys.argv[2], sys.argv[3], sys.argv[4])
        return
    raise SystemExit(2)

if __name__ == "__main__":
    main()
