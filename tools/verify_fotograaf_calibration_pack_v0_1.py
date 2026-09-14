#!/usr/bin/env python3
"""Validate TruthRaw FotoGraaf Calibration Pack v0.1 records.

This is a fail-closed structural/admission verifier. It does not establish that
the underlying laboratory measurements are truthful; it verifies that a pack
cannot *claim* CALIBRATED_PHYSICAL authority unless the required scope,
acquisition counts, fit/validation separation, and external-reference fields
declared by the v0.1 contract are present.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any, Dict, Iterable, Mapping

CONTRACT_SCHEMA = "truthraw.fotograaf-calibration-contract.v0.1"
PACK_SCHEMA = "truthraw.fotograaf-calibration-pack.v0.1"
HEX64 = re.compile(r"^[0-9a-f]{64}$")
VALID_PACK_STATUS = {
    "RESEARCH_ONLY",
    "VALIDATED_RELATIVE",
    "VALIDATED_ABSOLUTE_FOR_DECLARED_QUANTITY",
    "REJECTED",
    "EXPIRED_OR_OUT_OF_SCOPE",
}
VALID_AUTHORITY = {
    "MEASURED_SOURCE",
    "CALIBRATED_PHYSICAL",
    "INFERRED_SCENE",
    "BOUNDED_CENSORED",
    "UNKNOWN",
}


class ValidationError(RuntimeError):
    pass


def require(cond: bool, message: str) -> None:
    if not cond:
        raise ValidationError(message)


def load_json(path: Path) -> Dict[str, Any]:
    with path.open("r", encoding="utf-8") as f:
        value = json.load(f)
    require(isinstance(value, dict), f"{path}: top level must be an object")
    return value


def require_nonempty_string(value: Any, name: str) -> None:
    require(isinstance(value, str) and bool(value.strip()), f"{name} must be a non-empty string")


def require_hex64(value: Any, name: str) -> None:
    require(isinstance(value, str) and HEX64.fullmatch(value) is not None,
            f"{name} must be lowercase 64-hex SHA-256")


def _module(contract: Mapping[str, Any], name: str) -> Mapping[str, Any]:
    modules = contract.get("modules")
    require(isinstance(modules, dict) and name in modules, f"contract missing module {name}")
    mod = modules[name]
    require(isinstance(mod, dict), f"contract module {name} must be an object")
    return mod


def validate_contract(contract: Mapping[str, Any]) -> None:
    require(contract.get("schema") == CONTRACT_SCHEMA, "wrong calibration contract schema")
    require(contract.get("scientificMasterRouteChange") is False,
            "calibration contract may not silently change Scientific Master route")
    require(contract.get("sceneEvidenceCountsChangedByCalibration") is False,
            "calibration may not change later scene evidence counts")
    scope = contract.get("scopeRequiredFields")
    require(isinstance(scope, list) and len(scope) >= 10 and len(scope) == len(set(scope)),
            "scopeRequiredFields must be a unique non-trivial list")
    required_modules = {
        "C0_IDENTITY", "C1_DARK_NOISE", "C2_LINEARITY_GAIN_SATURATION",
        "C3_FLAT_SHADING", "C4_COLOR_SPECTRAL", "C5_RELATIVE_RADIOMETRY",
        "C6_ABSOLUTE_RADIOMETRY", "C7_INCIDENT_LIGHT_GEOMETRY",
    }
    modules = contract.get("modules")
    require(isinstance(modules, dict) and set(modules) == required_modules,
            "contract must define exactly C0..C7 modules")
    rules = contract.get("promotionRules")
    require(isinstance(rules, dict) and rules, "promotionRules missing")
    for quantity, deps in rules.items():
        require_nonempty_string(quantity, "promotionRules quantity")
        require(isinstance(deps, list) and deps, f"promotion rule {quantity} has no dependencies")
        for dep in deps:
            require(dep in required_modules, f"promotion rule {quantity} references unknown module {dep}")
    dr = contract.get("dynamicRange")
    require(isinstance(dr, dict), "dynamicRange missing")
    require(dr.get("scientificMeaning") == "capture-domain dynamic range, not display HDR",
            "dynamicRange must remain distinct from display HDR")
    require(dr.get("clippedHighlightRule") and "BOUNDED_CENSORED" in dr["clippedHighlightRule"],
            "dynamicRange must preserve clipped samples as censored")
    fit = contract.get("fitValidationSeparation")
    require(isinstance(fit, dict) and fit.get("required") is True and
            fit.get("validationCapturesMayNotBeUsedToFit") is True,
            "fit/validation separation must be mandatory")


def _validated_module(pack: Mapping[str, Any], name: str) -> Mapping[str, Any]:
    modules = pack.get("modules")
    require(isinstance(modules, dict), "pack.modules missing")
    require(name in modules, f"pack.modules missing {name}")
    rec = modules[name]
    require(isinstance(rec, dict), f"pack.modules.{name} must be an object")
    require(rec.get("present") is True and rec.get("validated") is True,
            f"{name} must be present and validated")
    fit_ids = rec.get("fitDatasetIds")
    val_ids = rec.get("validationDatasetIds")
    require(isinstance(fit_ids, list) and all(isinstance(x, str) and x for x in fit_ids),
            f"{name}.fitDatasetIds invalid")
    require(isinstance(val_ids, list) and all(isinstance(x, str) and x for x in val_ids),
            f"{name}.validationDatasetIds invalid")
    require(set(fit_ids).isdisjoint(val_ids), f"{name}: fit and validation datasets overlap")
    return rec


def _metric_int(metrics: Mapping[str, Any], key: str, minimum: int, module: str) -> None:
    value = metrics.get(key)
    require(isinstance(value, int) and not isinstance(value, bool) and value >= minimum,
            f"{module}.metrics.{key} must be >= {minimum}")


def _metric_true(metrics: Mapping[str, Any], key: str, module: str) -> None:
    require(metrics.get(key) is True, f"{module}.metrics.{key} must be true")


def validate_module_acquisition(pack: Mapping[str, Any], contract: Mapping[str, Any], name: str) -> None:
    rec = _validated_module(pack, name)
    metrics = rec.get("metrics")
    require(isinstance(metrics, dict), f"{name}.metrics missing")
    acquisition = _module(contract, name).get("acquisition", {})
    if name == "C0_IDENTITY":
        minimum = _module(contract, name).get("minimum", {})
        for key in ("captureFileSha256ForEveryFrame", "parserBackendIdentity",
                    "scopeKeyComplete", "metadataSnapshotPerFrame", "gainMapOpcodeIdentity"):
            if minimum.get(key) is True:
                _metric_true(metrics, key, name)
    elif name == "C1_DARK_NOISE":
        _metric_int(metrics, "exposureTimesPerState", int(acquisition["minExposureTimesPerState"]), name)
        _metric_int(metrics, "minRepeatsPerGainExposureTemperatureCell",
                    int(acquisition["minRepeatsPerGainExposureTemperatureCell"]), name)
        _metric_int(metrics, "gainReadoutStatesCovered", 1, name)
        _metric_true(metrics, "photonBlockedDark", name)
        _metric_true(metrics, "allIntendedGainReadoutStatesCovered", name)
        if metrics.get("temperatureCalibrated") is True:
            _metric_int(metrics, "temperatureBins", int(acquisition["temperatureBinsForTemperatureClaim"]), name)
    elif name == "C2_LINEARITY_GAIN_SATURATION":
        _metric_int(metrics, "signalLevelsPerGainReadoutState",
                    int(acquisition["minSignalLevelsPerGainReadoutState"]), name)
        _metric_int(metrics, "minRepeatsPerSignalLevel", int(acquisition["minRepeatsPerSignalLevel"]), name)
        _metric_int(metrics, "levelsBracketingSaturationOnset",
                    int(acquisition["minLevelsBracketingSaturationOnset"]), name)
        _metric_int(metrics, "independentValidationLevelsNotUsedForFit",
                    int(acquisition["independentValidationLevelsNotUsedForFit"]), name)
        _metric_true(metrics, "darkReferenceUsed", name)
        _metric_true(metrics, "sourceStabilityMonitored", name)
    elif name == "C3_FLAT_SHADING":
        _metric_int(metrics, "signalLevels", int(acquisition["minSignalLevels"]), name)
        _metric_int(metrics, "minRepeatsPerSignalLevel", int(acquisition["minRepeatsPerSignalLevel"]), name)
        _metric_true(metrics, "darkReferenceUsed", name)
        _metric_true(metrics, "fieldUniformityCharacterized", name)
        _metric_true(metrics, "allClaimedFocusStatesCovered", name)
    elif name == "C4_COLOR_SPECTRAL":
        _metric_int(metrics, "characterizedIlluminants", int(acquisition["minCharacterizedIlluminants"]), name)
        _metric_int(metrics, "targetPatches", int(acquisition["minTargetPatches"]), name)
        _metric_int(metrics, "minRepeatsPerIlluminant", int(acquisition["minRepeatsPerIlluminant"]), name)
        _metric_true(metrics, "spdMeasured", name)
        _metric_true(metrics, "spectralTargetDataAvailable", name)
        _metric_true(metrics, "heldOutValidation", name)
    elif name == "C5_RELATIVE_RADIOMETRY":
        _metric_int(metrics, "referenceLevels", int(acquisition["minReferenceLevels"]), name)
        _metric_int(metrics, "minRepeatsPerLevel", int(acquisition["minRepeatsPerLevel"]), name)
        _metric_int(metrics, "heldOutValidationLevels", int(acquisition["heldOutValidationLevels"]), name)
        _metric_true(metrics, "referenceStabilityMeasured", name)
        _metric_true(metrics, "geometryFixedAndRecorded", name)
        _metric_true(metrics, "exposureAndApertureProvenance", name)
    elif name == "C6_ABSOLUTE_RADIOMETRY":
        _metric_int(metrics, "referenceLevels", int(acquisition["minReferenceLevels"]), name)
        _metric_int(metrics, "minRepeatsPerLevel", int(acquisition["minRepeatsPerLevel"]), name)
        for key in ("traceableReference", "instrumentModelSerialPresent",
                    "calibrationCertificateIdentityPresent", "certificateValidAtAcquisition",
                    "measurementUncertaintyPresent", "spectralBandpassPresent",
                    "geometryAndAngularConditionsPresent"):
            _metric_true(metrics, key, name)
    elif name == "C7_INCIDENT_LIGHT_GEOMETRY":
        _metric_int(metrics, "distinctLightDirections", int(acquisition["minDistinctLightDirections"]), name)
        _metric_int(metrics, "distinctLightLevelsOrDistances",
                    int(acquisition["minDistinctLightLevelsOrDistances"]), name)
        for key in ("knownOrMeasuredGeometry", "surfaceNormals", "materialReflectanceOrBrdfReference",
                    "lightPositionDirection", "cosineCorrectedIrradianceReference",
                    "shadowVisibilityGroundTruth"):
            _metric_true(metrics, key, name)


def _has_external_reference(pack: Mapping[str, Any], kind: str, absolute: bool = False) -> bool:
    refs = pack.get("externalReferences")
    if not isinstance(refs, list):
        return False
    for ref in refs:
        if not isinstance(ref, dict) or ref.get("kind") != kind:
            continue
        if absolute:
            if not (ref.get("traceable") is True and ref.get("certificateId") and
                    ref.get("uncertainty") and ref.get("serial")):
                continue
        return True
    return False


def validate_pack(pack: Mapping[str, Any], contract: Mapping[str, Any]) -> Dict[str, Any]:
    validate_contract(contract)
    require(pack.get("schema") == PACK_SCHEMA, "wrong calibration pack schema")
    require_nonempty_string(pack.get("packId"), "packId")
    status = pack.get("status")
    require(status in VALID_PACK_STATUS, f"invalid status {status!r}")
    scope = pack.get("scope")
    require(isinstance(scope, dict), "scope missing")
    for field in contract["scopeRequiredFields"]:
        value = scope.get(field)
        if field in ("rawWidth", "rawHeight"):
            require(isinstance(value, int) and value > 0, f"scope.{field} must be positive integer")
        else:
            require_nonempty_string(value, f"scope.{field}")
    for field in ("datasetManifestSha256", "protocolSha256", "modelSha256"):
        require_hex64(pack.get(field), field)
    require(pack.get("physicalFrameCountForLaterScene") == 1,
            "calibration pack may not change later scene physicalFrameCount")
    require(pack.get("independentEvidenceCountForLaterScene") == 1,
            "calibration pack may not change later scene independentEvidenceCount")
    require(pack.get("fitValidationSeparated") is True,
            "fitValidationSeparated must be true")
    require(pack.get("thresholdProtocolSealedBeforeFit") is True,
            "thresholdProtocolSealedBeforeFit must be true")
    temperature = pack.get("temperature")
    require(isinstance(temperature, dict), "temperature record missing")
    require(isinstance(temperature.get("calibrated"), bool), "temperature.calibrated must be boolean")
    bins = temperature.get("bins")
    require(isinstance(bins, list), "temperature.bins must be a list")
    if temperature["calibrated"]:
        min_bins = int(contract["environment"]["packTemperatureAdmission"]["minBinsWhenTemperatureCalibrated"])
        require(len(bins) >= min_bins, f"temperature-calibrated pack requires at least {min_bins} bins")
        for module_name, module_record in pack.get("modules", {}).items():
            if isinstance(module_record, dict) and module_record.get("validated") is True:
                metrics = module_record.get("metrics", {})
                require(isinstance(metrics, dict), f"{module_name}.metrics missing")
                covered = metrics.get("temperatureBinsCovered")
                require(isinstance(covered, int) and covered >= min_bins,
                        f"{module_name}.metrics.temperatureBinsCovered must be >= {min_bins} for temperature-calibrated pack")

    claims = pack.get("claims")
    require(isinstance(claims, list), "claims must be a list")
    eligibility: Dict[str, str] = {}
    rules = contract["promotionRules"]

    for claim in claims:
        require(isinstance(claim, dict), "claim must be an object")
        quantity = claim.get("quantity")
        authority = claim.get("authority")
        require(quantity in rules, f"unknown calibrated quantity {quantity!r}")
        require(authority in VALID_AUTHORITY, f"invalid authority {authority!r} for {quantity}")
        validated = claim.get("validated") is True
        if authority == "CALIBRATED_PHYSICAL":
            require(validated, f"{quantity}: CALIBRATED_PHYSICAL requires validated=true")
            require(status not in {"RESEARCH_ONLY", "REJECTED", "EXPIRED_OR_OUT_OF_SCOPE"},
                    f"{quantity}: pack status {status} cannot emit CALIBRATED_PHYSICAL")
            for dependency in rules[quantity]:
                validate_module_acquisition(pack, contract, dependency)
            if quantity == "capture_dynamic_range":
                dynamic_range = claim.get("dynamicRange")
                require(isinstance(dynamic_range, dict), "capture_dynamic_range.dynamicRange missing")
                require(isinstance(dynamic_range.get("snrThreshold"), (int, float)) and
                        dynamic_range["snrThreshold"] > 0,
                        "capture_dynamic_range requires positive snrThreshold")
                require_nonempty_string(dynamic_range.get("saturationModelId"),
                                        "capture_dynamic_range.saturationModelId")
                require_nonempty_string(dynamic_range.get("noiseFloorModelId"),
                                        "capture_dynamic_range.noiseFloorModelId")
            if quantity == "absolute_scene_radiance":
                require(status == "VALIDATED_ABSOLUTE_FOR_DECLARED_QUANTITY",
                        "absolute_scene_radiance requires absolute pack status")
                traceability = pack.get("traceability")
                require(isinstance(traceability, dict) and traceability.get("absolute") is True,
                        "absolute_scene_radiance requires absolute traceability")
                require(_has_external_reference(pack, "radiance_or_spectral_radiance_reference", absolute=True),
                        "absolute_scene_radiance requires traceable radiance reference")
            if quantity == "validated_incident_light_inference":
                require(_has_external_reference(pack, "cosine_corrected_irradiance_reference"),
                        "validated incident-light inference requires cosine-corrected irradiance reference")
            if quantity == "absolute_incident_irradiance":
                require(status == "VALIDATED_ABSOLUTE_FOR_DECLARED_QUANTITY",
                        "absolute_incident_irradiance requires absolute pack status")
                traceability = pack.get("traceability")
                require(isinstance(traceability, dict) and traceability.get("absolute") is True,
                        "absolute_incident_irradiance requires absolute traceability")
                require(_has_external_reference(pack, "cosine_corrected_irradiance_reference", absolute=True),
                        "absolute_incident_irradiance requires traceable cosine-corrected irradiance reference")
            if quantity == "independent_colorimetry":
                require(_has_external_reference(pack, "illuminant_spd_measurement"),
                        "independent_colorimetry requires illuminant SPD reference")
                require(_has_external_reference(pack, "spectral_target_reference"),
                        "independent_colorimetry requires spectral target reference")
            eligibility[quantity] = "CALIBRATED_PHYSICAL_ADMITTED"
        else:
            eligibility[quantity] = f"{authority}_ONLY"

    return {
        "schema": PACK_SCHEMA,
        "packId": pack["packId"],
        "status": status,
        "valid": True,
        "promotionEligibility": eligibility,
        "sceneEvidenceCounts": {"physicalFrameCount": 1, "independentEvidenceCount": 1},
    }


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--contract", type=Path, required=True)
    parser.add_argument("--pack", type=Path)
    parser.add_argument("--json-out", type=Path)
    args = parser.parse_args(list(argv) if argv is not None else None)
    try:
        contract = load_json(args.contract)
        validate_contract(contract)
        result: Dict[str, Any] = {"contractValid": True, "schema": CONTRACT_SCHEMA}
        if args.pack is not None:
            result["pack"] = validate_pack(load_json(args.pack), contract)
        if args.json_out is not None:
            args.json_out.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(json.dumps(result, indent=2, sort_keys=True))
        return 0
    except (OSError, json.JSONDecodeError, ValidationError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
