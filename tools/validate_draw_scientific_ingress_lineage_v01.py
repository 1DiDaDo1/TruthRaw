#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import sys

HEX64 = re.compile(r"^[0-9a-f]{64}$")

def canonical_hash(doc: dict) -> str:
    x = dict(doc)
    x.pop("binding_state_sha256", None)
    return hashlib.sha256(
        json.dumps(
            x,
            sort_keys=True,
            separators=(",", ":"),
            ensure_ascii=False,
            allow_nan=False,
        ).encode("utf-8")
    ).hexdigest()

def fail(errors: list[str]) -> None:
    print("DRAW_SCIENTIFIC_INGRESS_LINEAGE_V01_FAIL")
    for error in errors:
        print(error)
    raise SystemExit(1)

def hex64(value) -> bool:
    return isinstance(value, str) and HEX64.fullmatch(value) is not None

def main() -> None:
    if len(sys.argv) != 5:
        fail([
            "usage: validate_draw_scientific_ingress_lineage_v01.py "
            "PRE_ADMISSION.json QUARANTINE.json HOST_RESULT.json BINDING.json"
        ])

    try:
        pre = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
        quarantine = json.loads(Path(sys.argv[2]).read_text(encoding="utf-8"))
        host = json.loads(Path(sys.argv[3]).read_text(encoding="utf-8"))
        binding = json.loads(Path(sys.argv[4]).read_text(encoding="utf-8"))
    except Exception as exc:
        fail(["json:" + str(exc)])

    errors: list[str] = []

    if binding.get("schema") != "D.RAW/ScientificIngressLineageBinding/0.1":
        errors.append("schema")
    if binding.get("status") != "BOUND_ONE_PHYSICAL_OBSERVATION":
        errors.append("status")

    physical = binding.get("physical_observation") or {}
    ingress = binding.get("ingress_derivation") or {}
    pipeline = binding.get("pipeline_lineage") or {}
    policy = binding.get("identity_policy") or {}
    invariants = binding.get("invariants") or {}

    parent_sha = (pre.get("source_evidence") or {}).get("sha256")
    pre_state = pre.get("state_sha256")
    pre_payload = (pre.get("serialized_cfa_payload") or {}).get("sha256")

    q_parent = (quarantine.get("parent_source") or {}).get("sha256")
    q_derived = (quarantine.get("derived_ingress") or {}).get("sha256")
    q_manifest = quarantine.get("manifest_sha256")
    q_payload = quarantine.get("serialized_cfa_payload") or {}

    host_source = host.get("source_evidence_sha256")

    for name, value in (
        ("parent_sha", parent_sha),
        ("pre_state", pre_state),
        ("pre_payload", pre_payload),
        ("q_parent", q_parent),
        ("q_derived", q_derived),
        ("q_manifest", q_manifest),
        ("host_source", host_source),
        ("binding_state", binding.get("binding_state_sha256")),
    ):
        if not hex64(value):
            errors.append(name + ".sha256")

    if pre.get("schema") != "D.RAW/SourcePreAdmissionManifest/0.2":
        errors.append("pre.schema")
    if pre.get("status") != "CANDIDATE_NOT_SCIENTIFICALLY_ADMITTED":
        errors.append("pre.status")

    if quarantine.get("schema") != "D.RAW/DngOrientationQuarantine/0.1":
        errors.append("quarantine.schema")
    if quarantine.get("status") != "DERIVED_INGRESS_CONTAINER_NO_NEW_EVIDENCE":
        errors.append("quarantine.status")

    if host.get("schema") != "D.RAW/HostScientificRouteResult/0.1":
        errors.append("host.schema")
    if host.get("status") != "PASS":
        errors.append("host.status")
    if host.get("source_reverified") is not True:
        errors.append("host.source_reverified")

    # Parent physical-source binding.
    if q_parent != parent_sha:
        errors.append("parent.pre_to_quarantine")
    if physical.get("parent_source_sha256") != parent_sha:
        errors.append("physical.parent_source")
    expected_physical_id = "DRAW_PHYSICAL_OBS_" + str(parent_sha)
    if physical.get("physical_observation_id") != expected_physical_id:
        errors.append("physical.identity")
    if physical.get("pre_admission_manifest_state_sha256") != pre_state:
        errors.append("physical.pre_admission_state")

    # One physical observation only.
    if physical.get("physical_frame_count") != 1:
        errors.append("physical.frame_count")
    if physical.get("independent_evidence_count") != 1:
        errors.append("physical.evidence_count")

    # Compatibility-ingress binding.
    if ingress.get("schema") != "D.RAW/DngOrientationQuarantine/0.1":
        errors.append("ingress.schema")
    if ingress.get("manifest_sha256") != q_manifest:
        errors.append("ingress.manifest")
    if ingress.get("derived_ingress_sha256") != q_derived:
        errors.append("ingress.derived_sha")
    if ingress.get("derived_container_role") != "SCIENTIFIC_STORAGE_COORDINATE_INGRESS_ONLY":
        errors.append("ingress.role")
    if ingress.get("same_physical_observation") is not True:
        errors.append("ingress.same_physical_observation")
    if ingress.get("cfa_payload_byte_identical_to_parent") is not True:
        errors.append("ingress.cfa_identity_flag")

    if q_payload.get("byte_identical") is not True:
        errors.append("quarantine.cfa_identity")
    if q_payload.get("parent_sha256") != q_payload.get("derived_sha256"):
        errors.append("quarantine.cfa_hash_changed")
    if q_payload.get("parent_sha256") != pre_payload:
        errors.append("quarantine.pre_payload")
    if ingress.get("serialized_cfa_payload_sha256") != pre_payload:
        errors.append("ingress.payload")

    q_lineage = quarantine.get("lineage") or {}
    for key, expected in {
        "same_physical_observation": True,
        "parent_source_mutated": False,
        "cfa_payload_mutated": False,
        "physical_frame_count_increment": 0,
        "independent_evidence_count_increment": 0,
        "creates_new_evidence": False,
        "scientific_writeback_allowed": False,
    }.items():
        if q_lineage.get(key) != expected:
            errors.append("quarantine.lineage." + key)

    # Actual pipeline input must be the derived compatibility container.
    if host_source != q_derived:
        errors.append("pipeline.input_not_derived_ingress")
    if pipeline.get("pipeline_input_sha256") != host_source:
        errors.append("pipeline.input_binding")

    expected_pipeline_obs = "DRAW_OBS_" + str(q_derived)
    expected_gauge = "DRAW_SOURCE_LOCAL_GAUGE_" + str(q_derived)
    if host.get("observation_id") != expected_pipeline_obs:
        errors.append("host.pipeline_observation_id")
    if host.get("scale_gauge_id") != expected_gauge:
        errors.append("host.scale_gauge_id")
    if pipeline.get("pipeline_observation_id") != host.get("observation_id"):
        errors.append("pipeline.observation_id")
    if pipeline.get("scale_gauge_id") != host.get("scale_gauge_id"):
        errors.append("pipeline.gauge")

    for field in (
        "scientific_master_sha256",
        "authority_field_sha256",
        "truthnegative_state_sha256",
        "drawnegative_state_sha256",
    ):
        if not hex64(host.get(field)):
            errors.append("host." + field)
        if pipeline.get(field) != host.get(field):
            errors.append("pipeline." + field)

    for field in (
        "source_reverified",
        "common_gauge_admitted",
        "cross_observation_radiometric_equality_allowed",
        "cross_observation_radiometric_fusion_allowed",
    ):
        if pipeline.get(field) != host.get(field):
            errors.append("pipeline." + field)

    if host.get("common_gauge_admitted") is not False:
        errors.append("host.common_gauge")
    if host.get("cross_observation_radiometric_equality_allowed") is not False:
        errors.append("host.equality")
    if host.get("cross_observation_radiometric_fusion_allowed") is not False:
        errors.append("host.fusion")
    if host.get("physical_frame_count") != 1:
        errors.append("host.frame_count")
    if host.get("independent_evidence_count") != 1:
        errors.append("host.evidence_count")
    if host.get("creates_new_evidence") is not False:
        errors.append("host.creates_new_evidence")
    if host.get("scientific_writeback_allowed") is not False:
        errors.append("host.writeback")

    if pipeline.get("stored_sample_sensel_semantics_certified") != host.get(
        "adapter_stored_sample_sensel_semantics_certified"
    ):
        errors.append("pipeline.sensel_semantics")
    if pipeline.get("direct_sensor_adc_claim_allowed") != host.get(
        "adapter_direct_sensor_adc_claim_allowed"
    ):
        errors.append("pipeline.adc_claim")
    if pipeline.get("stored_sample_sensel_semantics_certified") is not False:
        errors.append("pipeline.sensel_authority_upgrade")
    if pipeline.get("direct_sensor_adc_claim_allowed") is not False:
        errors.append("pipeline.adc_authority_upgrade")

    expected_policy = {
        "physical_observation_identity_source": "IMMUTABLE_PARENT_SOURCE_SHA256",
        "pipeline_identity_role": "COMPUTATIONAL_LINEAGE_ONLY",
        "pipeline_observation_id_is_physical_observation_id": False,
        "derived_ingress_is_new_observation": False,
        "orientation_presentation_authority": "UNKNOWN",
    }
    for key, expected in expected_policy.items():
        if policy.get(key) != expected:
            errors.append("policy." + key)

    if pipeline.get("pipeline_observation_id") == physical.get("physical_observation_id"):
        errors.append("identity_roles_collapsed")

    expected_invariants = {
        "parent_source_mutated": False,
        "cfa_payload_mutated": False,
        "physical_frame_count_increment": 0,
        "independent_evidence_count_increment": 0,
        "creates_new_evidence": False,
        "scientific_writeback_allowed": False,
        "calibration_transfer_implied": False,
        "cross_observation_relation_granted": False,
        "cross_observation_fusion_granted": False,
    }
    for key, expected in expected_invariants.items():
        if invariants.get(key) != expected:
            errors.append("invariant." + key)

    if binding.get("binding_state_sha256") != canonical_hash(binding):
        errors.append("binding_state_sha256:" + canonical_hash(binding))

    if errors:
        fail(errors)

    print("DRAW_SCIENTIFIC_INGRESS_LINEAGE_V01_PASS")
    print("physical_observation_id=" + physical["physical_observation_id"])
    print("pipeline_observation_id=" + pipeline["pipeline_observation_id"])
    print("same_physical_observation=true")
    print("derived_ingress_is_new_observation=false")
    print("creates_new_evidence=false")

if __name__ == "__main__":
    main()
