#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

CAP_KEYS = {
    "sampling_geometry", "geometry_pose", "radiometry", "colorimetry",
    "spectral", "optical_support", "noise_uncertainty", "temporal",
    "provenance",
}
LENS = {"MAIN": 1, "ULTRA_WIDE": 2, "TELEPHOTO": 3, "FRONT": 4, "EXTERNAL": 5, "UNKNOWN": 6}
TEMPORAL = {"UNKNOWN": 1, "CAPTURE_TIMESTAMP_ONLY": 2, "EXPOSURE_INTERVAL": 3, "GLOBAL_SHUTTER_INTERVAL": 4, "ROLLING_SHUTTER_MODEL": 5}

def fail(tag: str, errors: list[str]) -> None:
    print(tag + "_FAIL")
    for e in errors:
        print(e)
    raise SystemExit(1)

def digest(doc: dict, key: str) -> str:
    x = dict(doc)
    x.pop(key, None)
    return hashlib.sha256(
        json.dumps(x, sort_keys=True, separators=(",", ":"), ensure_ascii=False, allow_nan=False).encode()
    ).hexdigest()

def hstr(h, s: str) -> None:
    b = s.encode()
    h.update(struct.pack("<Q", len(b)))
    h.update(b)

def graph_sha(obs_id: str, draw_sha: str, source_sha: str, lens_role: str, gauge_id: str,
              temporal: str, exposure_known: bool, rolling_known: bool, frames: int, evidence: int) -> str:
    h = hashlib.sha256()
    h.update(b"D_RAW_FREE_WORLD_OBSERVATION_NODE_V0_1")
    hstr(h, obs_id)
    h.update(bytes.fromhex(draw_sha))
    h.update(bytes.fromhex(source_sha))
    h.update(bytes([LENS[lens_role]]))
    hstr(h, gauge_id)
    h.update(bytes([TEMPORAL[temporal]]))
    h.update(bytes([1 if exposure_known else 0, 1 if rolling_known else 0]))
    h.update(struct.pack("<I", frames))
    h.update(struct.pack("<I", evidence))
    return h.hexdigest()

def load(path: str) -> dict:
    return json.loads(Path(path).read_text(encoding="utf-8"))

def validate_pre(pre: dict) -> list[str]:
    e = []
    if pre.get("schema") != "D.RAW/SourcePreAdmissionManifest/0.3":
        e.append("schema")
    if pre.get("status") != "CANDIDATE_NOT_SCIENTIFICALLY_ADMITTED":
        e.append("status")
    src = pre.get("source_evidence") or {}
    if src.get("sealed") is not True or src.get("hash_verified_from_source_bytes") is not True:
        e.append("source")
    if not isinstance(src.get("sha256"), str) or len(src.get("sha256", "")) != 64:
        e.append("source.sha256")
    if not isinstance(src.get("byte_length"), int) or src.get("byte_length", 0) <= 0:
        e.append("source.bytes")

    att = pre.get("capture_attestation") or {}
    inst = pre.get("instrument") or {}
    if att.get("authority") != "USER_ATTESTED_CAPTURE" or att.get("self_captured") is not True:
        e.append("attestation")
    if att.get("lens_role") != inst.get("lens_role"):
        e.append("attestation.lens")
    if att.get("attestation_scope") != "SELF_CAPTURE_AND_LENS_ROLE_ONLY":
        e.append("attestation.scope")
    if att.get("runtime_camera2_result_claimed") is not False:
        e.append("attestation.runtime_claim")
    if inst.get("lens_role_authority") != "USER_ATTESTED_CAPTURE":
        e.append("instrument.lens_authority")
    if inst.get("physical_camera_id_authority") != "PROJECT_CANONICAL_DEVICE_MAP_NOT_RUNTIME_RESULT":
        e.append("instrument.camera_authority")
    if inst.get("runtime_active_physical_camera_id") is not None:
        e.append("instrument.runtime_camera_invented")

    route = pre.get("source_route") or {}
    if route.get("runtime_camera2_route_proven") is not False:
        e.append("route.runtime")
    if not isinstance(route.get("route_id"), str) or not route.get("route_id"):
        e.append("route.id")

    s = pre.get("serialized_storage_domain") or {}
    if s.get("authority") != "SOURCE_BOUND":
        e.append("storage.authority")
    for k in ("container", "source_format", "compression", "photometric", "cfa_pattern", "make", "model"):
        if not isinstance(s.get(k), str) or not s.get(k):
            e.append("storage." + k)
    for k in ("width", "height", "bits_per_sample"):
        if not isinstance(s.get(k), int) or s.get(k, 0) <= 0:
            e.append("storage." + k)

    p = pre.get("serialized_cfa_payload") or {}
    if p.get("authority") != "SOURCE_BOUND" or p.get("exact_serialized_strip_bytes") is not True:
        e.append("payload.authority")
    if p.get("decoded_uint16_hash_matches_serialized_strips") is not True:
        e.append("payload.decode_identity")
    if not isinstance(p.get("sha256"), str) or len(p.get("sha256", "")) != 64:
        e.append("payload.sha")

    for name in ("capture_sample_domain", "readout_domain", "sensor_pixel_mode"):
        b = pre.get(name) or {}
        if b.get("authority") != "UNKNOWN" or b.get("value") is not None:
            e.append(name + ".must_unknown")

    if pre.get("evidence_counts") != {"physical_frame_count": 1, "independent_evidence_count": 1}:
        e.append("counts")
    caps = pre.get("initial_capability_placement") or {}
    if set(caps) != CAP_KEYS:
        e.append("capabilities.complete")
    if caps.get("sampling_geometry") != f"SOURCE_BOUND_{s.get('width')}x{s.get('height')}":
        e.append("capabilities.sampling")
    if caps.get("provenance") != "SOURCE_BOUND":
        e.append("capabilities.provenance")
    for k in CAP_KEYS - {"sampling_geometry", "provenance"}:
        if caps.get(k) != "UNKNOWN":
            e.append("capabilities." + k)

    inv = pre.get("invariants") or {}
    expected = {
        "user_attestation_upgrades_sensor_semantics": False,
        "project_camera_map_is_runtime_result": False,
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
        if inv.get(k) != v:
            e.append("invariant." + k)
    if pre.get("state_sha256") != digest(pre, "state_sha256"):
        e.append("state_sha256:" + digest(pre, "state_sha256"))
    return e

def validate_lineage(pre: dict, host: dict, lin: dict) -> list[str]:
    e = validate_pre(pre)
    if host.get("schema") != "D.RAW/HostScientificRouteResult/0.1" or host.get("status") != "PASS":
        e.append("host.status")
    if host.get("source_reverified") is not True:
        e.append("host.reverify")
    if lin.get("schema") != "D.RAW/ScientificIngressLineageBinding/0.2":
        e.append("lineage.schema")
    if lin.get("lineage_mode") != "DIRECT_PHYSICAL_SOURCE_INGRESS":
        e.append("lineage.mode")
    src = (pre.get("source_evidence") or {}).get("sha256")
    payload = (pre.get("serialized_cfa_payload") or {}).get("sha256")
    po = lin.get("physical_observation") or {}
    ing = lin.get("scientific_ingress") or {}
    pl = lin.get("pipeline_lineage") or {}
    if po.get("physical_observation_id") != "DRAW_PHYSICAL_OBS_" + str(src):
        e.append("physical.id")
    if po.get("parent_source_sha256") != src:
        e.append("physical.source")
    if po.get("pre_admission_manifest_state_sha256") != pre.get("state_sha256"):
        e.append("physical.pre")
    if po.get("physical_frame_count") != 1 or po.get("independent_evidence_count") != 1:
        e.append("physical.counts")
    if ing.get("pipeline_input_sha256") != src or host.get("source_evidence_sha256") != src:
        e.append("ingress.source")
    if ing.get("derived_ingress_sha256") is not None:
        e.append("ingress.derived_forbidden")
    if ing.get("serialized_cfa_payload_sha256") != payload:
        e.append("ingress.payload")
    if ing.get("pipeline_input_is_physical_source") is not True or ing.get("same_physical_observation") is not True:
        e.append("ingress.identity")
    if pl.get("pipeline_input_sha256") != src:
        e.append("pipeline.input")
    if pl.get("pipeline_observation_id") != host.get("observation_id"):
        e.append("pipeline.obs")
    if pl.get("pipeline_observation_id") != "DRAW_OBS_" + str(src):
        e.append("pipeline.obs_derivation")
    if pl.get("scale_gauge_id") != host.get("scale_gauge_id") or pl.get("scale_gauge_id") != "DRAW_SOURCE_LOCAL_GAUGE_" + str(src):
        e.append("pipeline.gauge")
    for k in ("scientific_master_sha256", "authority_field_sha256", "truthnegative_state_sha256", "drawnegative_state_sha256"):
        if pl.get(k) != host.get(k):
            e.append("pipeline." + k)
    for k in ("source_reverified", "common_gauge_admitted", "cross_observation_radiometric_equality_allowed", "cross_observation_radiometric_fusion_allowed"):
        if pl.get(k) != host.get(k):
            e.append("pipeline." + k)
    if host.get("physical_frame_count") != 1 or host.get("independent_evidence_count") != 1:
        e.append("host.counts")
    if host.get("adapter_stored_sample_sensel_semantics_certified") is not False or pl.get("stored_sample_sensel_semantics_certified") is not False:
        e.append("sensel_authority")
    if host.get("adapter_direct_sensor_adc_claim_allowed") is not False or pl.get("direct_sensor_adc_claim_allowed") is not False:
        e.append("adc_authority")
    if host.get("creates_new_evidence") is not False or host.get("scientific_writeback_allowed") is not False:
        e.append("host.authority")
    pol = lin.get("identity_policy") or {}
    for k, v in {
        "physical_observation_identity_source": "IMMUTABLE_PHYSICAL_SOURCE_SHA256",
        "pipeline_identity_role": "COMPUTATIONAL_LINEAGE_ONLY",
        "pipeline_observation_id_is_physical_observation_id": False,
        "pipeline_input_is_physical_source": True,
        "derived_ingress_is_new_observation": False,
        "lens_role_authority": "USER_ATTESTED_CAPTURE",
        "runtime_physical_camera_result_proven": False,
    }.items():
        if pol.get(k) != v:
            e.append("policy." + k)
    inv = lin.get("invariants") or {}
    for k, v in {
        "parent_source_mutated": False, "cfa_payload_mutated": False,
        "physical_frame_count_increment": 0, "independent_evidence_count_increment": 0,
        "creates_new_evidence": False, "scientific_writeback_allowed": False,
        "calibration_transfer_implied": False, "cross_observation_relation_granted": False,
        "cross_observation_fusion_granted": False,
    }.items():
        if inv.get(k) != v:
            e.append("lineage.invariant." + k)
    if lin.get("binding_state_sha256") != digest(lin, "binding_state_sha256"):
        e.append("lineage.state:" + digest(lin, "binding_state_sha256"))
    return e

def validate_record(lin: dict, record: dict) -> list[str]:
    e = []
    if record.get("schema") != "D.RAW/DRAWObservationRecord/0.5":
        e.append("record.schema")
    if record.get("status") != "PHYSICAL_OBSERVATION_BOUND_TO_DIRECT_PIPELINE_LINEAGE":
        e.append("record.status")
    p = record.get("pipeline_record_v0_3") or {}
    validator = Path(__file__).with_name("validate_draw_observation_record_v03.py")
    with tempfile.NamedTemporaryFile("w", suffix=".json", delete=False) as f:
        json.dump(p, f)
        tmp = f.name
    try:
        cp = subprocess.run([sys.executable, str(validator), tmp], capture_output=True, text=True)
        if cp.returncode != 0:
            e.append("pipeline_v03_gate:" + (cp.stdout.strip().replace("\n", "|") or cp.stderr.strip()))
    finally:
        Path(tmp).unlink(missing_ok=True)

    po = lin.get("physical_observation") or {}
    pl = lin.get("pipeline_lineage") or {}
    src = po.get("parent_source_sha256")
    if record.get("physical_observation_id") != po.get("physical_observation_id"):
        e.append("record.physical_id")
    if (record.get("physical_source_evidence") or {}).get("sha256") != src:
        e.append("record.source")
    if (record.get("physical_source_evidence") or {}).get("sealed") is not True:
        e.append("record.source_sealed")
    prov = record.get("physical_provenance") or {}
    if prov.get("scientific_ingress_lineage_binding_state_sha256") != lin.get("binding_state_sha256"):
        e.append("record.lineage")
    if prov.get("pre_admission_manifest_state_sha256") != po.get("pre_admission_manifest_state_sha256"):
        e.append("record.pre")
    if prov.get("serialized_cfa_payload_sha256") != (lin.get("scientific_ingress") or {}).get("serialized_cfa_payload_sha256"):
        e.append("record.payload")
    if prov.get("runtime_active_physical_camera_result_proven") is not False:
        e.append("record.runtime_camera_claim")
    b = record.get("scientific_ingress_lineage") or {}
    if b.get("lineage_mode") != "DIRECT_PHYSICAL_SOURCE_INGRESS":
        e.append("record.lineage_mode")
    if b.get("binding_state_sha256") != lin.get("binding_state_sha256"):
        e.append("record.lineage_state")
    if b.get("pipeline_source_sha256") != src or (p.get("source_evidence") or {}).get("sha256") != src:
        e.append("record.direct_source")
    if b.get("pipeline_observation_id") != p.get("observation_id") or p.get("observation_id") != pl.get("pipeline_observation_id"):
        e.append("record.pipeline_obs")
    if (p.get("drawnegative") or {}).get("state_sha256") != pl.get("drawnegative_state_sha256"):
        e.append("record.drawnegative")
    if (p.get("scientific_master") or {}).get("sha256") != pl.get("scientific_master_sha256"):
        e.append("record.master")

    node = record.get("physical_graph_node") or {}
    tf = p.get("temporal_footprint") or {}
    try:
        expected = graph_sha(
            record["physical_observation_id"], p["drawnegative"]["state_sha256"], src,
            p["procedure"]["lens_role"], p["gauge"]["scale_gauge_id"],
            tf["model"], tf["exposure_interval_known"], tf["rolling_shutter_model_known"], 1, 1
        )
        if node.get("state_sha256") != expected:
            e.append("record.graph_digest:" + expected)
    except Exception as exc:
        e.append("record.graph_compute:" + str(exc))
    if node.get("observation_id") != record.get("physical_observation_id") or node.get("source_evidence_sha256") != src:
        e.append("record.graph_identity")
    if node.get("drawnegative_state_sha256") != (p.get("drawnegative") or {}).get("state_sha256"):
        e.append("record.graph_drawnegative")
    if node.get("scale_gauge_id") != (p.get("gauge") or {}).get("scale_gauge_id"):
        e.append("record.graph_gauge")
    if node.get("state_sha256") == (p.get("graph_node") or {}).get("state_sha256"):
        e.append("record.graph_roles_collapsed")
    if record.get("evidence_counts") != {"physical_frame_count": 1, "independent_evidence_count": 1}:
        e.append("record.counts")
    pol = record.get("identity_policy") or {}
    for k, v in {
        "physical_observation_is_free_world_identity": True,
        "pipeline_observation_id_role": "D_RAW_NEGATIVE_V0_1_COMPUTATIONAL_LINEAGE",
        "pipeline_input_is_physical_source": True,
        "pipeline_identity_may_change_with_future_compatibility_representation": True,
        "physical_identity_changes_with_compatibility_representation": False,
        "derived_ingress_is_new_physical_observation": False,
        "graph_relations_use_physical_observation_id": True,
    }.items():
        if pol.get(k) != v:
            e.append("record.policy." + k)
    inv = record.get("invariants") or {}
    for k, v in {
        "physical_source_mutated": False, "direct_ingress_promotes_source_authority": False,
        "drawnegative_v0_1_mutated": False, "physical_frame_count_increment": 0,
        "independent_evidence_count_increment": 0, "creates_new_evidence": False,
        "scientific_writeback_allowed": False, "calibration_transfer_implied": False,
        "cross_observation_relation_granted": False, "cross_observation_fusion_granted": False,
    }.items():
        if inv.get(k) != v:
            e.append("record.invariant." + k)
    if record.get("record_state_sha256") != digest(record, "record_state_sha256"):
        e.append("record.state:" + digest(record, "record_state_sha256"))
    return e

def validate_envelope(pre: dict, lin: dict, record: dict, env: dict) -> list[str]:
    e = validate_record(lin, record)
    if env.get("schema") != "D.RAW/SourceCapabilityEnvelope/0.3" or env.get("status") != "PHYSICAL_SOURCE_KNOWLEDGE_MAP":
        e.append("env.schema_status")
    if env.get("observation_record_state_sha256") != record.get("record_state_sha256"):
        e.append("env.record")
    if env.get("physical_observation_id") != record.get("physical_observation_id"):
        e.append("env.physical_id")
    src = (record.get("physical_source_evidence") or {}).get("sha256")
    if env.get("physical_source_evidence_sha256") != src:
        e.append("env.source")
    if env.get("physical_graph_node_sha256") != (record.get("physical_graph_node") or {}).get("state_sha256"):
        e.append("env.graph")
    if env.get("scientific_ingress_lineage_binding_state_sha256") != lin.get("binding_state_sha256"):
        e.append("env.lineage")
    p = record.get("pipeline_record_v0_3") or {}
    pl = env.get("pipeline_lineage") or {}
    if pl.get("pipeline_observation_id") != p.get("observation_id") or pl.get("pipeline_source_sha256") != src:
        e.append("env.pipeline")
    if pl.get("drawnegative_state_sha256") != (p.get("drawnegative") or {}).get("state_sha256"):
        e.append("env.drawnegative")
    if pl.get("scientific_master_sha256") != (p.get("scientific_master") or {}).get("sha256"):
        e.append("env.master")

    inst = env.get("instrument") or {}
    pi = pre.get("instrument") or {}
    if inst.get("lens_role") != pi.get("lens_role") or inst.get("lens_role_authority") != pi.get("lens_role_authority"):
        e.append("env.lens")
    if inst.get("project_camera_id") != pi.get("project_physical_camera_id"):
        e.append("env.project_camera")
    if inst.get("camera_id_authority") != pi.get("physical_camera_id_authority"):
        e.append("env.camera_authority")
    if inst.get("runtime_active_physical_camera_id") is not None:
        e.append("env.runtime_camera")

    sd = env.get("source_domains") or {}
    cp = sd.get("capture_provenance") or {}
    if cp.get("authority") != "USER_ATTESTED_CAPTURE" or cp.get("self_captured") is not True or cp.get("runtime_camera2_route_proven") is not False:
        e.append("env.capture_provenance")
    storage = sd.get("serialized_storage_domain") or {}
    ps = pre.get("serialized_storage_domain") or {}
    for k in ("authority", "container", "source_format", "width", "height", "bits_per_sample", "compression", "cfa_pattern"):
        if storage.get(k) != ps.get(k):
            e.append("env.storage." + k)
    if storage.get("serialized_cfa_payload_sha256") != (pre.get("serialized_cfa_payload") or {}).get("sha256"):
        e.append("env.storage.payload")
    ing = sd.get("scientific_ingress") or {}
    if ing.get("mode") != "DIRECT_PHYSICAL_SOURCE_INGRESS" or ing.get("sha256") != src:
        e.append("env.ingress")
    if ing.get("lineage_binding_sha256") != lin.get("binding_state_sha256"):
        e.append("env.ingress_lineage")
    for name in ("capture_sample_domain", "readout_domain", "sensor_pixel_mode"):
        b = sd.get(name) or {}
        if b.get("authority") != "UNKNOWN" or b.get("value") is not None:
            e.append("env." + name)

    caps = env.get("capabilities") or {}
    if set(caps) != CAP_KEYS:
        e.append("env.capabilities.complete")
    dims = p.get("authority_dimensions") or {}
    for k in ("sampling_geometry", "geometry_pose", "radiometry", "colorimetry", "spectral", "optical_support", "noise_uncertainty", "temporal"):
        if (caps.get(k) or {}).get("authority") != (dims.get(k) or {}).get("authority"):
            e.append("env.capability." + k)
    for k in CAP_KEYS:
        c = caps.get(k) or {}
        if c.get("cross_observation_relation_admitted") is not False or c.get("implicit_transfer_allowed") is not False:
            e.append("env.capability_authority." + k)
        if c.get("calibration_binding_sha256") is not None or c.get("validity_domain_sha256") is not None:
            e.append("env.calibration." + k)
    for k in ("spectral", "optical_support", "temporal"):
        c = caps.get(k) or {}
        if c.get("knowledge_origin") != "UNKNOWN" or c.get("identity_sha256") is not None or c.get("lineage_binding_sha256") is not None:
            e.append("env.unknown." + k)
    if (caps.get("sampling_geometry") or {}).get("knowledge_origin") != "PHYSICAL_SOURCE":
        e.append("env.sampling_origin")
    if (caps.get("radiometry") or {}).get("identity_sha256") != (p.get("scientific_master") or {}).get("sha256"):
        e.append("env.radiometry_identity")
    if (caps.get("provenance") or {}).get("identity_sha256") != lin.get("binding_state_sha256"):
        e.append("env.provenance")
    if env.get("calibration_bindings") != []:
        e.append("env.calibrations")
    g = env.get("gauge") or {}
    if g.get("relation") != "SOURCE_LOCAL_ONLY" or g.get("shared_free_world_gauge_id") is not None:
        e.append("env.gauge")
    if g.get("scale_gauge_id") != (p.get("gauge") or {}).get("scale_gauge_id"):
        e.append("env.gauge_id")
    inv = env.get("invariants") or {}
    for k, v in {
        "unknown_is_valid_state": True, "missing_capability_placement_allowed": False,
        "physical_source_identity_replaced_by_pipeline_identity": False,
        "direct_ingress_is_new_observation": False, "lens_identity_upgrades_authority": False,
        "file_format_upgrades_authority": False, "user_attestation_upgrades_sensor_semantics": False,
        "implicit_calibration_transfer_allowed": False, "cross_observation_relation_granted": False,
        "cross_observation_fusion_granted": False, "creates_new_evidence": False,
        "scientific_writeback_allowed": False, "appearance_writeback_allowed": False,
    }.items():
        if inv.get(k) != v:
            e.append("env.invariant." + k)
    if env.get("state_sha256") != digest(env, "state_sha256"):
        e.append("env.state:" + digest(env, "state_sha256"))
    return e

def validate_admission(pre: dict, lin: dict, record: dict, env: dict, pkg: dict) -> list[str]:
    e = validate_envelope(pre, lin, record, env)
    if pkg.get("schema") != "D.RAW/SourceAdmissionPackage/0.3" or pkg.get("status") != "ADMITTED_SOURCE_LOCAL":
        e.append("pkg.schema_status")
    if pkg.get("physical_observation_id") != record.get("physical_observation_id") or pkg.get("physical_observation_id") != env.get("physical_observation_id"):
        e.append("pkg.physical_id")
    src = (record.get("physical_source_evidence") or {}).get("sha256")
    if pkg.get("physical_source_evidence_sha256") != src:
        e.append("pkg.source")
    if (pkg.get("observation_record") or {}).get("record_state_sha256") != record.get("record_state_sha256"):
        e.append("pkg.record")
    if (pkg.get("observation_record") or {}).get("physical_graph_node_sha256") != (record.get("physical_graph_node") or {}).get("state_sha256"):
        e.append("pkg.graph")
    if (pkg.get("capability_envelope") or {}).get("state_sha256") != env.get("state_sha256"):
        e.append("pkg.env")
    if (pkg.get("scientific_ingress_lineage") or {}).get("binding_state_sha256") != lin.get("binding_state_sha256"):
        e.append("pkg.lineage")
    if (pkg.get("scientific_ingress_lineage") or {}).get("lineage_mode") != "DIRECT_PHYSICAL_SOURCE_INGRESS":
        e.append("pkg.lineage_mode")
    p = record.get("pipeline_record_v0_3") or {}
    if (pkg.get("pipeline_lineage") or {}).get("pipeline_source_sha256") != src:
        e.append("pkg.pipeline_source")
    if (pkg.get("pipeline_lineage") or {}).get("pipeline_observation_id") != p.get("observation_id"):
        e.append("pkg.pipeline_obs")
    if (pkg.get("pipeline_lineage") or {}).get("drawnegative_state_sha256") != (p.get("drawnegative") or {}).get("state_sha256"):
        e.append("pkg.drawnegative")
    g = pkg.get("gauge") or {}
    if g.get("relation") != "SOURCE_LOCAL_ONLY" or g.get("shared_free_world_gauge_id") is not None:
        e.append("pkg.gauge")
    if g.get("scale_gauge_id") != (p.get("gauge") or {}).get("scale_gauge_id"):
        e.append("pkg.gauge_id")
    if pkg.get("evidence_counts") != {"physical_frame_count": 1, "independent_evidence_count": 1}:
        e.append("pkg.counts")
    scope = pkg.get("capture_identity_scope") or {}
    pi = pre.get("instrument") or {}
    if scope.get("lens_role") != pi.get("lens_role") or scope.get("lens_role_authority") != "USER_ATTESTED_CAPTURE":
        e.append("pkg.lens")
    if scope.get("project_physical_camera_id") != pi.get("project_physical_camera_id"):
        e.append("pkg.project_camera")
    if scope.get("physical_camera_id_authority") != "PROJECT_CANONICAL_DEVICE_MAP_NOT_RUNTIME_RESULT":
        e.append("pkg.camera_authority")
    if scope.get("runtime_active_physical_camera_result_proven") is not False:
        e.append("pkg.runtime_camera")
    for name in ("capture_sample_domain", "readout_domain", "sensor_pixel_mode"):
        b = (pkg.get("source_domain_status") or {}).get(name) or {}
        if b.get("authority") != "UNKNOWN" or b.get("value") is not None:
            e.append("pkg." + name)
    if pkg.get("calibration_binding_count") != 0 or pkg.get("graph_relations") != [] or pkg.get("fusion_admissions") != []:
        e.append("pkg.cross_observation_authority")
    admission = pkg.get("admission_scope") or {}
    for k, v in {
        "physical_source_admitted": True, "scientific_pipeline_lineage_admitted": True,
        "free_world_physical_node_admitted": True, "lens_role_admitted": True,
        "runtime_camera2_route_admitted": False, "cross_observation_authority_admitted": False,
    }.items():
        if admission.get(k) != v:
            e.append("pkg.scope." + k)
    inv = pkg.get("invariants") or {}
    for k, v in {
        "physical_source_mutated": False, "direct_ingress_is_new_observation": False,
        "unknown_source_domains_resolved_by_admission": False,
        "user_attestation_upgrades_sensor_semantics": False,
        "drawnegative_v0_1_mutated": False, "calibration_transfer_implied": False,
        "cross_observation_relation_granted": False, "cross_observation_fusion_granted": False,
        "creates_new_evidence": False, "scientific_writeback_allowed": False,
        "appearance_writeback_allowed": False,
    }.items():
        if inv.get(k) != v:
            e.append("pkg.invariant." + k)
    if pkg.get("state_sha256") != digest(pkg, "state_sha256"):
        e.append("pkg.state:" + digest(pkg, "state_sha256"))
    return e

def main() -> None:
    if len(sys.argv) < 3:
        raise SystemExit("usage: ... pre PRE | lineage PRE HOST LIN | record LIN REC | envelope PRE LIN REC ENV | admission PRE LIN REC ENV PKG")
    mode = sys.argv[1]
    if mode == "pre" and len(sys.argv) == 3:
        errors = validate_pre(load(sys.argv[2]))
        tag = "DRAW_SOURCE_PRE_ADMISSION_V03"
    elif mode == "lineage" and len(sys.argv) == 5:
        errors = validate_lineage(load(sys.argv[2]), load(sys.argv[3]), load(sys.argv[4]))
        tag = "DRAW_SCIENTIFIC_INGRESS_LINEAGE_V02"
    elif mode == "record" and len(sys.argv) == 4:
        errors = validate_record(load(sys.argv[2]), load(sys.argv[3]))
        tag = "DRAW_OBSERVATION_RECORD_V05"
    elif mode == "envelope" and len(sys.argv) == 6:
        errors = validate_envelope(load(sys.argv[2]), load(sys.argv[3]), load(sys.argv[4]), load(sys.argv[5]))
        tag = "DRAW_SOURCE_CAPABILITY_ENVELOPE_V03"
    elif mode == "admission" and len(sys.argv) == 7:
        errors = validate_admission(load(sys.argv[2]), load(sys.argv[3]), load(sys.argv[4]), load(sys.argv[5]), load(sys.argv[6]))
        tag = "DRAW_SOURCE_ADMISSION_PACKAGE_V03"
    else:
        raise SystemExit(2)
    if errors:
        fail(tag, errors)
    print(tag + "_PASS")
    print("creates_new_evidence=false")
    if mode == "admission":
        pkg = load(sys.argv[6])
        print("status=ADMITTED_SOURCE_LOCAL")
        print("physical_observation_id=" + pkg["physical_observation_id"])
        print("runtime_camera2_route_admitted=false")
        print("graph_relations=0")
        print("fusion_admissions=0")

if __name__ == "__main__":
    main()
