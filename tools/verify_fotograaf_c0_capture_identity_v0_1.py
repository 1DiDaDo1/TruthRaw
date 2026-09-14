#!/usr/bin/env python3
"""Fail-closed FotoGraaf C0 capture-identity gate v0.1.

C0 seals one exact calibration scope before any C1-C7 frame may be admitted.
It binds source bytes, a capture metadata sidecar, observed camera identity,
capture/sample domain, firmware, focus/stabilization state and direct-CFA
topology. It does not create calibration authority or alter later scene evidence.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
from typing import Any, Dict, Iterable, Mapping

CONTRACT_SCHEMA = "truthraw.fotograaf-c0-capture-identity-contract.v0.1"
RECORD_SCHEMA = "truthraw.fotograaf-c0-capture-identity-record.v0.1"
HEX64 = re.compile(r"^[0-9a-f]{64}$")


class C0Error(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise C0Error(message)


def load_json(path: Path) -> Dict[str, Any]:
    with path.open("r", encoding="utf-8") as f:
        value = json.load(f)
    require(isinstance(value, dict), f"{path}: top level must be object")
    return value


def canonical_bytes(value: Any) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode("utf-8")


def canonical_sha256(value: Any) -> str:
    return hashlib.sha256(canonical_bytes(value)).hexdigest()


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def require_sha(value: Any, name: str) -> str:
    require(isinstance(value, str) and HEX64.fullmatch(value) is not None,
            f"{name} must be lowercase 64-hex SHA-256")
    return value


def _placeholder(value: str, forbidden: list[str]) -> bool:
    upper = value.strip().upper()
    if not upper:
        return True
    return any(token in upper for token in forbidden)


def require_exact_string(value: Any, name: str, forbidden: list[str]) -> str:
    require(isinstance(value, str), f"{name} must be string")
    require(not _placeholder(value, forbidden), f"{name} contains unresolved/placeholder value")
    return value


def validate_contract(contract: Mapping[str, Any]) -> None:
    require(contract.get("schema") == CONTRACT_SCHEMA, "wrong C0 contract schema")
    require(contract.get("recordSchema") == RECORD_SCHEMA, "wrong C0 record schema")
    scope = contract.get("requiredScopeFields")
    require(isinstance(scope, list) and len(scope) >= 16 and len(scope) == len(set(scope)),
            "requiredScopeFields must be unique and complete")
    obs = contract.get("requiredIdentityObservationFields")
    require(isinstance(obs, list) and len(obs) >= 7 and len(obs) == len(set(obs)),
            "requiredIdentityObservationFields invalid")
    topology = contract.get("requiredTopologyFields")
    require(isinstance(topology, list) and len(topology) >= 8 and len(topology) == len(set(topology)),
            "requiredTopologyFields invalid")
    forbidden = contract.get("forbiddenPlaceholderTokens")
    require(isinstance(forbidden, list) and forbidden and all(isinstance(x, str) and x for x in forbidden),
            "forbiddenPlaceholderTokens invalid")
    profile = contract.get("expectedHonorTeleProfile")
    require(isinstance(profile, dict), "expectedHonorTeleProfile missing")
    require(profile.get("deviceMake") == "HONOR" and profile.get("deviceModel") == "BKQ-N49",
            "Honor tele profile device mismatch")
    require(profile.get("cameraSystemId") == "5" and profile.get("lensRole") == "TELE",
            "Honor tele profile camera/lens mismatch")
    require(profile.get("rawWidth") == 4080 and profile.get("rawHeight") == 3072 and
            profile.get("cfaPattern") == "BGGR", "Honor tele profile topology mismatch")
    rules = contract.get("rules")
    require(isinstance(rules, dict), "rules missing")
    for key in (
        "sourceBytesMustBeRehashedForSeal",
        "metadataSnapshotMustBeRehashedForSeal",
        "cameraSystemIdMustBeObservedAtAcquisition",
        "physicalCameraIdMustBeObservedAtAcquisition",
        "directCfaMeasurementRequired",
    ):
        require(rules.get(key) is True, f"rule {key} must be true")
    require(rules.get("ordinaryDngMetadataAloneMayProvePhysicalCameraId") is False,
            "DNG metadata alone may not prove physical camera identity")
    require(rules.get("isoMagnitudeMaySelectCaptureSampleDomain") is False,
            "ISO magnitude may not select capture sample domain")
    require(rules.get("sceneEvidenceCountsChangedByCalibration") is False,
            "C0 may not change later scene evidence counts")


def _validate_file_binding(binding: Any, name: str, actual_path: Path, forbidden: list[str]) -> Dict[str, Any]:
    require(isinstance(binding, dict), f"{name} must be object")
    file_name = require_exact_string(binding.get("fileName"), f"{name}.fileName", forbidden)
    expected_sha = require_sha(binding.get("sha256"), f"{name}.sha256")
    expected_len = binding.get("byteLength")
    require(isinstance(expected_len, int) and not isinstance(expected_len, bool) and expected_len > 0,
            f"{name}.byteLength must be positive integer")
    require(actual_path.is_file(), f"{name}: file missing: {actual_path}")
    actual_len = actual_path.stat().st_size
    require(actual_len == expected_len,
            f"{name}.byteLength mismatch expected={expected_len} actual={actual_len}")
    actual_sha = sha256_file(actual_path)
    require(actual_sha == expected_sha,
            f"{name}.sha256 mismatch expected={expected_sha} actual={actual_sha}")
    return {"fileName": file_name, "sha256": actual_sha, "byteLength": actual_len}


def _validate_scope(scope: Any, contract: Mapping[str, Any]) -> Dict[str, Any]:
    require(isinstance(scope, dict), "scope missing")
    forbidden = contract["forbiddenPlaceholderTokens"]
    for field in contract["requiredScopeFields"]:
        require(field in scope, f"scope.{field} missing")
        if field in ("rawWidth", "rawHeight"):
            require(isinstance(scope[field], int) and not isinstance(scope[field], bool) and scope[field] > 0,
                    f"scope.{field} must be positive integer")
        else:
            require_exact_string(scope[field], f"scope.{field}", forbidden)

    profile = contract["expectedHonorTeleProfile"]
    for field, expected in profile.items():
        require(scope.get(field) == expected,
                f"scope.{field} expected Honor tele value {expected!r}, got {scope.get(field)!r}")
    return dict(scope)


def _validate_observations(observations: Any, scope: Mapping[str, Any], contract: Mapping[str, Any]) -> None:
    require(isinstance(observations, dict), "identityObservations missing")
    forbidden = contract["forbiddenPlaceholderTokens"]
    for field in contract["requiredIdentityObservationFields"]:
        obs = observations.get(field)
        require(isinstance(obs, dict), f"identityObservations.{field} missing")
        value = require_exact_string(obs.get("value"), f"identityObservations.{field}.value", forbidden)
        method = require_exact_string(obs.get("method"), f"identityObservations.{field}.method", forbidden)
        require_exact_string(obs.get("evidenceId"), f"identityObservations.{field}.evidenceId", forbidden)
        require(value == str(scope[field]),
                f"identityObservations.{field}.value does not match scope")
        if field == "cameraSystemId":
            require(method != "INFERRED_FROM_FILENAME" and method != "INFERRED_FROM_ISO",
                    "cameraSystemId must be observed from acquisition identity, not inferred")
        if field == "physicalCameraId":
            require(method not in ("DNG_METADATA_ONLY", "INFERRED_FROM_FOCAL_LENGTH"),
                    "physicalCameraId must be capture-bound, not inferred from DNG/focal length")
        if field == "captureSampleDomainId":
            require(method != "INFERRED_FROM_ISO",
                    "captureSampleDomainId may not be inferred from ISO magnitude")


def _validate_topology(topology: Any, scope: Mapping[str, Any], contract: Mapping[str, Any]) -> None:
    require(isinstance(topology, dict), "topologyEvidence missing")
    forbidden = contract["forbiddenPlaceholderTokens"]
    for field in contract["requiredTopologyFields"]:
        require(field in topology, f"topologyEvidence.{field} missing")
    for field in ("rawWidth", "rawHeight"):
        require(isinstance(topology[field], int) and not isinstance(topology[field], bool) and topology[field] > 0,
                f"topologyEvidence.{field} must be positive integer")
        require(topology[field] == scope[field], f"topologyEvidence.{field} does not match scope")
    for field in ("cfaPattern", "sampleRepresentation"):
        require_exact_string(topology[field], f"topologyEvidence.{field}", forbidden)
        require(topology[field] == scope[field], f"topologyEvidence.{field} does not match scope")
    require_exact_string(topology["parserBackendId"], "topologyEvidence.parserBackendId", forbidden)
    require(topology["directCfaMeasurement"] is True, "topologyEvidence.directCfaMeasurement must be true")
    require(topology["processedRgbInput"] is False, "processed RGB input is forbidden for C0")
    require(topology["multiFrameEvidenceMerged"] is False, "multi-frame merged input is forbidden for C0")


def canonical_record_sha256(record: Mapping[str, Any]) -> str:
    copy = dict(record)
    copy.pop("c0RecordSha256", None)
    return canonical_sha256(copy)


def validate_record(record: Mapping[str, Any], contract: Mapping[str, Any],
                    source_path: Path, metadata_path: Path) -> Dict[str, Any]:
    validate_contract(contract)
    require(record.get("schema") == RECORD_SCHEMA, "wrong C0 record schema")
    forbidden = contract["forbiddenPlaceholderTokens"]
    require_exact_string(record.get("recordId"), "recordId", forbidden)

    scope = _validate_scope(record.get("scope"), contract)
    scope_sha = canonical_sha256(scope)
    declared_scope = require_sha(record.get("scopeKeySha256"), "scopeKeySha256")
    require(declared_scope == scope_sha,
            f"scopeKeySha256 mismatch expected={scope_sha} declared={declared_scope}")

    source = _validate_file_binding(record.get("sourceEvidence"), "sourceEvidence", source_path, forbidden)
    metadata = _validate_file_binding(record.get("metadataSnapshot"), "metadataSnapshot", metadata_path, forbidden)
    _validate_observations(record.get("identityObservations"), scope, contract)
    _validate_topology(record.get("topologyEvidence"), scope, contract)

    require(record.get("physicalFrameCountForLaterScene") == 1,
            "C0 may not change later scene physicalFrameCount")
    require(record.get("independentEvidenceCountForLaterScene") == 1,
            "C0 may not change later scene independentEvidenceCount")
    require(record.get("calibrationAuthorityGrantedByC0") is False,
            "C0 identity seal must not itself grant calibrated physical authority")

    computed_record = canonical_record_sha256(record)
    declared_record = record.get("c0RecordSha256")
    if declared_record is not None:
        require_sha(declared_record, "c0RecordSha256")
        require(declared_record == computed_record,
                f"c0RecordSha256 mismatch expected={computed_record} declared={declared_record}")

    return {
        "valid": True,
        "decision": "C0_IDENTITY_SEALED",
        "recordId": record["recordId"],
        "scopeKeySha256": scope_sha,
        "c0RecordSha256": computed_record,
        "sourceEvidenceSha256": source["sha256"],
        "metadataSnapshotSha256": metadata["sha256"],
        "honorTeleProfileMatched": True,
        "directCfaMeasurement": True,
        "physicalFrameCountForLaterScene": 1,
        "independentEvidenceCountForLaterScene": 1,
        "calibrationAuthorityGrantedByC0": False,
        "nextEligibleModule": "C1_DARK_NOISE_OR_C2_LINEARITY_GAIN_SATURATION",
    }


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--contract", type=Path, required=True)
    parser.add_argument("--record", type=Path)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--metadata-snapshot", type=Path)
    parser.add_argument("--json-out", type=Path)
    args = parser.parse_args(list(argv) if argv is not None else None)
    try:
        contract = load_json(args.contract)
        validate_contract(contract)
        result: Dict[str, Any] = {
            "contractValid": True,
            "schema": CONTRACT_SCHEMA,
            "status": "C0_GATE_IMPLEMENTED_NOT_PHYSICALLY_ACQUIRED",
        }
        provided = [args.record is not None, args.source is not None, args.metadata_snapshot is not None]
        require(all(provided) or not any(provided),
                "--record, --source and --metadata-snapshot must be supplied together")
        if args.record is not None:
            result["seal"] = validate_record(
                load_json(args.record), contract, args.source, args.metadata_snapshot
            )
            result["status"] = "C0_IDENTITY_SEALED"
        text = json.dumps(result, indent=2, sort_keys=True) + "\n"
        if args.json_out is not None:
            args.json_out.write_text(text, encoding="utf-8")
        print(text, end="")
        return 0
    except (OSError, json.JSONDecodeError, C0Error) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
