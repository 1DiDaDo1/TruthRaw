#!/usr/bin/env python3
"""Seal one controlled FotoGraaf calibration capture v0.1.

Input is an acquisition-time evidence envelope produced by a source-side capture
companion or equally authoritative capture harness. The sealer waits until the
RAW/DNG bytes are finalized, hashes those exact bytes, creates the calibration
metadata snapshot and C0 identity record, and then re-validates the resulting
C0 seal against the physical files.

It does not infer physicalCameraId from ordinary DNG/EXIF metadata, does not use
ISO magnitude to choose a sample/gain domain, and grants no calibration authority.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import sys
from pathlib import Path
from typing import Any, Dict, Iterable, Mapping

try:
    import verify_fotograaf_c0_capture_identity_v0_1 as c0
except ModuleNotFoundError:
    from tools import verify_fotograaf_c0_capture_identity_v0_1 as c0  # type: ignore

CONTRACT_SCHEMA = "truthraw.fotograaf-capture-evidence-envelope-contract.v0.1"
ENVELOPE_SCHEMA = "truthraw.fotograaf-capture-evidence-envelope.v0.1"
SNAPSHOT_SCHEMA = "truthraw.fotograaf-calibration-capture-metadata-snapshot.v0.1"
HEX64 = re.compile(r"^[0-9a-f]{64}$")


class SealError(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SealError(message)


def load_json(path: Path) -> Dict[str, Any]:
    with path.open("r", encoding="utf-8") as f:
        value = json.load(f)
    require(isinstance(value, dict), f"{path}: top-level must be object")
    return value


def nonempty(value: Any, name: str) -> str:
    require(isinstance(value, str) and value.strip(), f"{name} must be non-empty string")
    return value


def safe_relative(value: Any, name: str) -> Path:
    text = nonempty(value, name)
    p = Path(text)
    require(not p.is_absolute() and ".." not in p.parts and str(p) not in ("", "."),
            f"{name} must be safe relative path")
    return p


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def canonical_bytes(value: Any) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode("utf-8")


def validate_contract(contract: Mapping[str, Any]) -> None:
    require(contract.get("schema") == CONTRACT_SCHEMA, "wrong capture-envelope contract schema")
    require(contract.get("envelopeSchema") == ENVELOPE_SCHEMA, "envelope schema mismatch")
    require(contract.get("metadataSnapshotSchema") == SNAPSHOT_SCHEMA, "snapshot schema mismatch")
    required = contract.get("requiredEnvelopeFields")
    require(isinstance(required, list) and len(required) >= 7 and len(required) == len(set(required)),
            "requiredEnvelopeFields invalid")
    methods = contract.get("allowedIdentityObservationMethods")
    require(isinstance(methods, dict) and methods, "allowedIdentityObservationMethods missing")
    forbidden = contract.get("forbiddenInferenceMethods")
    require(isinstance(forbidden, list) and forbidden, "forbiddenInferenceMethods missing")
    rules = contract.get("rules")
    require(isinstance(rules, dict), "rules missing")
    require(rules.get("physicalCameraIdMayBeRecoveredFromOrdinaryDngMetadataAfterTheFact") is False,
            "physicalCameraId must not be recoverable from ordinary DNG metadata")
    require(rules.get("captureSampleDomainMayBeSelectedFromIsoMagnitude") is False,
            "sample domain may not be selected from ISO")
    require(rules.get("gainReadoutStateMayBeSelectedFromIsoMagnitude") is False,
            "gain/readout state may not be selected from ISO")
    require(rules.get("sealingGrantsCalibrationAuthority") is False,
            "sealing may not grant calibration authority")
    require(rules.get("sceneEvidenceCountsChangedBySealing") is False,
            "sealing may not change later scene evidence counts")


def _validate_observation(field: str, obs: Any, expected_value: Any,
                          contract: Mapping[str, Any]) -> Dict[str, str]:
    require(isinstance(obs, dict), f"identityObservations.{field} must be object")
    value = nonempty(obs.get("value"), f"identityObservations.{field}.value")
    method = nonempty(obs.get("method"), f"identityObservations.{field}.method")
    evidence_id = nonempty(obs.get("evidenceId"), f"identityObservations.{field}.evidenceId")
    require(method not in set(contract["forbiddenInferenceMethods"]),
            f"identityObservations.{field}: forbidden inference method {method}")
    allowed = contract["allowedIdentityObservationMethods"].get(field)
    require(isinstance(allowed, list) and method in allowed,
            f"identityObservations.{field}: method {method} not admitted")
    require(str(expected_value) == value,
            f"identityObservations.{field}.value does not match scope")
    return {"value": value, "method": method, "evidenceId": evidence_id}


def _finite(value: Any, name: str) -> float:
    require(isinstance(value, (int, float)) and not isinstance(value, bool), f"{name} must be numeric")
    result = float(value)
    require(math.isfinite(result), f"{name} must be finite")
    return result


def validate_envelope(envelope: Mapping[str, Any], contract: Mapping[str, Any],
                      c0_contract: Mapping[str, Any]) -> Dict[str, Any]:
    validate_contract(contract)
    c0.validate_contract(c0_contract)
    require(envelope.get("schema") == ENVELOPE_SCHEMA, "wrong capture evidence envelope schema")
    for field in contract["requiredEnvelopeFields"]:
        require(field in envelope, f"envelope missing {field}")
    capture_id = nonempty(envelope["captureId"], "captureId")
    safe_relative(envelope["sourceFileName"], "sourceFileName")

    scope = envelope["scope"]
    require(isinstance(scope, dict), "scope missing")
    for field in c0_contract["requiredScopeFields"]:
        require(field in scope, f"scope missing {field}")
        if field in ("rawWidth", "rawHeight"):
            require(isinstance(scope[field], int) and not isinstance(scope[field], bool) and scope[field] > 0,
                    f"scope.{field} must be positive integer")
        else:
            nonempty(scope[field], f"scope.{field}")

    observations = envelope["identityObservations"]
    require(isinstance(observations, dict), "identityObservations missing")
    normalized_obs: Dict[str, Dict[str, str]] = {}
    for field in c0_contract["requiredIdentityObservationFields"]:
        require(field in observations, f"identityObservations missing {field}")
        normalized_obs[field] = _validate_observation(field, observations[field], scope[field], contract)

    topology = envelope["topologyEvidence"]
    require(isinstance(topology, dict), "topologyEvidence missing")
    for field in c0_contract["requiredTopologyFields"]:
        require(field in topology, f"topologyEvidence missing {field}")
    require(topology["rawWidth"] == scope["rawWidth"] and topology["rawHeight"] == scope["rawHeight"],
            "topology dimensions mismatch scope")
    require(topology["cfaPattern"] == scope["cfaPattern"], "topology CFA mismatch scope")
    require(topology["sampleRepresentation"] == scope["sampleRepresentation"],
            "topology sampleRepresentation mismatch scope")
    require(topology.get("directCfaMeasurement") is True, "directCfaMeasurement must be true")
    require(topology.get("processedRgbInput") is False, "processedRgbInput must be false")
    require(topology.get("multiFrameEvidenceMerged") is False, "multiFrameEvidenceMerged must be false")
    nonempty(topology.get("parserBackendId"), "topologyEvidence.parserBackendId")

    measurement = envelope["measurement"]
    require(isinstance(measurement, dict), "measurement missing")
    for field in contract["measurementRequiredFields"]:
        require(field in measurement, f"measurement missing {field}")
    require(isinstance(measurement["exposureTimeNs"], int) and measurement["exposureTimeNs"] > 0,
            "measurement.exposureTimeNs must be positive integer")
    require(isinstance(measurement["isoMetadata"], int) and measurement["isoMetadata"] > 0,
            "measurement.isoMetadata must be positive integer")
    gain_obs = measurement["gainReadoutStateObservation"]
    require(isinstance(gain_obs, dict), "gainReadoutStateObservation must be object")
    gain_value = nonempty(gain_obs.get("value"), "gainReadoutStateObservation.value")
    gain_method = nonempty(gain_obs.get("method"), "gainReadoutStateObservation.method")
    nonempty(gain_obs.get("evidenceId"), "gainReadoutStateObservation.evidenceId")
    require(gain_method not in set(contract["forbiddenInferenceMethods"]),
            f"forbidden gain/readout inference method {gain_method}")
    require(gain_method in contract["gainReadoutObservationMethods"],
            f"gain/readout observation method {gain_method} not admitted")
    for field in ("blackLevelIdentity", "whiteLevelIdentity", "gainMapOpcodeIdentity", "focusState", "stabilizationState"):
        nonempty(measurement[field], f"measurement.{field}")
    require(measurement["focusState"] == scope["focusStateClass"], "measurement focusState mismatch scope")
    require(measurement["stabilizationState"] == scope["stabilizationState"],
            "measurement stabilizationState mismatch scope")
    temp = measurement["temperatureObservation"]
    require(isinstance(temp, dict) and temp.get("status") in ("MEASURED", "UNAVAILABLE"),
            "temperatureObservation invalid")
    if temp.get("status") == "MEASURED":
        nonempty(temp.get("sensorId"), "temperatureObservation.sensorId")
        _finite(temp.get("valueC"), "temperatureObservation.valueC")
    else:
        nonempty(temp.get("reason"), "temperatureObservation.reason")

    acquisition = envelope["acquisition"]
    require(isinstance(acquisition, dict), "acquisition missing")
    for field in contract["acquisitionRequiredFields"]:
        require(field in acquisition, f"acquisition missing {field}")
    module = nonempty(acquisition["module"], "acquisition.module")
    role = nonempty(acquisition["role"], "acquisition.role")
    split = acquisition["split"]
    require(split in ("FIT", "VALIDATION"), "acquisition.split invalid")
    nonempty(acquisition["acquisitionAnchorId"], "acquisition.acquisitionAnchorId")
    allowed_roles = contract["allowedModuleRoles"].get(module)
    require(isinstance(allowed_roles, list) and role in allowed_roles, "acquisition module/role invalid")
    if module == "C2_LINEARITY_GAIN_SATURATION":
        for field in contract["c2RequiredMeasurementFields"]:
            require(field in measurement, f"C2 measurement missing {field}")
        nonempty(measurement["controlledSignalLevelId"], "measurement.controlledSignalLevelId")
        nonempty(measurement["sourceStabilityReferenceId"], "measurement.sourceStabilityReferenceId")
        require(isinstance(measurement["isSaturationBracket"], bool),
                "measurement.isSaturationBracket must be boolean")
        if role == "SATURATION_BRACKET":
            require(measurement["isSaturationBracket"] is True,
                    "SATURATION_BRACKET role requires isSaturationBracket=true")

    return {
        "captureId": capture_id,
        "scope": dict(scope),
        "identityObservations": normalized_obs,
        "topologyEvidence": dict(topology),
        "measurement": dict(measurement),
        "gainReadoutStateId": gain_value,
        "acquisition": dict(acquisition),
    }


def seal_capture(envelope: Mapping[str, Any], contract: Mapping[str, Any], c0_contract: Mapping[str, Any],
                 root: Path, output_dir: Path) -> Dict[str, Any]:
    normalized = validate_envelope(envelope, contract, c0_contract)
    source_rel = safe_relative(envelope["sourceFileName"], "sourceFileName")
    source_path = root / source_rel
    require(source_path.is_file(), f"source file missing: {source_rel}")
    source_len = source_path.stat().st_size
    require(source_len > 0, "source file must be non-empty")
    source_sha = sha256_file(source_path)
    scope = normalized["scope"]
    scope_sha = c0.canonical_sha256(scope)
    capture_id = normalized["captureId"]

    measurement = dict(normalized["measurement"])
    measurement.pop("gainReadoutStateObservation", None)
    measurement["gainReadoutStateId"] = normalized["gainReadoutStateId"]
    snapshot = {
        "schema": SNAPSHOT_SCHEMA,
        "captureId": capture_id,
        "sourceEvidenceSha256": source_sha,
        "scopeKeySha256": scope_sha,
        "measurement": measurement,
        "acquisition": normalized["acquisition"],
        "calibrationAuthorityGrantedBySnapshot": False,
    }
    snapshot_bytes = canonical_bytes(snapshot) + b"\n"
    snapshot_sha = sha256_bytes(snapshot_bytes)

    output_dir.mkdir(parents=True, exist_ok=True)
    metadata_rel = Path("metadata") / f"{capture_id}_metadata_snapshot_v0_1.json"
    c0_rel = Path("c0") / f"{capture_id}_c0_record_v0_1.json"
    (output_dir / metadata_rel).parent.mkdir(parents=True, exist_ok=True)
    (output_dir / c0_rel).parent.mkdir(parents=True, exist_ok=True)
    metadata_path = output_dir / metadata_rel
    metadata_path.write_bytes(snapshot_bytes)

    record = {
        "schema": c0.RECORD_SCHEMA,
        "recordId": f"C0_{capture_id}",
        "scope": scope,
        "scopeKeySha256": scope_sha,
        "sourceEvidence": {
            "fileName": str(source_rel),
            "sha256": source_sha,
            "byteLength": source_len,
        },
        "metadataSnapshot": {
            "fileName": str(metadata_rel),
            "sha256": snapshot_sha,
            "byteLength": len(snapshot_bytes),
        },
        "identityObservations": normalized["identityObservations"],
        "topologyEvidence": normalized["topologyEvidence"],
        "physicalFrameCountForLaterScene": 1,
        "independentEvidenceCountForLaterScene": 1,
        "calibrationAuthorityGrantedByC0": False,
    }
    record["c0RecordSha256"] = c0.canonical_record_sha256(record)
    c0_path = output_dir / c0_rel
    c0_path.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    # C0 verifier resolves file names relative to one root. Build a validation root view
    # by requiring source root == output root or by validating source and metadata with
    # explicit absolute paths through the lower-level validator.
    seal = c0.validate_record(record, c0_contract, source_path, metadata_path)
    return {
        "schema": "truthraw.fotograaf-sealed-calibration-capture.v0.1",
        "captureId": capture_id,
        "sourceFileName": str(source_rel),
        "sourceEvidenceSha256": source_sha,
        "sourceByteLength": source_len,
        "metadataSnapshotFileName": str(metadata_rel),
        "metadataSnapshotSha256": snapshot_sha,
        "c0RecordFileName": str(c0_rel),
        "c0RecordSha256": seal["c0RecordSha256"],
        "scopeKeySha256": scope_sha,
        "captureSampleDomainId": scope["captureSampleDomainId"],
        "gainReadoutStateId": normalized["gainReadoutStateId"],
        "acquisition": normalized["acquisition"],
        "physicalFrameCountForLaterScene": 1,
        "independentEvidenceCountForLaterScene": 1,
        "calibrationAuthorityGrantedBySealing": False,
    }


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--contract", type=Path, required=True)
    parser.add_argument("--c0-contract", type=Path, required=True)
    parser.add_argument("--envelope", type=Path)
    parser.add_argument("--root", type=Path)
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--json-out", type=Path)
    args = parser.parse_args(list(argv) if argv is not None else None)
    try:
        contract = load_json(args.contract)
        c0_contract = load_json(args.c0_contract)
        validate_contract(contract)
        c0.validate_contract(c0_contract)
        supplied = [args.envelope is not None, args.root is not None, args.output_dir is not None]
        require(all(supplied) or not any(supplied), "--envelope, --root and --output-dir must be supplied together")
        result: Dict[str, Any] = {"contractValid": True, "schema": CONTRACT_SCHEMA}
        if args.envelope is not None:
            sealed = seal_capture(load_json(args.envelope), contract, c0_contract,
                                  args.root.resolve(), args.output_dir.resolve())
            result["sealedCapture"] = sealed
        text = json.dumps(result, indent=2, sort_keys=True) + "\n"
        if args.json_out is not None:
            args.json_out.write_text(text, encoding="utf-8")
        print(text, end="")
        return 0
    except (OSError, json.JSONDecodeError, SealError, c0.C0Error) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
