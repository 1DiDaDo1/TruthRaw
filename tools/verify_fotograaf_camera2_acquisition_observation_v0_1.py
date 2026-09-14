#!/usr/bin/env python3
"""Validate a source-side FotoGraaf Camera2 acquisition observation v0.1.

The record deliberately stops before captureSampleDomainId/gainReadoutStateId
classification. It proves only acquisition-time Camera2 facts and the exact
post-finalization DNG byte identity. It grants no calibration authority.
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

CONTRACT_SCHEMA = "truthraw.fotograaf-camera2-acquisition-observation-contract.v0.1"
OBSERVATION_SCHEMA = "truthraw.fotograaf-camera2-acquisition-observation.v0.1"
HEX64 = re.compile(r"^[0-9a-f]{64}$")


class ObservationError(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ObservationError(message)


def load_json(path: Path) -> Dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        value = json.load(handle)
    require(isinstance(value, dict), f"{path}: top level must be object")
    return value


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def nonempty(value: Any, name: str) -> str:
    require(isinstance(value, str) and value.strip(), f"{name} must be non-empty string")
    return value


def integer(value: Any, name: str, *, positive: bool = False) -> int:
    require(isinstance(value, int) and not isinstance(value, bool), f"{name} must be integer")
    if positive:
        require(value > 0, f"{name} must be positive")
    return value


def finite_or_null(value: Any, name: str) -> float | None:
    if value is None:
        return None
    require(isinstance(value, (int, float)) and not isinstance(value, bool), f"{name} must be numeric or null")
    out = float(value)
    require(math.isfinite(out), f"{name} must be finite")
    return out


def validate_contract(contract: Mapping[str, Any]) -> None:
    require(contract.get("schema") == CONTRACT_SCHEMA, "wrong Camera2 observation contract schema")
    require(contract.get("observationSchema") == OBSERVATION_SCHEMA, "observation schema mismatch")
    for key in ("requiredTopLevelFields", "requiredCameraFields", "requiredTopologyFields", "requiredRequestFields", "requiredResultFields"):
        value = contract.get(key)
        require(isinstance(value, list) and value and len(value) == len(set(value)), f"{key} invalid")
    statuses = contract.get("physicalCameraObservationStatuses")
    require(isinstance(statuses, list) and set(statuses) == {
        "MEASURED_PHYSICAL_OUTPUT_RESULT",
        "MEASURED_ACTIVE_PHYSICAL_RESULT",
        "MEASURED_DIRECT_CAMERA_RESULT",
        "NOT_AVAILABLE",
    }, "physicalCameraObservationStatuses invalid")
    rules = contract.get("authorityRules")
    require(isinstance(rules, dict), "authorityRules missing")
    for key in (
        "sourceBytesMustBeHashedAfterDngFinalization",
        "camera2LogicalIdIsCaptureTimeEvidence",
        "directNonLogicalCameraResultMayIdentifyOpenedPhysicalEndpoint",
        "physicalIdRequiresCaptureResultEvidence",
    ):
        require(rules.get(key) is True, f"authorityRules.{key} must be true")
    for key in (
        "ordinaryDngMetadataMayRecoverPhysicalCameraId",
        "observationMayContainCaptureSampleDomainId",
        "observationMayContainGainReadoutStateId",
        "isoMayClassifyCaptureSampleDomain",
        "isoMayClassifyGainReadoutState",
        "cameraSystemIdMappingMayBeAssumedFromHistoricalLensLabel",
        "calibrationAuthorityGranted",
    ):
        require(rules.get(key) is False, f"authorityRules.{key} must be false")
    require(rules.get("physicalFrameCount") == 1, "physicalFrameCount must stay 1")
    require(rules.get("independentEvidenceCount") == 1, "independentEvidenceCount must stay 1")


def validate_observation(observation: Mapping[str, Any], contract: Mapping[str, Any], source: Path | None = None) -> Dict[str, Any]:
    validate_contract(contract)
    require(observation.get("schema") == OBSERVATION_SCHEMA, "wrong acquisition observation schema")
    for field in contract["requiredTopLevelFields"]:
        require(field in observation, f"observation missing {field}")
    nonempty(observation.get("captureId"), "captureId")
    nonempty(observation.get("createdAtUtc"), "createdAtUtc")

    source_info = observation["source"]
    require(isinstance(source_info, dict), "source must be object")
    nonempty(source_info.get("displayName"), "source.displayName")
    declared_sha = nonempty(source_info.get("sha256"), "source.sha256")
    require(HEX64.fullmatch(declared_sha) is not None, "source.sha256 must be lowercase 64-hex")
    declared_len = integer(source_info.get("byteLength"), "source.byteLength", positive=True)
    require(source_info.get("hashTiming") == "AFTER_DNG_FINALIZATION", "source.hashTiming must prove post-finalization hashing")
    if source is not None:
        require(source.is_file(), f"source file missing: {source}")
        require(source.stat().st_size == declared_len, "source byteLength mismatch")
        require(sha256_file(source) == declared_sha, "source SHA-256 mismatch")

    device = observation["device"]
    require(isinstance(device, dict), "device must be object")
    for field in ("manufacturer", "model", "buildFingerprint"):
        nonempty(device.get(field), f"device.{field}")

    camera = observation["camera"]
    require(isinstance(camera, dict), "camera must be object")
    for field in contract["requiredCameraFields"]:
        require(field in camera, f"camera missing {field}")
    logical_id = nonempty(camera["logicalCameraId"], "camera.logicalCameraId")
    require(isinstance(camera["logicalMultiCamera"], bool), "camera.logicalMultiCamera must be boolean")
    requested_physical = camera["requestedPhysicalCameraId"]
    require(requested_physical is None or (isinstance(requested_physical, str) and requested_physical),
            "camera.requestedPhysicalCameraId must be string or null")
    advertised = camera["advertisedPhysicalCameraIds"]
    require(isinstance(advertised, list) and all(isinstance(x, str) and x for x in advertised),
            "camera.advertisedPhysicalCameraIds invalid")
    require(len(advertised) == len(set(advertised)), "camera.advertisedPhysicalCameraIds contains duplicates")
    result_camera_id = nonempty(camera["resultCameraId"], "camera.resultCameraId")
    physical = camera["physicalCameraObservation"]
    require(isinstance(physical, dict), "camera.physicalCameraObservation must be object")
    status = physical.get("status")
    require(status in contract["physicalCameraObservationStatuses"], "invalid physicalCameraObservation.status")
    value = physical.get("value")
    evidence_method = physical.get("method")
    if status == "NOT_AVAILABLE":
        require(value is None, "NOT_AVAILABLE physicalCameraObservation must have null value")
        require(evidence_method == "CAMERA2_RESULT_KEY_NOT_AVAILABLE", "NOT_AVAILABLE physical method mismatch")
    elif status == "MEASURED_PHYSICAL_OUTPUT_RESULT":
        nonempty(value, "camera.physicalCameraObservation.value")
        require(requested_physical is not None and value == requested_physical,
                "physical output result must match requested physical id")
        require(value in advertised, "requested physical id must be advertised by logical camera")
        require(evidence_method == "CAMERA2_PHYSICAL_OUTPUT_RESULT", "physical output result method mismatch")
        require(result_camera_id == value, "physical output result cameraId mismatch")
    elif status == "MEASURED_ACTIVE_PHYSICAL_RESULT":
        nonempty(value, "camera.physicalCameraObservation.value")
        require(camera["logicalMultiCamera"] is True, "active physical result requires logical multi-camera")
        require(value in advertised, "active physical id must be advertised by logical camera")
        require(evidence_method == "CAMERA2_ACTIVE_PHYSICAL_RESULT", "active physical result method mismatch")
    else:
        nonempty(value, "camera.physicalCameraObservation.value")
        require(camera["logicalMultiCamera"] is False, "direct camera result cannot stand in for a logical multi-camera physical id")
        require(requested_physical is None, "direct camera result may not also claim a requested physical output")
        require(value == logical_id and result_camera_id == logical_id,
                "direct camera result must equal the opened Camera2 camera id")
        require(evidence_method == "CAMERA2_DIRECT_CAMERA_RESULT", "direct camera result method mismatch")

    topology = observation["topology"]
    require(isinstance(topology, dict), "topology must be object")
    for field in contract["requiredTopologyFields"]:
        require(field in topology, f"topology missing {field}")
    require(topology["format"] == "RAW_SENSOR", "topology.format must be RAW_SENSOR")
    integer(topology["rawWidth"], "topology.rawWidth", positive=True)
    integer(topology["rawHeight"], "topology.rawHeight", positive=True)
    nonempty(topology["cfaPattern"], "topology.cfaPattern")
    require(topology["rawCapability"] is True, "RAW capability required")
    require(topology["directCfaMeasurement"] is True, "direct CFA measurement required")
    require(topology["processedRgbInput"] is False, "processed RGB is forbidden")
    require(topology["multiFrameEvidenceMerged"] is False, "multi-frame merged evidence is forbidden")

    request = observation["request"]
    require(isinstance(request, dict), "request must be object")
    for field in contract["requiredRequestFields"]:
        require(field in request, f"request missing {field}")
    integer(request["sensorSensitivityIso"], "request.sensorSensitivityIso", positive=True)
    integer(request["exposureTimeNs"], "request.exposureTimeNs", positive=True)
    finite_or_null(request["focusDistanceDiopters"], "request.focusDistanceDiopters")
    require(isinstance(request["oisRequestedOff"], bool), "request.oisRequestedOff must be boolean")

    result = observation["result"]
    require(isinstance(result, dict), "result must be object")
    for field in contract["requiredResultFields"]:
        require(field in result, f"result missing {field}")
    integer(result["sensorSensitivityIso"], "result.sensorSensitivityIso", positive=True)
    integer(result["exposureTimeNs"], "result.exposureTimeNs", positive=True)
    sensor_ts = integer(result["sensorTimestampNs"], "result.sensorTimestampNs", positive=True)
    image_ts = integer(result["imageTimestampNs"], "result.imageTimestampNs", positive=True)
    require(result["timestampMatch"] is True and sensor_ts == image_ts,
            "RAW image timestamp must exactly match capture-result sensor timestamp")
    finite_or_null(result["focusDistanceDiopters"], "result.focusDistanceDiopters")
    require(result["oisMode"] is None or isinstance(result["oisMode"], int), "result.oisMode invalid")
    require(result["noiseReductionMode"] is None or isinstance(result["noiseReductionMode"], int),
            "result.noiseReductionMode invalid")

    authority = observation["authority"]
    require(isinstance(authority, dict), "authority must be object")
    require(authority.get("recordClass") == "CAMERA2_ACQUISITION_OBSERVATION_ONLY", "authority.recordClass mismatch")
    require(authority.get("calibrationAuthorityGranted") is False, "Camera2 observation may not grant calibration authority")
    require(authority.get("c0EnvelopeReady") is False, "unclassified Camera2 observation may not claim C0 envelope readiness")
    require(authority.get("physicalFrameCountForLaterScene") == 1, "later scene physicalFrameCount must stay 1")
    require(authority.get("independentEvidenceCountForLaterScene") == 1, "later scene independentEvidenceCount must stay 1")
    missing = authority.get("missingBeforeC0Envelope")
    require(isinstance(missing, list) and "captureSampleDomainId" in missing and "gainReadoutStateId" in missing,
            "authority must explicitly retain sample/gain classification as missing")

    text = json.dumps(observation, sort_keys=True)
    require('"captureSampleDomainId"' not in text, "captureSampleDomainId is forbidden in Camera2 observation")
    require('"gainReadoutStateId"' not in text, "gainReadoutStateId is forbidden in Camera2 observation")

    physical_measured = status != "NOT_AVAILABLE"
    honor = contract.get("honorTeleResearchTarget", {})
    honor_topology_candidate = (
        device.get("manufacturer", "").upper() == str(honor.get("deviceMake", "")).upper()
        and device.get("model") == honor.get("deviceModel")
        and topology["rawWidth"] == honor.get("targetRawWidth")
        and topology["rawHeight"] == honor.get("targetRawHeight")
        and topology["cfaPattern"] == honor.get("targetCfaPattern")
    )
    return {
        "valid": True,
        "decision": "CAMERA2_ACQUISITION_OBSERVATION_VALID",
        "captureId": observation["captureId"],
        "sourceSha256": declared_sha,
        "sourceBytesVerified": source is not None,
        "logicalCameraId": logical_id,
        "physicalCameraIdentityMeasured": physical_measured,
        "physicalCameraObservationStatus": status,
        "honorTeleTopologyCandidate": honor_topology_candidate,
        "c0EnvelopeReady": False,
        "missingBeforeC0Envelope": list(missing),
        "calibrationAuthorityGranted": False,
    }


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--contract", type=Path, required=True)
    parser.add_argument("--observation", type=Path)
    parser.add_argument("--source", type=Path)
    args = parser.parse_args(list(argv) if argv is not None else None)
    try:
        contract = load_json(args.contract)
        validate_contract(contract)
        result: Dict[str, Any] = {"contractValid": True, "schema": CONTRACT_SCHEMA, "status": "CAMERA2_OBSERVATION_GATE_IMPLEMENTED"}
        if args.observation is not None:
            require(args.source is not None, "--source is required with --observation")
            result["observation"] = validate_observation(load_json(args.observation), contract, args.source)
        elif args.source is not None:
            raise ObservationError("--source requires --observation")
        print(json.dumps(result, indent=2, sort_keys=True))
        return 0
    except (OSError, json.JSONDecodeError, ObservationError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
