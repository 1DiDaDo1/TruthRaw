#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

def canonical_sha256(doc: dict) -> str:
    return hashlib.sha256(
        json.dumps(
            doc,
            sort_keys=True,
            separators=(",", ":"),
            ensure_ascii=False,
            allow_nan=False,
        ).encode("utf-8")
    ).hexdigest()

def package_sha256(doc: dict) -> str:
    x = dict(doc)
    x.pop("state_sha256", None)
    return canonical_sha256(x)

def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("--observation", required=True)
    p.add_argument("--envelope", required=True)
    p.add_argument("--output", required=True)
    a = p.parse_args()

    observation = json.loads(Path(a.observation).read_text(encoding="utf-8"))
    envelope = json.loads(Path(a.envelope).read_text(encoding="utf-8"))

    if observation.get("schema") != "D.RAW/DRAWObservationRecord/0.3":
        raise SystemExit("observation must be DRAWObservationRecord v0.3")
    if envelope.get("schema") != "D.RAW/SourceCapabilityEnvelope/0.1":
        raise SystemExit("envelope must be SourceCapabilityEnvelope v0.1")

    oid = observation.get("observation_id")
    source = (observation.get("source_evidence") or {}).get("sha256")
    if envelope.get("observation_id") != oid:
        raise SystemExit("observation/envelope observation_id mismatch")
    if envelope.get("source_evidence_sha256") != source:
        raise SystemExit("observation/envelope source hash mismatch")

    gauge = observation.get("gauge") or {}
    if gauge.get("gauge_relation") != "SOURCE_LOCAL_ONLY":
        raise SystemExit("initial admission requires SOURCE_LOCAL_ONLY gauge")
    if gauge.get("shared_free_world_gauge_id") is not None:
        raise SystemExit("initial admission cannot carry a shared Free World gauge")

    counts = observation.get("evidence_counts") or {}
    if counts != {"physical_frame_count": 1, "independent_evidence_count": 1}:
        raise SystemExit("initial admission requires one physical frame and one evidence item")

    package = {
        "schema": "D.RAW/SourceAdmissionPackage/0.1",
        "status": "ADMITTED_SOURCE_LOCAL",
        "observation_id": oid,
        "source_evidence_sha256": source,
        "observation_record": {
            "schema": "D.RAW/DRAWObservationRecord/0.3",
            "canonical_sha256": canonical_sha256(observation),
            "graph_node_sha256": (observation.get("graph_node") or {}).get("state_sha256"),
        },
        "capability_envelope": {
            "schema": "D.RAW/SourceCapabilityEnvelope/0.1",
            "state_sha256": envelope.get("state_sha256"),
        },
        "gauge": {
            "relation": "SOURCE_LOCAL_ONLY",
            "scale_gauge_id": gauge.get("scale_gauge_id"),
            "shared_free_world_gauge_id": None,
        },
        "evidence_counts": counts,
        "calibration_binding_count": len(observation.get("calibration_bindings") or []),
        "graph_relations": [],
        "fusion_admissions": [],
        "invariants": {
            "source_evidence_mutated": False,
            "scientific_master_mutated_by_admission": False,
            "drawnegative_mutated_by_admission": False,
            "calibration_transfer_implied": False,
            "cross_observation_relation_granted": False,
            "cross_observation_fusion_granted": False,
            "appearance_writeback_allowed": False,
        },
    }
    package["state_sha256"] = package_sha256(package)
    Path(a.output).write_text(json.dumps(package, indent=2) + "\n", encoding="utf-8")

    print("DRAW_SOURCE_ADMISSION_PACKAGE_V01_CREATED")
    print("status=ADMITTED_SOURCE_LOCAL")
    print("observation_id=" + str(oid))
    print("state_sha256=" + package["state_sha256"])

if __name__ == "__main__":
    main()
