#!/usr/bin/env python3
"""Fail-closed FotoGraaf CalibrationPack -> scene admission verifier v0.1.

This tool answers one narrow question: may a previously validated calibration
claim be bound to this exact sealed scene capture as CALIBRATED_PHYSICAL?

It does not fit a calibration, change source bytes, change the Scientific
Master, or turn calibration frames into additional scene evidence.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import sys
from pathlib import Path
from typing import Any, Dict, Iterable, Mapping, Sequence

try:
    from tools.verify_fotograaf_calibration_pack_v0_1 import (
        ValidationError as PackValidationError,
        load_json,
        validate_pack,
    )
except ModuleNotFoundError:  # direct execution: python3 tools/this_file.py
    from verify_fotograaf_calibration_pack_v0_1 import (  # type: ignore
        ValidationError as PackValidationError,
        load_json,
        validate_pack,
    )

ADMISSION_CONTRACT_SCHEMA = "truthraw.fotograaf-calibration-scene-admission-contract.v0.1"
SCENE_SCHEMA = "truthraw.fotograaf-calibration-scene-request.v0.1"
BINDING_SCHEMA = "truthraw.fotograaf-calibration-binding.v0.1"
HEX64 = re.compile(r"^[0-9a-f]{64}$")


class AdmissionError(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AdmissionError(message)


def nonempty(value: Any, name: str) -> str:
    require(isinstance(value, str) and bool(value.strip()), f"{name} must be a non-empty string")
    return value


def finite_number(value: Any, name: str, *, positive: bool = False) -> float:
    require(isinstance(value, (int, float)) and not isinstance(value, bool), f"{name} must be numeric")
    number = float(value)
    require(math.isfinite(number), f"{name} must be finite")
    if positive:
        require(number > 0.0, f"{name} must be > 0")
    return number


def canonical_sha256(value: Any) -> str:
    encoded = json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


def validate_admission_contract(contract: Mapping[str, Any]) -> None:
    require(contract.get("schema") == ADMISSION_CONTRACT_SCHEMA, "wrong scene-admission contract schema")
    require(contract.get("scientificMasterRouteChange") is False,
            "scene admission may not silently change the Scientific Master route")
    require(contract.get("sceneEvidenceCountsChangedByCalibration") is False,
            "scene admission may not change scene evidence counts")
    exact = contract.get("exactScopeFields")
    require(isinstance(exact, list) and len(exact) >= 10 and len(exact) == len(set(exact)),
            "exactScopeFields must be a unique non-trivial list")
    required_measurement = contract.get("sceneMeasurementRequiredFields")
    require(isinstance(required_measurement, list) and required_measurement,
            "sceneMeasurementRequiredFields missing")
    runtime_domain = contract.get("runtimeValidDomainRequiredFields")
    require(isinstance(runtime_domain, list) and runtime_domain,
            "runtimeValidDomainRequiredFields missing")
    binding_packet = contract.get("bindingPacket")
    require(isinstance(binding_packet, dict), "bindingPacket missing")
    must_bind = binding_packet.get("mustBind")
    require(isinstance(must_bind, list) and "modelSha256" in must_bind and "protocolSha256" in must_bind,
            "bindingPacket must bind exact modelSha256 and protocolSha256")
    worker = contract.get("workerInvariance")
    require(isinstance(worker, dict) and worker.get("workerCountMayChangeBinding") is False and
            worker.get("executionWorkerCountExcludedFromBindingDigest") is True,
            "worker-invariance contract is not fail-closed")
    hdr = contract.get("hdrRule")
    require(isinstance(hdr, dict) and hdr.get("productHdrAppearanceIsCalibrationClaim") is False and
            hdr.get("productHdrMayTriggerIsoGainReinterpretation") is False,
            "HDR appearance must remain downstream of calibration admission")


def validate_scene_request(scene: Mapping[str, Any], contract: Mapping[str, Any]) -> None:
    require(scene.get("schema") == SCENE_SCHEMA, "wrong scene request schema")
    source = scene.get("sourceEvidenceSha256")
    require(isinstance(source, str) and HEX64.fullmatch(source) is not None,
            "sourceEvidenceSha256 must be lowercase 64-hex")
    require(scene.get("physicalFrameCount") == 1, "scene physicalFrameCount must remain 1")
    require(scene.get("independentEvidenceCount") == 1, "scene independentEvidenceCount must remain 1")

    scope = scene.get("scope")
    require(isinstance(scope, dict), "scene.scope missing")
    for field in contract["exactScopeFields"]:
        require(field in scope, f"scene.scope missing {field}")
        if field in ("rawWidth", "rawHeight"):
            require(isinstance(scope[field], int) and not isinstance(scope[field], bool) and scope[field] > 0,
                    f"scene.scope.{field} must be positive integer")
        else:
            nonempty(scope[field], f"scene.scope.{field}")

    measurement = scene.get("measurement")
    require(isinstance(measurement, dict), "scene.measurement missing")
    for field in contract["sceneMeasurementRequiredFields"]:
        require(field in measurement, f"scene.measurement missing {field}")
    finite_number(measurement["exposureTimeSeconds"], "measurement.exposureTimeSeconds", positive=True)
    finite_number(measurement["isoMetadata"], "measurement.isoMetadata", positive=True)
    finite_number(measurement["fNumber"], "measurement.fNumber", positive=True)
    nonempty(measurement["gainReadoutStateId"], "measurement.gainReadoutStateId")
    if "temperatureC" in measurement and measurement["temperatureC"] is not None:
        finite_number(measurement["temperatureC"], "measurement.temperatureC")

    requests = scene.get("requestedClaims")
    require(isinstance(requests, list) and requests, "requestedClaims must be a non-empty list")
    seen = set()
    for index, request in enumerate(requests):
        require(isinstance(request, dict), f"requestedClaims[{index}] must be an object")
        quantity = nonempty(request.get("quantity"), f"requestedClaims[{index}].quantity")
        require(quantity not in seen, f"duplicate requested quantity {quantity}")
        seen.add(quantity)
        require(isinstance(request.get("requireCalibratedPhysical"), bool),
                f"requestedClaims[{index}].requireCalibratedPhysical must be boolean")

    worker_count = scene.get("executionWorkerCount", 1)
    require(isinstance(worker_count, int) and not isinstance(worker_count, bool) and worker_count >= 1,
            "executionWorkerCount must be a positive integer")


def _range_contains(spec: Any, value: float, name: str) -> tuple[bool, str | None]:
    if not isinstance(spec, dict):
        return False, f"VALID_DOMAIN_MISSING_{name}"
    try:
        lo = finite_number(spec.get("minInclusive"), f"{name}.minInclusive")
        hi = finite_number(spec.get("maxInclusive"), f"{name}.maxInclusive")
    except AdmissionError:
        return False, f"VALID_DOMAIN_INVALID_{name}"
    if lo > hi:
        return False, f"VALID_DOMAIN_INVALID_{name}"
    if not (lo <= value <= hi):
        return False, f"OUTSIDE_VALID_DOMAIN_{name}"
    return True, None


def _claim_for_quantity(pack: Mapping[str, Any], quantity: str) -> Mapping[str, Any] | None:
    claims = pack.get("claims")
    if not isinstance(claims, list):
        return None
    for claim in claims:
        if isinstance(claim, dict) and claim.get("quantity") == quantity:
            return claim
    return None


def _scope_mismatches(scene_scope: Mapping[str, Any], pack_scope: Mapping[str, Any], fields: Sequence[str]) -> list[str]:
    mismatches = []
    for field in fields:
        if scene_scope.get(field) != pack_scope.get(field):
            mismatches.append(field)
    return mismatches


def _runtime_domain_check(scene: Mapping[str, Any], pack: Mapping[str, Any], claim: Mapping[str, Any],
                          contract: Mapping[str, Any]) -> tuple[bool, str | None]:
    domain = claim.get("validDomain")
    if not isinstance(domain, dict) or domain.get("scopeBound") is not True:
        return False, "CLAIM_VALID_DOMAIN_NOT_SCOPE_BOUND"
    for field in contract["runtimeValidDomainRequiredFields"]:
        if field not in domain:
            return False, f"CLAIM_VALID_DOMAIN_MISSING_{field}"

    measurement = scene["measurement"]
    gain_states = domain.get("gainReadoutStateIds")
    if not isinstance(gain_states, list) or not gain_states or not all(isinstance(x, str) and x for x in gain_states):
        return False, "CLAIM_VALID_DOMAIN_INVALID_gainReadoutStateIds"
    if measurement["gainReadoutStateId"] not in gain_states:
        return False, "GAIN_READOUT_STATE_OUTSIDE_VALID_DOMAIN"

    checks = (
        ("exposureTimeSeconds", float(measurement["exposureTimeSeconds"])),
        ("isoMetadata", float(measurement["isoMetadata"])),
        ("fNumber", float(measurement["fNumber"])),
    )
    for name, value in checks:
        ok, reason = _range_contains(domain.get(name), value, name)
        if not ok:
            return False, reason

    temp_record = pack.get("temperature")
    temperature_calibrated = isinstance(temp_record, dict) and temp_record.get("calibrated") is True
    if temperature_calibrated:
        if "temperatureC" not in domain:
            return False, "TEMPERATURE_CALIBRATED_PACK_MISSING_CLAIM_TEMPERATURE_DOMAIN"
        if measurement.get("temperatureC") is None:
            return False, "SCENE_TEMPERATURE_REQUIRED_BUT_MISSING"
        ok, reason = _range_contains(domain.get("temperatureC"), float(measurement["temperatureC"]), "temperatureC")
        if not ok:
            return False, reason
    return True, None


def _make_binding(scene: Mapping[str, Any], pack: Mapping[str, Any], claim: Mapping[str, Any]) -> Dict[str, Any]:
    scope_digest = canonical_sha256(pack["scope"])
    domain_digest = canonical_sha256(claim["validDomain"])
    binding_core: Dict[str, Any] = {
        "schema": BINDING_SCHEMA,
        "sourceEvidenceSha256": scene["sourceEvidenceSha256"],
        "packId": pack["packId"],
        "datasetManifestSha256": pack["datasetManifestSha256"],
        "protocolSha256": pack["protocolSha256"],
        "modelSha256": pack["modelSha256"],
        "calibrationScopeSha256": scope_digest,
        "quantity": claim["quantity"],
        "authority": "CALIBRATED_PHYSICAL",
        "modelId": claim["modelId"],
        "uncertaintyModelId": claim["uncertaintyModelId"],
        "validDomainSha256": domain_digest,
        "validationReportSha256": claim["validationReportSha256"],
        "uncertaintyReportSha256": claim["uncertaintyReportSha256"],
        "acceptanceProtocolId": claim["acceptanceProtocolId"],
        "captureState": {
            "captureSampleDomainId": scene["scope"]["captureSampleDomainId"],
            "gainReadoutStateId": scene["measurement"]["gainReadoutStateId"],
            "exposureTimeSeconds": scene["measurement"]["exposureTimeSeconds"],
            "isoMetadata": scene["measurement"]["isoMetadata"],
            "fNumber": scene["measurement"]["fNumber"],
            "temperatureC": scene["measurement"].get("temperatureC"),
        },
        "physicalFrameCount": 1,
        "independentEvidenceCount": 1,
        "changesScientificMasterByItself": False,
        "changesSourceEvidence": False,
        "workerCountAffectsBinding": False,
    }
    if "unit" in claim:
        binding_core["unit"] = claim["unit"]
    binding = dict(binding_core)
    binding["bindingSha256"] = canonical_sha256(binding_core)
    return binding


def admit_scene(scene: Mapping[str, Any], pack: Mapping[str, Any], calibration_contract: Mapping[str, Any],
                admission_contract: Mapping[str, Any]) -> Dict[str, Any]:
    validate_admission_contract(admission_contract)
    validate_scene_request(scene, admission_contract)
    validate_pack(pack, calibration_contract)

    scope_mismatches = _scope_mismatches(
        scene["scope"], pack["scope"], admission_contract["exactScopeFields"]
    )
    results = []
    strict_failure = False

    for request in scene["requestedClaims"]:
        quantity = request["quantity"]
        required = request["requireCalibratedPhysical"]
        decision = "NO_CALIBRATED_PHYSICAL_BINDING"
        reason: str | None = None
        binding = None

        if scope_mismatches:
            reason = "SCOPE_MISMATCH:" + ",".join(scope_mismatches)
        else:
            claim = _claim_for_quantity(pack, quantity)
            if claim is None:
                reason = "REQUESTED_CLAIM_MISSING_FROM_PACK"
            elif claim.get("authority") != "CALIBRATED_PHYSICAL" or claim.get("validated") is not True:
                reason = "REQUESTED_CLAIM_NOT_CALIBRATED_PHYSICAL"
            else:
                ok, domain_reason = _runtime_domain_check(scene, pack, claim, admission_contract)
                if not ok:
                    reason = domain_reason
                else:
                    decision = "CALIBRATED_PHYSICAL_ADMITTED"
                    binding = _make_binding(scene, pack, claim)

        if decision != "CALIBRATED_PHYSICAL_ADMITTED" and required:
            decision = "FAIL_CLOSED_REQUIRED_CALIBRATION_UNAVAILABLE"
            strict_failure = True

        results.append({
            "quantity": quantity,
            "decision": decision,
            "reason": reason,
            "binding": binding,
        })

    return {
        "schema": "truthraw.fotograaf-calibration-scene-admission-result.v0.1",
        "sourceEvidenceSha256": scene["sourceEvidenceSha256"],
        "packId": pack["packId"],
        "strictFailure": strict_failure,
        "scopeMatched": not scope_mismatches,
        "scopeMismatches": scope_mismatches,
        "physicalFrameCount": 1,
        "independentEvidenceCount": 1,
        "scientificMasterRouteChanged": False,
        "executionWorkerCountObserved": scene.get("executionWorkerCount", 1),
        "executionWorkerCountAffectsAuthority": False,
        "results": results,
    }


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--calibration-contract", type=Path, required=True)
    parser.add_argument("--admission-contract", type=Path, required=True)
    parser.add_argument("--pack", type=Path)
    parser.add_argument("--scene", type=Path)
    parser.add_argument("--json-out", type=Path)
    args = parser.parse_args(list(argv) if argv is not None else None)
    try:
        calibration_contract = load_json(args.calibration_contract)
        admission_contract = load_json(args.admission_contract)
        validate_admission_contract(admission_contract)
        result: Dict[str, Any] = {
            "admissionContractValid": True,
            "schema": ADMISSION_CONTRACT_SCHEMA,
        }
        require((args.pack is None) == (args.scene is None), "--pack and --scene must be supplied together")
        exit_code = 0
        if args.pack is not None and args.scene is not None:
            result["admission"] = admit_scene(
                load_json(args.scene), load_json(args.pack), calibration_contract, admission_contract
            )
            if result["admission"]["strictFailure"]:
                exit_code = 3
        rendered = json.dumps(result, indent=2, sort_keys=True) + "\n"
        if args.json_out is not None:
            args.json_out.write_text(rendered, encoding="utf-8")
        print(rendered, end="")
        return exit_code
    except (OSError, json.JSONDecodeError, PackValidationError, AdmissionError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
