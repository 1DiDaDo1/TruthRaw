#!/usr/bin/env python3
"""Build fail-closed FotoGraaf C0/C1/C2 calibration intake bundles v0.1.

The intake builder does not fit calibration models and grants no physical authority.
It revalidates every capture through the C0 identity gate, rehashes the exact
source bytes, checks the acquisition sidecar, partitions captures by exact C0
scope, emits dataset-manifest candidates, and reports Phase-A C1/C2 completeness.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from collections import defaultdict
from pathlib import Path
from typing import Any, Dict, Iterable, Mapping

try:
    import verify_fotograaf_c0_capture_identity_v0_1 as c0
    import verify_fotograaf_calibration_dataset_manifest_v0_1 as manifest_v0
except ModuleNotFoundError:  # imported as tools module from tests
    from tools import verify_fotograaf_c0_capture_identity_v0_1 as c0  # type: ignore
    from tools import verify_fotograaf_calibration_dataset_manifest_v0_1 as manifest_v0  # type: ignore

CONTRACT_SCHEMA = "truthraw.fotograaf-calibration-intake-contract.v0.1"
SESSION_SCHEMA = "truthraw.fotograaf-calibration-intake-session.v0.1"
SNAPSHOT_SCHEMA = "truthraw.fotograaf-calibration-capture-metadata-snapshot.v0.1"
BUNDLE_SCHEMA = "truthraw.fotograaf-calibration-intake-bundle.v0.1"


class IntakeError(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise IntakeError(message)


def load_json(path: Path) -> Dict[str, Any]:
    with path.open("r", encoding="utf-8") as f:
        value = json.load(f)
    require(isinstance(value, dict), f"{path}: top level must be object")
    return value


def canonical_bytes(value: Any) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode("utf-8")


def canonical_sha256(value: Any) -> str:
    return hashlib.sha256(canonical_bytes(value)).hexdigest()


def safe_relative(value: Any, name: str) -> Path:
    require(isinstance(value, str) and value.strip(), f"{name} must be non-empty string")
    p = Path(value)
    require(not p.is_absolute() and ".." not in p.parts and str(p) not in ("", "."),
            f"{name} must be safe relative path")
    return p


def nonempty(value: Any, name: str) -> str:
    require(isinstance(value, str) and value.strip(), f"{name} must be non-empty string")
    return value


def validate_contract(contract: Mapping[str, Any], plan: Mapping[str, Any] | None = None) -> None:
    require(contract.get("schema") == CONTRACT_SCHEMA, "wrong intake contract schema")
    require(contract.get("sessionSchema") == SESSION_SCHEMA, "session schema mismatch")
    require(contract.get("metadataSnapshotSchema") == SNAPSHOT_SCHEMA, "snapshot schema mismatch")
    require(contract.get("bundleSchema") == BUNDLE_SCHEMA, "bundle schema mismatch")
    entries = contract.get("requiredSessionEntryFields")
    require(isinstance(entries, list) and len(entries) >= 8 and len(entries) == len(set(entries)),
            "requiredSessionEntryFields invalid")
    roles = contract.get("allowedModuleRoles")
    require(isinstance(roles, dict) and set(roles) == {"C1_DARK_NOISE", "C2_LINEARITY_GAIN_SATURATION"},
            "allowedModuleRoles must contain C1 and C2")
    completeness = contract.get("phaseACompleteness")
    require(isinstance(completeness, dict), "phaseACompleteness missing")
    require(int(completeness.get("minimumAcquisitionAnchors", 0)) > 0, "minimumAcquisitionAnchors invalid")
    c1 = completeness.get("C1_DARK_NOISE")
    c2 = completeness.get("C2_LINEARITY_GAIN_SATURATION")
    require(isinstance(c1, dict) and isinstance(c2, dict), "C1/C2 completeness missing")
    for key in ("minimumDistinctExposureTimesPerAnchor", "minimumIndependentRepeatsPerAnchorExposure"):
        require(isinstance(c1.get(key), int) and c1[key] > 0, f"C1 {key} invalid")
    for key in ("minimumDistinctSignalLevelsPerAnchor", "minimumIndependentRepeatsPerSignalLevel",
                "minimumHeldOutValidationLevelsPerAnchor", "minimumSaturationBracketLevelsPerAnchor"):
        require(isinstance(c2.get(key), int) and c2[key] > 0, f"C2 {key} invalid")
    rules = contract.get("rules")
    require(isinstance(rules, dict), "rules missing")
    require(rules.get("everyCaptureMustPassC0AgainDuringIntake") is True, "C0 revalidation must be required")
    require(rules.get("isoMagnitudeMaySelectCaptureSampleDomain") is False,
            "ISO magnitude may not select sample domain")
    require(rules.get("sceneEvidenceCountsChangedByCalibration") is False,
            "intake may not change later scene evidence counts")

    if plan is not None:
        require(plan.get("schema") == "truthraw.fotograaf-calibration-acquisition-plan.v0.1",
                "wrong acquisition-plan schema")
        phase = plan.get("phaseA")
        require(isinstance(phase, dict), "plan phaseA missing")
        modules = phase.get("modules")
        require(isinstance(modules, dict), "plan phaseA.modules missing")
        pc1 = modules["C1_DARK_NOISE"]
        pc2 = modules["C2_LINEARITY_GAIN_SATURATION"]
        require(completeness["minimumAcquisitionAnchors"] == pc1["gainReadoutAnchorCount"] == pc2["gainReadoutAnchorCount"],
                "intake anchor count must match Phase-A plan")
        require(c1["minimumDistinctExposureTimesPerAnchor"] == pc1["exposureTimesPerState"],
                "C1 exposure count differs from plan")
        require(c1["minimumIndependentRepeatsPerAnchorExposure"] == pc1["repeatsPerCell"],
                "C1 repeat count differs from plan")
        require(c2["minimumDistinctSignalLevelsPerAnchor"] == pc2["signalLevelsPerState"],
                "C2 signal-level count differs from plan")
        require(c2["minimumIndependentRepeatsPerSignalLevel"] == pc2["repeatsPerLevel"],
                "C2 repeat count differs from plan")
        require(c2["minimumHeldOutValidationLevelsPerAnchor"] == pc2["heldOutValidationLevelsWithinTwelve"],
                "C2 held-out level count differs from plan")
        require(c2["minimumSaturationBracketLevelsPerAnchor"] == pc2["saturationBracketLevelsMinimum"],
                "C2 saturation-bracket count differs from plan")


def validate_snapshot(snapshot: Mapping[str, Any], source_sha: str, scope_sha: str,
                      contract: Mapping[str, Any], module: str) -> Dict[str, Any]:
    require(snapshot.get("schema") == SNAPSHOT_SCHEMA, "wrong capture metadata snapshot schema")
    require(snapshot.get("sourceEvidenceSha256") == source_sha,
            "metadata snapshot sourceEvidenceSha256 does not match C0 source")
    require(snapshot.get("scopeKeySha256") == scope_sha,
            "metadata snapshot scopeKeySha256 does not match C0 scope")
    measurement = snapshot.get("measurement")
    require(isinstance(measurement, dict), "metadata snapshot measurement missing")
    for field in contract["metadataRequiredMeasurementFields"]:
        require(field in measurement, f"metadata snapshot measurement missing {field}")
    require(isinstance(measurement["exposureTimeNs"], int) and measurement["exposureTimeNs"] > 0,
            "exposureTimeNs must be positive integer")
    require(isinstance(measurement["isoMetadata"], int) and measurement["isoMetadata"] > 0,
            "isoMetadata must be positive integer")
    nonempty(measurement["gainReadoutStateId"], "gainReadoutStateId")
    for field in ("blackLevelIdentity", "whiteLevelIdentity", "gainMapOpcodeIdentity", "focusState", "stabilizationState"):
        nonempty(measurement[field], field)
    temp = measurement["temperatureObservation"]
    require(isinstance(temp, dict) and temp.get("status") in ("MEASURED", "UNAVAILABLE"),
            "temperatureObservation invalid")
    if module == "C2_LINEARITY_GAIN_SATURATION":
        for field in contract["c2RequiredMetadataFields"]:
            require(field in measurement, f"C2 metadata snapshot missing {field}")
        nonempty(measurement["controlledSignalLevelId"], "controlledSignalLevelId")
        nonempty(measurement["sourceStabilityReferenceId"], "sourceStabilityReferenceId")
        require(isinstance(measurement["isSaturationBracket"], bool), "isSaturationBracket must be boolean")
    return dict(measurement)


def _completeness(entries: list[Dict[str, Any]], contract: Mapping[str, Any]) -> Dict[str, Any]:
    req = contract["phaseACompleteness"]
    min_anchors = req["minimumAcquisitionAnchors"]
    c1_req = req["C1_DARK_NOISE"]
    c2_req = req["C2_LINEARITY_GAIN_SATURATION"]

    c1 = [x for x in entries if x["module"] == "C1_DARK_NOISE"]
    c2 = [x for x in entries if x["module"] == "C2_LINEARITY_GAIN_SATURATION"]
    findings: list[str] = []

    c1_anchors = sorted({x["acquisitionAnchorId"] for x in c1})
    if len(c1_anchors) < min_anchors:
        findings.append(f"C1_ANCHORS:{len(c1_anchors)}<{min_anchors}")
    for anchor in c1_anchors:
        rows = [x for x in c1 if x["acquisitionAnchorId"] == anchor]
        exposures = sorted({x["measurement"]["exposureTimeNs"] for x in rows})
        if len(exposures) < c1_req["minimumDistinctExposureTimesPerAnchor"]:
            findings.append(f"C1_EXPOSURES:{anchor}:{len(exposures)}<{c1_req['minimumDistinctExposureTimesPerAnchor']}")
        for exposure in exposures:
            n = sum(1 for x in rows if x["measurement"]["exposureTimeNs"] == exposure)
            if n < c1_req["minimumIndependentRepeatsPerAnchorExposure"]:
                findings.append(f"C1_REPEATS:{anchor}:{exposure}:{n}<{c1_req['minimumIndependentRepeatsPerAnchorExposure']}")

    c2_anchors = sorted({x["acquisitionAnchorId"] for x in c2})
    if len(c2_anchors) < min_anchors:
        findings.append(f"C2_ANCHORS:{len(c2_anchors)}<{min_anchors}")
    for anchor in c2_anchors:
        rows = [x for x in c2 if x["acquisitionAnchorId"] == anchor]
        levels = sorted({x["measurement"]["controlledSignalLevelId"] for x in rows})
        if len(levels) < c2_req["minimumDistinctSignalLevelsPerAnchor"]:
            findings.append(f"C2_LEVELS:{anchor}:{len(levels)}<{c2_req['minimumDistinctSignalLevelsPerAnchor']}")
        validation_levels = set()
        saturation_levels = set()
        for level in levels:
            level_rows = [x for x in rows if x["measurement"]["controlledSignalLevelId"] == level]
            if len(level_rows) < c2_req["minimumIndependentRepeatsPerSignalLevel"]:
                findings.append(f"C2_REPEATS:{anchor}:{level}:{len(level_rows)}<{c2_req['minimumIndependentRepeatsPerSignalLevel']}")
            if any(x["split"] == "VALIDATION" for x in level_rows):
                validation_levels.add(level)
            if any(x["measurement"]["isSaturationBracket"] for x in level_rows):
                saturation_levels.add(level)
        if len(validation_levels) < c2_req["minimumHeldOutValidationLevelsPerAnchor"]:
            findings.append(f"C2_VALIDATION_LEVELS:{anchor}:{len(validation_levels)}<{c2_req['minimumHeldOutValidationLevelsPerAnchor']}")
        if len(saturation_levels) < c2_req["minimumSaturationBracketLevelsPerAnchor"]:
            findings.append(f"C2_SATURATION_LEVELS:{anchor}:{len(saturation_levels)}<{c2_req['minimumSaturationBracketLevelsPerAnchor']}")

    return {
        "complete": not findings,
        "c1AcquisitionAnchorCount": len(c1_anchors),
        "c2AcquisitionAnchorCount": len(c2_anchors),
        "findings": findings,
    }


def build_intake(session: Mapping[str, Any], contract: Mapping[str, Any], c0_contract: Mapping[str, Any],
                 manifest_contract: Mapping[str, Any], root: Path, output_dir: Path) -> Dict[str, Any]:
    validate_contract(contract)
    c0.validate_contract(c0_contract)
    manifest_v0.validate_contract(manifest_contract)
    require(session.get("schema") == SESSION_SCHEMA, "wrong intake session schema")
    session_id = nonempty(session.get("sessionId"), "sessionId")
    captures = session.get("captures")
    require(isinstance(captures, list) and captures, "session captures must be non-empty list")
    output_dir.mkdir(parents=True, exist_ok=True)

    allowed = {role: module for module, roles in contract["allowedModuleRoles"].items() for role in roles}
    seen_ids: set[str] = set()
    seen_source_sha: set[str] = set()
    groups: Dict[str, Dict[str, Any]] = {}
    normalized: list[Dict[str, Any]] = []

    for index, entry in enumerate(captures):
        prefix = f"captures[{index}]"
        require(isinstance(entry, dict), f"{prefix} must be object")
        for field in contract["requiredSessionEntryFields"]:
            require(field in entry, f"{prefix}.{field} missing")
        capture_id = nonempty(entry["captureId"], f"{prefix}.captureId")
        require(capture_id not in seen_ids, f"duplicate captureId {capture_id}")
        seen_ids.add(capture_id)
        module = nonempty(entry["module"], f"{prefix}.module")
        role = nonempty(entry["role"], f"{prefix}.role")
        require(role in allowed and allowed[role] == module, f"{prefix}: invalid module/role mapping")
        split = entry["split"]
        require(split in ("FIT", "VALIDATION"), f"{prefix}.split invalid")
        anchor = nonempty(entry["acquisitionAnchorId"], f"{prefix}.acquisitionAnchorId")

        source_rel = safe_relative(entry["sourceFileName"], f"{prefix}.sourceFileName")
        meta_rel = safe_relative(entry["metadataSnapshotFileName"], f"{prefix}.metadataSnapshotFileName")
        c0_rel = safe_relative(entry["c0RecordFileName"], f"{prefix}.c0RecordFileName")
        source_path, meta_path, c0_path = root / source_rel, root / meta_rel, root / c0_rel
        record = load_json(c0_path)
        seal = c0.validate_record(record, c0_contract, source_path, meta_path)
        source_sha = seal["sourceEvidenceSha256"]
        require(source_sha not in seen_source_sha, f"duplicate source SHA-256 across intake session: {capture_id}")
        seen_source_sha.add(source_sha)
        require(record["sourceEvidence"]["fileName"] == str(source_rel), f"{prefix}: C0 source fileName mismatch")
        require(record["metadataSnapshot"]["fileName"] == str(meta_rel), f"{prefix}: C0 metadata fileName mismatch")

        snapshot = load_json(meta_path)
        measurement = validate_snapshot(snapshot, source_sha, seal["scopeKeySha256"], contract, module)
        require(measurement["focusState"] == record["scope"]["focusStateClass"], f"{prefix}: focus state mismatch")
        require(measurement["stabilizationState"] == record["scope"]["stabilizationState"], f"{prefix}: stabilization state mismatch")
        if role == "SATURATION_BRACKET":
            require(measurement.get("isSaturationBracket") is True, f"{prefix}: SATURATION_BRACKET role requires isSaturationBracket=true")

        scope_sha = seal["scopeKeySha256"]
        group = groups.setdefault(scope_sha, {"scope": record["scope"], "captures": []})
        require(group["scope"] == record["scope"], "scope hash collision or inconsistent scope")
        manifest_capture = {
            "captureId": capture_id,
            "module": module,
            "role": role,
            "split": split,
            "fileName": str(source_rel),
            "sha256": source_sha,
            "byteLength": source_path.stat().st_size,
            "scopeKeySha256": scope_sha,
            "metadataSnapshotSha256": seal["metadataSnapshotSha256"],
            "exposureTimeNs": measurement["exposureTimeNs"],
            "isoMetadata": measurement["isoMetadata"],
            "blackLevelIdentity": measurement["blackLevelIdentity"],
            "whiteLevelIdentity": measurement["whiteLevelIdentity"],
            "gainMapOpcodeIdentity": measurement["gainMapOpcodeIdentity"],
            "focusState": measurement["focusState"],
            "stabilizationState": measurement["stabilizationState"],
            "temperatureObservation": measurement["temperatureObservation"],
        }
        group["captures"].append(manifest_capture)
        normalized.append({
            "captureId": capture_id,
            "module": module,
            "role": role,
            "split": split,
            "acquisitionAnchorId": anchor,
            "scopeKeySha256": scope_sha,
            "c0RecordSha256": seal["c0RecordSha256"],
            "sourceEvidenceSha256": source_sha,
            "metadataSnapshotSha256": seal["metadataSnapshotSha256"],
            "metadataSnapshotFileName": str(meta_rel),
            "measurement": measurement,
        })

    refs = session.get("externalReferences", [])
    require(isinstance(refs, list), "externalReferences must be list")
    manifest_records = []
    for scope_sha, group in sorted(groups.items()):
        manifest = {
            "schema": manifest_v0.MANIFEST_SCHEMA,
            "manifestId": f"{session_id}:{scope_sha[:16]}",
            "scope": group["scope"],
            "scopeKeySha256": scope_sha,
            "physicalFrameCountForLaterScene": 1,
            "independentEvidenceCountForLaterScene": 1,
            "captures": sorted(group["captures"], key=lambda x: x["captureId"]),
            "externalReferences": refs,
        }
        manifest["manifestSha256"] = manifest_v0.canonical_manifest_sha256(manifest)
        verified = manifest_v0.validate_manifest(manifest, manifest_contract, root)
        file_name = f"{session_id}_{scope_sha[:16]}_dataset_manifest_v0_1.json"
        (output_dir / file_name).write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        manifest_records.append({
            "scopeKeySha256": scope_sha,
            "captureSampleDomainId": group["scope"]["captureSampleDomainId"],
            "manifestFileName": file_name,
            "datasetManifestSha256": verified["datasetManifestSha256"],
            "captureCount": verified["captureCount"],
        })

    completeness = _completeness(normalized, contract)
    bundle_core = {
        "schema": BUNDLE_SCHEMA,
        "sessionId": session_id,
        "status": "PHASE_A_C1_C2_COMPLETE_CANDIDATE" if completeness["complete"] else "CANDIDATE_INCOMPLETE",
        "scopePartitionCount": len(groups),
        "manifests": manifest_records,
        "captures": sorted(normalized, key=lambda x: x["captureId"]),
        "completeness": completeness,
        "physicalFrameCountForLaterScene": 1,
        "independentEvidenceCountForLaterScene": 1,
        "calibrationAuthorityGrantedByIntake": False,
    }
    bundle = dict(bundle_core)
    bundle["bundleSha256"] = canonical_sha256(bundle_core)
    (output_dir / f"{session_id}_intake_bundle_v0_1.json").write_text(
        json.dumps(bundle, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    return bundle


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--intake-contract", type=Path, required=True)
    parser.add_argument("--c0-contract", type=Path, required=True)
    parser.add_argument("--manifest-contract", type=Path, required=True)
    parser.add_argument("--plan", type=Path)
    parser.add_argument("--session", type=Path)
    parser.add_argument("--root", type=Path)
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--require-complete", action="store_true")
    args = parser.parse_args(list(argv) if argv is not None else None)
    try:
        contract = load_json(args.intake_contract)
        plan = load_json(args.plan) if args.plan is not None else None
        validate_contract(contract, plan)
        c0_contract = load_json(args.c0_contract)
        c0.validate_contract(c0_contract)
        manifest_contract = load_json(args.manifest_contract)
        manifest_v0.validate_contract(manifest_contract)
        result: Dict[str, Any] = {"contractValid": True, "schema": CONTRACT_SCHEMA}
        supplied = [args.session is not None, args.root is not None, args.output_dir is not None]
        require(all(supplied) or not any(supplied), "--session, --root and --output-dir must be supplied together")
        if args.session is not None:
            bundle = build_intake(load_json(args.session), contract, c0_contract, manifest_contract,
                                  args.root.resolve(), args.output_dir.resolve())
            result["bundle"] = bundle
            if args.require_complete and not bundle["completeness"]["complete"]:
                raise IntakeError("Phase-A C1/C2 acquisition is incomplete")
        print(json.dumps(result, indent=2, sort_keys=True))
        return 0
    except (OSError, json.JSONDecodeError, IntakeError, c0.C0Error, manifest_v0.ManifestError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
