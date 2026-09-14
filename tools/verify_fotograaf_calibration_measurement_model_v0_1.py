#!/usr/bin/env python3
"""Validate and digest FotoGraaf Calibration Measurement Model v0.1 artifacts."""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import sys
from pathlib import Path
from typing import Any, Dict, Iterable, Mapping

CONTRACT_SCHEMA = "truthraw.fotograaf-calibration-measurement-model-contract.v0.1"
MODEL_SCHEMA = "truthraw.fotograaf-calibration-measurement-model.v0.1"
HEX64 = re.compile(r"^[0-9a-f]{64}$")
ALLOWED_BLOCKS = {"black", "noise", "response", "saturation"}


class ModelValidationError(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ModelValidationError(message)


def load_json(path: Path) -> Dict[str, Any]:
    with path.open("r", encoding="utf-8") as f:
        value = json.load(f)
    require(isinstance(value, dict), f"{path}: top level must be an object")
    return value


def nonempty(value: Any, name: str) -> str:
    require(isinstance(value, str) and bool(value.strip()), f"{name} must be a non-empty string")
    return value


def hex64(value: Any, name: str) -> str:
    require(isinstance(value, str) and HEX64.fullmatch(value) is not None,
            f"{name} must be lowercase 64-hex SHA-256")
    return value


def finite(value: Any, name: str, *, positive: bool = False) -> float:
    require(isinstance(value, (int, float)) and not isinstance(value, bool), f"{name} must be numeric")
    x = float(value)
    require(math.isfinite(x), f"{name} must be finite")
    if positive:
        require(x > 0.0, f"{name} must be > 0")
    return x


def canonical_bytes(value: Mapping[str, Any]) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode("utf-8")


def canonical_sha256(value: Mapping[str, Any]) -> str:
    return hashlib.sha256(canonical_bytes(value)).hexdigest()


def validate_contract(contract: Mapping[str, Any]) -> None:
    require(contract.get("schema") == CONTRACT_SCHEMA, "wrong measurement-model contract schema")
    require(contract.get("artifactSchema") == MODEL_SCHEMA, "wrong artifact schema in contract")
    require(contract.get("scientificMasterRouteChange") is False,
            "measurement-model contract may not silently change Scientific Master route")
    require(contract.get("selfHashFieldAllowed") is False, "model artifact must not self-hash")
    coordinate = contract.get("coordinateContract")
    require(isinstance(coordinate, dict), "coordinateContract missing")
    require(coordinate.get("noiseCoordinate") == "NORMALIZED_PRE_EXISTING_GAINMAP",
            "noise coordinate must remain pre-existing-GainMap normalized")
    require(coordinate.get("perChannelResponseScaleAllowed") is False,
            "v0.1 must prohibit per-channel response scaling")
    require(coordinate.get("negativePostBlackAllowed") is True,
            "signed post-black values must remain allowed")


def _validate_coordinate(model: Mapping[str, Any], contract: Mapping[str, Any]) -> None:
    coordinate = model.get("coordinateContract")
    require(isinstance(coordinate, dict), "coordinateContract missing")
    expected = contract["coordinateContract"]
    for key in (
        "rawCodeUnit", "blackOffsetUnit", "saturationUnit", "noiseCoordinate",
        "responseScale", "existingSourceGainMapApplication", "perChannelResponseScaleAllowed",
        "negativePostBlackAllowed",
    ):
        require(coordinate.get(key) == expected.get(key), f"coordinateContract.{key} mismatch")


def validate_model(model: Mapping[str, Any], contract: Mapping[str, Any]) -> Dict[str, Any]:
    validate_contract(contract)
    require(model.get("schema") == MODEL_SCHEMA, "wrong measurement-model artifact schema")
    require("modelSha256" not in model, "model artifact must not contain modelSha256 self-hash")
    for field in contract["requiredCommonFields"]:
        require(field in model, f"model missing required field {field}")

    nonempty(model.get("modelId"), "modelId")
    hex64(model.get("protocolSha256"), "protocolSha256")
    hex64(model.get("datasetManifestSha256"), "datasetManifestSha256")
    hex64(model.get("calibrationScopeSha256"), "calibrationScopeSha256")
    nonempty(model.get("claimQuantity"), "claimQuantity")
    nonempty(model.get("captureSampleDomainId"), "captureSampleDomainId")
    _validate_coordinate(model, contract)

    require(model.get("requestsAdditionalGainMapCorrection") is False,
            "additional GainMap/lens-shading correction is forbidden")
    dark_snr = finite(model.get("darkSnrThreshold"), "darkSnrThreshold", positive=True)

    parameters = model.get("parameters")
    require(isinstance(parameters, dict), "parameters must be an object")
    unknown = set(parameters) - ALLOWED_BLOCKS
    require(not unknown, f"unknown parameter blocks: {sorted(unknown)}")
    enabled_count = 0

    if "black" in parameters:
        block = parameters["black"]
        require(isinstance(block, dict), "parameters.black must be an object")
        require(block.get("enabled") is True, "parameters.black.enabled must be true when block is present")
        require(block.get("module") == "C1_DARK_NOISE", "black block must bind C1_DARK_NOISE")
        values = block.get("phaseCodeOffsets")
        require(isinstance(values, list) and len(values) == 4, "black.phaseCodeOffsets must contain four values")
        for i, value in enumerate(values):
            finite(value, f"black.phaseCodeOffsets[{i}]")
        enabled_count += 1

    if "noise" in parameters:
        block = parameters["noise"]
        require(isinstance(block, dict), "parameters.noise must be an object")
        require(block.get("enabled") is True, "parameters.noise.enabled must be true when block is present")
        require(block.get("module") == "C1_DARK_NOISE", "noise block must bind C1_DARK_NOISE")
        values = block.get("rgbAffineSO")
        require(isinstance(values, list) and len(values) == 6, "noise.rgbAffineSO must contain six values")
        for i, value in enumerate(values):
            require(finite(value, f"noise.rgbAffineSO[{i}]") >= 0.0,
                    f"noise.rgbAffineSO[{i}] must be >= 0")
        enabled_count += 1

    if "response" in parameters:
        block = parameters["response"]
        require(isinstance(block, dict), "parameters.response must be an object")
        require(block.get("enabled") is True, "parameters.response.enabled must be true when block is present")
        modules = block.get("modules")
        require(isinstance(modules, list) and modules and
                set(modules).issubset({"C2_LINEARITY_GAIN_SATURATION", "C5_RELATIVE_RADIOMETRY"}) and
                "C2_LINEARITY_GAIN_SATURATION" in modules,
                "response block requires C2 and may additionally bind C5")
        finite(block.get("scalar"), "response.scalar", positive=True)
        require("perChannel" not in block, "per-channel response scaling is forbidden in v0.1")
        enabled_count += 1

    if "saturation" in parameters:
        block = parameters["saturation"]
        require(isinstance(block, dict), "parameters.saturation must be an object")
        require(block.get("enabled") is True, "parameters.saturation.enabled must be true when block is present")
        require(block.get("module") == "C2_LINEARITY_GAIN_SATURATION",
                "saturation block must bind C2_LINEARITY_GAIN_SATURATION")
        finite(block.get("code"), "saturation.code")
        enabled_count += 1

    require(enabled_count > 0, "at least one calibrated parameter block must be enabled")
    digest = canonical_sha256(model)
    return {
        "schema": MODEL_SCHEMA,
        "valid": True,
        "modelId": model["modelId"],
        "claimQuantity": model["claimQuantity"],
        "captureSampleDomainId": model["captureSampleDomainId"],
        "darkSnrThreshold": dark_snr,
        "enabledBlocks": sorted(parameters.keys()),
        "modelSha256": digest,
        "scientificMasterRouteChanged": False,
        "requestsAdditionalGainMapCorrection": False,
    }


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--contract", type=Path, required=True)
    parser.add_argument("--model", type=Path, required=True)
    parser.add_argument("--expected-model-sha256")
    parser.add_argument("--json-out", type=Path)
    args = parser.parse_args(list(argv) if argv is not None else None)
    try:
        contract = load_json(args.contract)
        model = load_json(args.model)
        result = validate_model(model, contract)
        if args.expected_model_sha256 is not None:
            hex64(args.expected_model_sha256, "expected-model-sha256")
            require(result["modelSha256"] == args.expected_model_sha256,
                    "computed model SHA-256 does not match expected modelSha256")
        rendered = json.dumps(result, indent=2, sort_keys=True) + "\n"
        if args.json_out is not None:
            args.json_out.write_text(rendered, encoding="utf-8")
        print(rendered, end="")
        return 0
    except (OSError, json.JSONDecodeError, ModelValidationError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
