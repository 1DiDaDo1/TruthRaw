#!/usr/bin/env python3
"""Bind an admitted FotoGraaf calibration binding to the exact serialized model artifact.

This is a shadow-only bridge. It validates identity and materializes numeric model
parameters for diagnostic execution. It never modifies source or Scientific Master.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
import re
import sys
from pathlib import Path
from typing import Any, Dict, Iterable, Mapping

try:
    from tools.verify_fotograaf_calibration_measurement_model_v0_1 import (
        ModelValidationError,
        load_json,
        validate_model,
    )
except ModuleNotFoundError:
    from verify_fotograaf_calibration_measurement_model_v0_1 import (  # type: ignore
        ModelValidationError,
        load_json,
        validate_model,
    )

BINDING_SCHEMA = "truthraw.fotograaf-calibration-binding.v0.1"
PACKET_SCHEMA = "truthraw.fotograaf-calibration-shadow-model-packet.v0.1"
HEX64 = re.compile(r"^[0-9a-f]{64}$")


class BindingModelError(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise BindingModelError(message)


def hex64(value: Any, name: str) -> str:
    require(isinstance(value, str) and HEX64.fullmatch(value) is not None,
            f"{name} must be lowercase 64-hex")
    return value


def canonical_sha256(value: Mapping[str, Any]) -> str:
    encoded = json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


def validate_binding(binding: Mapping[str, Any]) -> None:
    require(binding.get("schema") == BINDING_SCHEMA, "wrong calibration binding schema")
    for field in (
        "sourceEvidenceSha256", "datasetManifestSha256", "protocolSha256", "modelSha256",
        "calibrationScopeSha256", "validDomainSha256", "validationReportSha256",
        "uncertaintyReportSha256", "bindingSha256",
    ):
        hex64(binding.get(field), field)
    require(binding.get("authority") == "CALIBRATED_PHYSICAL", "binding authority must be CALIBRATED_PHYSICAL")
    require(binding.get("physicalFrameCount") == 1 and binding.get("independentEvidenceCount") == 1,
            "binding must preserve 1/1 scene evidence counts")
    require(binding.get("changesScientificMasterByItself") is False,
            "binding may not change Scientific Master by itself")
    require(binding.get("changesSourceEvidence") is False, "binding may not change source evidence")
    require(binding.get("workerCountAffectsBinding") is False, "worker count may not affect binding")
    for field in ("packId", "quantity", "modelId", "uncertaintyModelId", "acceptanceProtocolId"):
        require(isinstance(binding.get(field), str) and bool(binding[field]), f"binding.{field} missing")
    capture = binding.get("captureState")
    require(isinstance(capture, dict), "binding.captureState missing")
    require(isinstance(capture.get("captureSampleDomainId"), str) and capture["captureSampleDomainId"],
            "binding.captureState.captureSampleDomainId missing")

    core = copy.deepcopy(dict(binding))
    expected = core.pop("bindingSha256")
    require(canonical_sha256(core) == expected, "bindingSha256 does not match binding content")


def bind_model(binding: Mapping[str, Any], model: Mapping[str, Any], model_contract: Mapping[str, Any]) -> Dict[str, Any]:
    validate_binding(binding)
    model_result = validate_model(model, model_contract)
    require(model_result["modelSha256"] == binding["modelSha256"],
            "serialized model SHA-256 does not match admitted binding")
    require(model.get("protocolSha256") == binding["protocolSha256"],
            "model protocolSha256 does not match admitted binding")
    require(model.get("datasetManifestSha256") == binding["datasetManifestSha256"],
            "model datasetManifestSha256 does not match admitted binding")
    require(model.get("calibrationScopeSha256") == binding["calibrationScopeSha256"],
            "model calibrationScopeSha256 does not match admitted binding")
    require(model.get("claimQuantity") == binding["quantity"],
            "model claimQuantity does not match admitted binding")
    require(model.get("modelId") == binding["modelId"], "modelId does not match admitted binding")
    require(model.get("captureSampleDomainId") == binding["captureState"]["captureSampleDomainId"],
            "model captureSampleDomainId does not match admitted scene domain")

    params = model["parameters"]
    black = params.get("black")
    noise = params.get("noise")
    response = params.get("response")
    saturation = params.get("saturation")

    packet_core: Dict[str, Any] = {
        "schema": PACKET_SCHEMA,
        "sourceEvidenceSha256": binding["sourceEvidenceSha256"],
        "bindingSha256": binding["bindingSha256"],
        "protocolSha256": binding["protocolSha256"],
        "modelSha256": binding["modelSha256"],
        "modelId": binding["modelId"],
        "uncertaintyModelId": binding["uncertaintyModelId"],
        "captureSampleDomainId": binding["captureState"]["captureSampleDomainId"],
        "claimQuantity": binding["quantity"],
        "physicalFrameCount": 1,
        "independentEvidenceCount": 1,
        "shadowOnly": True,
        "changesScientificMaster": False,
        "parameters": {
            "useCalibratedBlack": black is not None,
            "calibratedBlackPhaseCode": black["phaseCodeOffsets"] if black else None,
            "useCalibratedNoise": noise is not None,
            "calibratedNoiseProfileRgb": noise["rgbAffineSO"] if noise else None,
            "useCalibratedResponseScale": response is not None,
            "responseScale": response["scalar"] if response else 1.0,
            "useCalibratedSaturation": saturation is not None,
            "calibratedSaturationCode": saturation["code"] if saturation else None,
            "darkSnrThreshold": model["darkSnrThreshold"],
            "requestsAdditionalGainMapCorrection": False,
        },
    }
    packet = dict(packet_core)
    packet["packetSha256"] = canonical_sha256(packet_core)
    return packet


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--model-contract", type=Path, required=True)
    parser.add_argument("--binding", type=Path, required=True)
    parser.add_argument("--model", type=Path, required=True)
    parser.add_argument("--json-out", type=Path)
    args = parser.parse_args(list(argv) if argv is not None else None)
    try:
        packet = bind_model(load_json(args.binding), load_json(args.model), load_json(args.model_contract))
        rendered = json.dumps(packet, indent=2, sort_keys=True) + "\n"
        if args.json_out is not None:
            args.json_out.write_text(rendered, encoding="utf-8")
        print(rendered, end="")
        return 0
    except (OSError, json.JSONDecodeError, ModelValidationError, BindingModelError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
