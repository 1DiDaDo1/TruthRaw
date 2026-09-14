#!/usr/bin/env python3
"""Verify TruthRaw FotoGraaf calibration dataset manifests v0.1.

The verifier validates manifest structure, canonical scope binding, capture-role
semantics, fit/validation separation, unique capture evidence, and optionally
rehashes every referenced local file under --root.

It does not decide whether laboratory conditions were physically correct.
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

CONTRACT_SCHEMA = "truthraw.fotograaf-calibration-dataset-manifest-contract.v0.1"
MANIFEST_SCHEMA = "truthraw.fotograaf-calibration-dataset-manifest.v0.1"
HEX64 = re.compile(r"^[0-9a-f]{64}$")


class ManifestError(RuntimeError):
    pass


def require(cond: bool, message: str) -> None:
    if not cond:
        raise ManifestError(message)


def load_json(path: Path) -> Dict[str, Any]:
    with path.open("r", encoding="utf-8") as f:
        value = json.load(f)
    require(isinstance(value, dict), f"{path}: top level must be object")
    return value


def require_string(value: Any, name: str) -> str:
    require(isinstance(value, str) and value.strip() != "", f"{name} must be non-empty string")
    return value


def require_sha(value: Any, name: str) -> str:
    require(isinstance(value, str) and HEX64.fullmatch(value) is not None,
            f"{name} must be lowercase 64-hex SHA-256")
    return value


def canonical_json_bytes(value: Any) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode("utf-8")


def canonical_scope_sha256(scope: Mapping[str, Any]) -> str:
    return hashlib.sha256(canonical_json_bytes(scope)).hexdigest()


def canonical_manifest_sha256(manifest: Mapping[str, Any]) -> str:
    copy = dict(manifest)
    copy.pop("manifestSha256", None)
    return hashlib.sha256(canonical_json_bytes(copy)).hexdigest()


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def validate_contract(contract: Mapping[str, Any]) -> None:
    require(contract.get("schema") == CONTRACT_SCHEMA, "wrong manifest contract schema")
    require(contract.get("manifestSchema") == MANIFEST_SCHEMA, "contract manifestSchema mismatch")
    require(contract.get("sceneEvidenceCountsChangedByCalibration") is False,
            "calibration manifest may not change later scene evidence counts")
    scope = contract.get("requiredScopeFields")
    require(isinstance(scope, list) and len(scope) >= 10 and len(scope) == len(set(scope)),
            "requiredScopeFields invalid")
    modules = contract.get("captureModules")
    require(isinstance(modules, dict) and modules, "captureModules missing")
    roles = []
    for module, module_roles in modules.items():
        require_string(module, "captureModules module")
        require(isinstance(module_roles, list) and module_roles, f"{module} roles missing")
        for role in module_roles:
            require_string(role, f"{module} role")
            roles.append(role)
    require(len(roles) == len(set(roles)), "capture roles must map to only one module in v0.1")
    require(contract.get("splitValues") == ["FIT", "VALIDATION"],
            "v0.1 splitValues must be FIT, VALIDATION")
    digest = contract.get("canonicalDigest")
    require(isinstance(digest, dict) and digest.get("algorithm") == "SHA-256",
            "canonicalDigest must use SHA-256")


def _validate_temperature(obs: Any, contract: Mapping[str, Any], prefix: str) -> None:
    require(isinstance(obs, dict), f"{prefix}.temperatureObservation must be object")
    status = obs.get("status")
    tcontract = contract["temperatureObservation"]
    require(status in tcontract["statuses"], f"{prefix}.temperatureObservation.status invalid")
    if status == "MEASURED":
        for field in tcontract["measuredRequiredFields"]:
            require(field in obs, f"{prefix}.temperatureObservation.{field} missing")
        require_string(obs.get("sensorId"), f"{prefix}.temperatureObservation.sensorId")
        value = obs.get("valueC")
        require(isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(float(value)),
                f"{prefix}.temperatureObservation.valueC must be finite number")
        if "uncertaintyC" in obs:
            uncertainty = obs["uncertaintyC"]
            require(isinstance(uncertainty, (int, float)) and not isinstance(uncertainty, bool) and
                    math.isfinite(float(uncertainty)) and float(uncertainty) >= 0.0,
                    f"{prefix}.temperatureObservation.uncertaintyC invalid")
    else:
        require_string(obs.get("reason"), f"{prefix}.temperatureObservation.reason")


def _safe_relative_path(name: str, prefix: str) -> Path:
    path = Path(name)
    require(not path.is_absolute(), f"{prefix}.fileName must be relative")
    require(".." not in path.parts, f"{prefix}.fileName may not escape root")
    require(str(path) not in ("", "."), f"{prefix}.fileName invalid")
    return path


def _verify_file(root: Path, file_name: str, expected_sha: str, expected_len: int, prefix: str) -> None:
    relative = _safe_relative_path(file_name, prefix)
    path = root / relative
    require(path.is_file(), f"{prefix}: referenced file missing under root: {file_name}")
    size = path.stat().st_size
    require(size == expected_len, f"{prefix}: byteLength mismatch expected={expected_len} actual={size}")
    actual = sha256_file(path)
    require(actual == expected_sha, f"{prefix}: SHA-256 mismatch expected={expected_sha} actual={actual}")


def validate_manifest(manifest: Mapping[str, Any], contract: Mapping[str, Any], root: Path | None = None) -> Dict[str, Any]:
    validate_contract(contract)
    require(manifest.get("schema") == MANIFEST_SCHEMA, "wrong calibration dataset manifest schema")
    require_string(manifest.get("manifestId"), "manifestId")
    scope = manifest.get("scope")
    require(isinstance(scope, dict), "scope missing")
    for field in contract["requiredScopeFields"]:
        value = scope.get(field)
        if field in ("rawWidth", "rawHeight"):
            require(isinstance(value, int) and not isinstance(value, bool) and value > 0,
                    f"scope.{field} must be positive integer")
        else:
            require_string(value, f"scope.{field}")
    scope_sha = canonical_scope_sha256(scope)
    declared_scope_sha = require_sha(manifest.get("scopeKeySha256"), "scopeKeySha256")
    require(scope_sha == declared_scope_sha,
            f"scopeKeySha256 mismatch expected={scope_sha} declared={declared_scope_sha}")
    require(manifest.get("physicalFrameCountForLaterScene") == 1,
            "calibration manifest may not change later scene physicalFrameCount")
    require(manifest.get("independentEvidenceCountForLaterScene") == 1,
            "calibration manifest may not change later scene independentEvidenceCount")

    captures = manifest.get("captures")
    require(isinstance(captures, list), "captures must be list")
    capture_ids = set()
    capture_hashes = set()
    fit_ids = set()
    validation_ids = set()
    role_map: Dict[str, str] = {}
    for module, roles in contract["captureModules"].items():
        for role in roles:
            role_map[role] = module

    counts: Dict[str, Dict[str, int]] = {}
    for index, capture in enumerate(captures):
        prefix = f"captures[{index}]"
        require(isinstance(capture, dict), f"{prefix} must be object")
        for field in contract["captureRequiredFields"]:
            require(field in capture, f"{prefix}.{field} missing")
        capture_id = require_string(capture["captureId"], f"{prefix}.captureId")
        require(capture_id not in capture_ids, f"duplicate captureId {capture_id}")
        capture_ids.add(capture_id)
        module = require_string(capture["module"], f"{prefix}.module")
        role = require_string(capture["role"], f"{prefix}.role")
        require(role in role_map, f"{prefix}: unsupported role {role}")
        require(role_map[role] == module,
                f"{prefix}: role {role} belongs to {role_map[role]}, not {module}")
        split = capture["split"]
        require(split in contract["splitValues"], f"{prefix}.split invalid")
        if split == "FIT":
            fit_ids.add(capture_id)
        else:
            validation_ids.add(capture_id)
        capture_sha = require_sha(capture["sha256"], f"{prefix}.sha256")
        require(capture_sha not in capture_hashes,
                f"{prefix}: duplicate capture SHA-256; repeated use of same file is not an independent capture")
        capture_hashes.add(capture_sha)
        byte_length = capture["byteLength"]
        require(isinstance(byte_length, int) and not isinstance(byte_length, bool) and byte_length > 0,
                f"{prefix}.byteLength must be positive integer")
        require_sha(capture["scopeKeySha256"], f"{prefix}.scopeKeySha256")
        require(capture["scopeKeySha256"] == scope_sha, f"{prefix}.scopeKeySha256 does not match manifest scope")
        require_sha(capture["metadataSnapshotSha256"], f"{prefix}.metadataSnapshotSha256")
        exposure = capture["exposureTimeNs"]
        require(isinstance(exposure, int) and not isinstance(exposure, bool) and exposure > 0,
                f"{prefix}.exposureTimeNs must be positive integer")
        iso = capture["isoMetadata"]
        require(isinstance(iso, int) and not isinstance(iso, bool) and iso > 0,
                f"{prefix}.isoMetadata must be positive integer")
        require_string(capture["blackLevelIdentity"], f"{prefix}.blackLevelIdentity")
        require_string(capture["whiteLevelIdentity"], f"{prefix}.whiteLevelIdentity")
        require_string(capture["gainMapOpcodeIdentity"], f"{prefix}.gainMapOpcodeIdentity")
        require_string(capture["focusState"], f"{prefix}.focusState")
        require_string(capture["stabilizationState"], f"{prefix}.stabilizationState")
        _validate_temperature(capture["temperatureObservation"], contract, prefix)
        file_name = require_string(capture["fileName"], f"{prefix}.fileName")
        _safe_relative_path(file_name, prefix)
        if root is not None:
            _verify_file(root, file_name, capture_sha, byte_length, prefix)
        counts.setdefault(module, {}).setdefault(role, 0)
        counts[module][role] += 1

    require(fit_ids.isdisjoint(validation_ids), "fit/validation capture ID overlap")

    refs = manifest.get("externalReferences", [])
    require(isinstance(refs, list), "externalReferences must be list")
    ref_ids = set()
    allowed_ref_kinds = set(contract["externalReferenceKinds"])
    for index, ref in enumerate(refs):
        prefix = f"externalReferences[{index}]"
        require(isinstance(ref, dict), f"{prefix} must be object")
        reference_id = require_string(ref.get("referenceId"), f"{prefix}.referenceId")
        require(reference_id not in ref_ids, f"duplicate external reference id {reference_id}")
        ref_ids.add(reference_id)
        kind = ref.get("kind")
        require(kind in allowed_ref_kinds, f"{prefix}.kind invalid")
        file_name = require_string(ref.get("fileName"), f"{prefix}.fileName")
        reference_sha = require_sha(ref.get("sha256"), f"{prefix}.sha256")
        byte_length = ref.get("byteLength")
        require(isinstance(byte_length, int) and not isinstance(byte_length, bool) and byte_length > 0,
                f"{prefix}.byteLength must be positive integer")
        _safe_relative_path(file_name, prefix)
        if root is not None:
            _verify_file(root, file_name, reference_sha, byte_length, prefix)

    computed = canonical_manifest_sha256(manifest)
    declared = manifest.get("manifestSha256")
    if declared is not None:
        require_sha(declared, "manifestSha256")
        require(declared == computed,
                f"manifestSha256 mismatch expected={computed} declared={declared}")

    return {
        "valid": True,
        "schema": MANIFEST_SCHEMA,
        "manifestId": manifest["manifestId"],
        "scopeKeySha256": scope_sha,
        "datasetManifestSha256": computed,
        "captureCount": len(captures),
        "fitCaptureCount": len(fit_ids),
        "validationCaptureCount": len(validation_ids),
        "moduleRoleCounts": counts,
        "externalReferenceCount": len(refs),
        "actualFilesRehashed": root is not None,
        "sceneEvidenceCounts": {"physicalFrameCount": 1, "independentEvidenceCount": 1},
    }


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--contract", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--root", type=Path)
    parser.add_argument("--json-out", type=Path)
    args = parser.parse_args(list(argv) if argv is not None else None)
    try:
        contract = load_json(args.contract)
        manifest = load_json(args.manifest)
        root = args.root.resolve() if args.root is not None else None
        result = validate_manifest(manifest, contract, root)
        text = json.dumps(result, indent=2, sort_keys=True) + "\n"
        if args.json_out is not None:
            args.json_out.write_text(text, encoding="utf-8")
        print(text, end="")
        return 0
    except (OSError, json.JSONDecodeError, ManifestError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
