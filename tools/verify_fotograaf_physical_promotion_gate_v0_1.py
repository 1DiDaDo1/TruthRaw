#!/usr/bin/env python3
"""TruthRaw FotoGraaf Physical Promotion Gate v0.1.

This verifier decides only whether an already admitted, independently validated
and shadow-tested calibration claim is ELIGIBLE_FOR_EXPLICIT_PROMOTION_REVIEW.
It never changes source bytes, Scientific Master pixels, Backplane/certificate
schemas, or scene evidence counts.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
from typing import Any, Dict, Iterable, Mapping

try:
    from tools.verify_fotograaf_calibration_pack_v0_1 import load_json, validate_pack
    from tools.verify_fotograaf_calibration_scene_admission_v0_1 import validate_admission_contract
except ModuleNotFoundError:  # direct execution
    from verify_fotograaf_calibration_pack_v0_1 import load_json, validate_pack  # type: ignore
    from verify_fotograaf_calibration_scene_admission_v0_1 import validate_admission_contract  # type: ignore

CONTRACT_SCHEMA = "truthraw.fotograaf-physical-promotion-gate-contract.v0.1"
REQUEST_SCHEMA = "truthraw.fotograaf-physical-promotion-request.v0.1"
VALIDATION_SCHEMA = "truthraw.fotograaf-physical-validation-report.v0.1"
UNCERTAINTY_SCHEMA = "truthraw.fotograaf-uncertainty-validation-report.v0.1"
SHADOW_SCHEMA = "truthraw.fotograaf-shadow-comparison-report.v0.1"
RESULT_SCHEMA = "truthraw.fotograaf-physical-promotion-result.v0.1"
HEX64 = re.compile(r"^[0-9a-f]{64}$")


class PromotionError(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise PromotionError(message)


def require_string(value: Any, name: str) -> str:
    require(isinstance(value, str) and bool(value.strip()), f"{name} must be a non-empty string")
    return value


def require_sha(value: Any, name: str) -> str:
    require(isinstance(value, str) and HEX64.fullmatch(value) is not None,
            f"{name} must be lowercase 64-hex SHA-256")
    return value


def canonical_sha256(value: Any) -> str:
    encoded = json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


def validate_contract(contract: Mapping[str, Any]) -> None:
    require(contract.get("schema") == CONTRACT_SCHEMA, "wrong physical-promotion contract schema")
    require(contract.get("scientificMasterRouteChange") is False,
            "promotion gate may not change Scientific Master route")
    require(contract.get("productionRouteMutation") is False,
            "promotion gate must be non-mutating")
    require(contract.get("promotionEligibilityOnly") is True,
            "v0.1 gate must produce eligibility only")
    require(contract.get("sceneEvidenceCountsChanged") is False,
            "promotion gate may not change scene evidence counts")
    states = contract.get("decisionStates")
    require(isinstance(states, list) and set(states) == {
        "ELIGIBLE_FOR_EXPLICIT_PROMOTION_REVIEW", "REMAIN_SHADOW_ONLY", "PROMOTION_REJECTED"
    }, "promotion decision states changed")
    reports = contract.get("reportSchemas")
    require(isinstance(reports, dict), "reportSchemas missing")
    require(reports.get("promotionRequest") == REQUEST_SCHEMA, "wrong request schema binding")
    require(reports.get("physicalValidation") == VALIDATION_SCHEMA, "wrong physical validation schema binding")
    require(reports.get("uncertaintyValidation") == UNCERTAINTY_SCHEMA, "wrong uncertainty schema binding")
    require(reports.get("shadowComparison") == SHADOW_SCHEMA, "wrong shadow schema binding")
    require(reports.get("promotionResult") == RESULT_SCHEMA, "wrong result schema binding")
    shadow = contract.get("shadowRules")
    require(isinstance(shadow, dict), "shadowRules missing")
    for key in (
        "shadowOnlyRequired", "productionRouteChangedMustBeFalse", "bindingSha256ExactMatchRequired",
        "modelSha256ExactMatchRequired", "protocolSha256ExactMatchRequired",
        "sourceIdentityRegressionPassedRequired", "productionScientificMasterIdentityUnchangedRequired",
        "sourceCensorPreservedRequired", "noDoubleCorrectionPassedRequired",
        "captureSampleDomainRegressionPassedRequired", "comparisonThresholdsPassedRequired",
        "thresholdProtocolMustMatchAcceptanceProtocol", "workerResultSha256sMustBeIdentical"
    ):
        require(shadow.get(key) is True, f"shadowRules.{key} must be true")
    require(shadow.get("workerResultSha256sRequired") == ["1", "2", "4"],
            "worker invariance must require exact 1/2/4 result fingerprints")
    ident = contract.get("identityRules")
    require(isinstance(ident, dict), "identityRules missing")
    require(ident.get("physicalFrameCount") == 1 and ident.get("independentEvidenceCount") == 1,
            "scene evidence counts must remain 1/1")
    require(ident.get("workerCountAffectsAuthority") is False,
            "worker count may not affect promotion authority")
    require(ident.get("calibrationFramesBecomeSceneEvidence") is False,
            "calibration frames may not become scene evidence")
    require(ident.get("calibrationMayUncensorSourceClip") is False,
            "calibration may not uncensor source clipping")
    require(ident.get("calibrationMayReapplySourceGainMap") is False,
            "calibration may not reapply source GainMap")
    gov = contract.get("schemaGovernance")
    require(isinstance(gov, dict) and gov.get("writePromotionIntoBackplaneV0_1") is False and
            gov.get("writePromotionIntoCertificateV0_1") is False and
            gov.get("promotionRecordMayChangeProductionPixelsByItself") is False,
            "promotion gate must not mutate frozen production schemas/pixels")


def _claim(pack: Mapping[str, Any], quantity: str) -> Mapping[str, Any]:
    claims = pack.get("claims")
    require(isinstance(claims, list), "pack.claims missing")
    for claim in claims:
        if isinstance(claim, dict) and claim.get("quantity") == quantity:
            return claim
    raise PromotionError(f"pack has no claim for {quantity}")


def _admitted_binding(admission: Mapping[str, Any], quantity: str) -> Mapping[str, Any]:
    require(admission.get("strictFailure") is False, "scene admission has strictFailure")
    require(admission.get("physicalFrameCount") == 1 and admission.get("independentEvidenceCount") == 1,
            "scene admission evidence counts must remain 1/1")
    require(admission.get("scientificMasterRouteChanged") is False,
            "scene admission may not change Scientific Master route")
    results = admission.get("results")
    require(isinstance(results, list), "admission.results missing")
    for item in results:
        if isinstance(item, dict) and item.get("quantity") == quantity:
            require(item.get("decision") == "CALIBRATED_PHYSICAL_ADMITTED",
                    f"{quantity} was not CALIBRATED_PHYSICAL_ADMITTED")
            binding = item.get("binding")
            require(isinstance(binding, dict), "admitted result missing binding")
            core = {k: v for k, v in binding.items() if k != "bindingSha256"}
            require_sha(binding.get("bindingSha256"), "binding.bindingSha256")
            require(binding["bindingSha256"] == canonical_sha256(core), "bindingSha256 is not canonical")
            require(binding.get("physicalFrameCount") == 1 and binding.get("independentEvidenceCount") == 1,
                    "binding evidence counts must remain 1/1")
            require(binding.get("changesScientificMasterByItself") is False and
                    binding.get("changesSourceEvidence") is False,
                    "binding may not mutate source/master")
            return binding
    raise PromotionError(f"admission has no result for {quantity}")


def _validate_request(request: Mapping[str, Any]) -> None:
    require(request.get("schema") == REQUEST_SCHEMA, "wrong promotion request schema")
    require_string(request.get("quantity"), "request.quantity")
    require_sha(request.get("sourceEvidenceSha256"), "request.sourceEvidenceSha256")
    require_string(request.get("packId"), "request.packId")
    require_sha(request.get("bindingSha256"), "request.bindingSha256")
    require(request.get("physicalFrameCount") == 1 and request.get("independentEvidenceCount") == 1,
            "promotion request evidence counts must remain 1/1")
    require(request.get("requestProductionRouteMutation") is False,
            "v0.1 promotion request may not request production-route mutation")


def _validate_physical_report(report: Mapping[str, Any], claim: Mapping[str, Any],
                              binding: Mapping[str, Any]) -> None:
    require(report.get("schema") == VALIDATION_SCHEMA, "wrong physical validation report schema")
    require(report.get("passed") is True, "held-out physical validation did not pass")
    require(report.get("quantity") == claim.get("quantity"), "physical validation quantity mismatch")
    require(report.get("modelId") == claim.get("modelId"), "physical validation modelId mismatch")
    require(report.get("acceptanceProtocolId") == claim.get("acceptanceProtocolId"),
            "physical validation acceptanceProtocolId mismatch")
    require(report.get("protocolSealedBeforeFinalFit") is True,
            "physical validation protocol was not sealed before final fit")
    require(report.get("fitDataReusedForHeldOutValidation") is False,
            "fit data were reused as held-out validation")
    fit_ids = report.get("fitDatasetIds")
    val_ids = report.get("heldOutValidationDatasetIds")
    require(isinstance(fit_ids, list) and fit_ids and all(isinstance(x, str) and x for x in fit_ids),
            "physical validation fitDatasetIds invalid")
    require(isinstance(val_ids, list) and val_ids and all(isinstance(x, str) and x for x in val_ids),
            "physical validation heldOutValidationDatasetIds invalid")
    require(set(fit_ids).isdisjoint(val_ids), "fit/held-out validation dataset IDs overlap")
    domains = report.get("captureSampleDomainIdsValidated")
    require(isinstance(domains, list) and binding["captureState"]["captureSampleDomainId"] in domains,
            "physical validation does not cover bound captureSampleDomainId")


def _validate_uncertainty_report(report: Mapping[str, Any], claim: Mapping[str, Any],
                                 binding: Mapping[str, Any]) -> None:
    require(report.get("schema") == UNCERTAINTY_SCHEMA, "wrong uncertainty validation report schema")
    require(report.get("passed") is True, "uncertainty validation did not pass")
    require(report.get("quantity") == claim.get("quantity"), "uncertainty quantity mismatch")
    require(report.get("uncertaintyModelId") == claim.get("uncertaintyModelId"),
            "uncertaintyModelId mismatch")
    require(report.get("validDomainBound") is True, "uncertainty report is not valid-domain bound")
    require(report.get("coverageAcceptancePassed") is True,
            "uncertainty coverage/acceptance did not pass")
    require_sha(report.get("validDomainSha256"), "uncertainty.validDomainSha256")
    require(report["validDomainSha256"] == binding.get("validDomainSha256"),
            "uncertainty validDomainSha256 mismatch")


def _validate_shadow_report(report: Mapping[str, Any], claim: Mapping[str, Any],
                            binding: Mapping[str, Any], contract: Mapping[str, Any]) -> None:
    require(report.get("schema") == SHADOW_SCHEMA, "wrong shadow comparison report schema")
    require(report.get("passed") is True, "shadow comparison did not pass")
    require(report.get("shadowOnly") is True, "shadow report is not shadow-only")
    require(report.get("productionRouteChanged") is False, "shadow evaluation changed production route")
    require(report.get("quantity") == claim.get("quantity"), "shadow quantity mismatch")
    require(report.get("sourceEvidenceSha256") == binding.get("sourceEvidenceSha256"),
            "shadow source identity mismatch")
    require(report.get("bindingSha256") == binding.get("bindingSha256"), "shadow bindingSha256 mismatch")
    require(report.get("modelSha256") == binding.get("modelSha256"), "shadow modelSha256 mismatch")
    require(report.get("protocolSha256") == binding.get("protocolSha256"), "shadow protocolSha256 mismatch")
    require(report.get("thresholdProtocolId") == claim.get("acceptanceProtocolId"),
            "shadow threshold protocol differs from acceptance protocol")
    for key in (
        "sourceIdentityRegressionPassed", "sourceCensorPreserved", "noDoubleCorrectionPassed",
        "captureSampleDomainRegressionPassed", "comparisonThresholdsPassed"
    ):
        require(report.get(key) is True, f"shadow {key} must be true")
    before = require_sha(report.get("productionScientificMasterBeforeSha256"),
                         "shadow.productionScientificMasterBeforeSha256")
    after = require_sha(report.get("productionScientificMasterAfterSha256"),
                        "shadow.productionScientificMasterAfterSha256")
    require(before == after, "production Scientific Master identity changed during shadow evaluation")
    workers = report.get("workerResultSha256")
    require(isinstance(workers, dict), "shadow.workerResultSha256 missing")
    required_workers = contract["shadowRules"]["workerResultSha256sRequired"]
    values = []
    for worker in required_workers:
        values.append(require_sha(workers.get(worker), f"shadow.workerResultSha256[{worker}]"))
    require(len(set(values)) == 1, "1/2/4-worker shadow results are not exactly invariant")
    if claim.get("quantity") in contract.get("noiseOnlyQuantities", []):
        require(report.get("noiseOnlySignalCoordinateBitIdentical") is True,
                "noise-only calibration changed signal coordinate")


def _validate_dependencies(pack: Mapping[str, Any], calibration_contract: Mapping[str, Any],
                           quantity: str) -> None:
    promotion_rules = calibration_contract.get("promotionRules")
    require(isinstance(promotion_rules, dict) and quantity in promotion_rules,
            f"no calibration promotion rule for {quantity}")
    modules = pack.get("modules")
    require(isinstance(modules, dict), "pack.modules missing")
    for name in promotion_rules[quantity]:
        record = modules.get(name)
        require(isinstance(record, dict) and record.get("present") is True and record.get("validated") is True,
                f"required calibration module {name} is not present+validated")


def evaluate_promotion(request: Mapping[str, Any], pack: Mapping[str, Any], admission: Mapping[str, Any],
                       physical_report: Mapping[str, Any], uncertainty_report: Mapping[str, Any],
                       shadow_report: Mapping[str, Any], calibration_contract: Mapping[str, Any],
                       admission_contract: Mapping[str, Any], shadow_contract: Mapping[str, Any],
                       promotion_contract: Mapping[str, Any]) -> Dict[str, Any]:
    validate_contract(promotion_contract)
    validate_admission_contract(admission_contract)
    validate_pack(pack, calibration_contract)
    _validate_request(request)

    require(shadow_contract.get("schema") == "truthraw.fotograaf-calibration-model-shadow-contract.v0.1",
            "wrong shadow contract schema")
    require(shadow_contract.get("scientificMasterRouteChange") is False and
            shadow_contract.get("sourceEvidenceMutation") is False and
            shadow_contract.get("sceneEvidenceCountsChanged") is False,
            "shadow contract is not conservation-safe")

    quantity = request["quantity"]
    claim = _claim(pack, quantity)
    require(claim.get("authority") == "CALIBRATED_PHYSICAL" and claim.get("validated") is True,
            "pack claim is not validated CALIBRATED_PHYSICAL")
    binding = _admitted_binding(admission, quantity)

    require(request["sourceEvidenceSha256"] == binding.get("sourceEvidenceSha256"),
            "request sourceEvidenceSha256 mismatch")
    require(request["packId"] == pack.get("packId") == binding.get("packId"), "packId mismatch")
    require(request["bindingSha256"] == binding.get("bindingSha256"), "request bindingSha256 mismatch")
    require(binding.get("datasetManifestSha256") == pack.get("datasetManifestSha256"),
            "dataset manifest binding mismatch")
    require(binding.get("protocolSha256") == pack.get("protocolSha256"), "protocol SHA binding mismatch")
    require(binding.get("modelSha256") == pack.get("modelSha256"), "model SHA binding mismatch")
    require(binding.get("modelId") == claim.get("modelId"), "modelId binding mismatch")
    require(binding.get("uncertaintyModelId") == claim.get("uncertaintyModelId"),
            "uncertaintyModelId binding mismatch")
    require(binding.get("acceptanceProtocolId") == claim.get("acceptanceProtocolId"),
            "acceptanceProtocolId binding mismatch")

    physical_sha = canonical_sha256(physical_report)
    uncertainty_sha = canonical_sha256(uncertainty_report)
    require(physical_sha == claim.get("validationReportSha256") == binding.get("validationReportSha256"),
            "physical validation report SHA does not match admitted claim")
    require(uncertainty_sha == claim.get("uncertaintyReportSha256") == binding.get("uncertaintyReportSha256"),
            "uncertainty report SHA does not match admitted claim")

    _validate_dependencies(pack, calibration_contract, quantity)
    _validate_physical_report(physical_report, claim, binding)
    _validate_uncertainty_report(uncertainty_report, claim, binding)
    _validate_shadow_report(shadow_report, claim, binding, promotion_contract)

    absolute = promotion_contract.get("absoluteQuantityRules", {}).get(quantity)
    if isinstance(absolute, dict):
        require_string(claim.get("unit"), f"{quantity}.unit")
        traceability = pack.get("traceability")
        require(isinstance(traceability, dict) and traceability.get("absolute") is True,
                f"{quantity} requires absolute traceability")
        required_modules = absolute.get("requiredModules", [])
        modules = pack.get("modules", {})
        for name in required_modules:
            record = modules.get(name)
            require(isinstance(record, dict) and record.get("present") is True and record.get("validated") is True,
                    f"absolute claim missing validated module {name}")

    shadow_sha = canonical_sha256(shadow_report)
    record_core: Dict[str, Any] = {
        "schema": RESULT_SCHEMA,
        "decision": "ELIGIBLE_FOR_EXPLICIT_PROMOTION_REVIEW",
        "productionRouteActivated": False,
        "sourceEvidenceSha256": binding["sourceEvidenceSha256"],
        "packId": binding["packId"],
        "quantity": quantity,
        "bindingSha256": binding["bindingSha256"],
        "datasetManifestSha256": binding["datasetManifestSha256"],
        "protocolSha256": binding["protocolSha256"],
        "modelSha256": binding["modelSha256"],
        "modelId": binding["modelId"],
        "uncertaintyModelId": binding["uncertaintyModelId"],
        "validDomainSha256": binding["validDomainSha256"],
        "physicalValidationReportSha256": physical_sha,
        "uncertaintyValidationReportSha256": uncertainty_sha,
        "shadowComparisonReportSha256": shadow_sha,
        "acceptanceProtocolId": binding["acceptanceProtocolId"],
        "captureSampleDomainId": binding["captureState"]["captureSampleDomainId"],
        "physicalFrameCount": 1,
        "independentEvidenceCount": 1,
        "workerCountAffectsAuthority": False,
        "changesScientificMasterByItself": False,
        "requiresSeparateProductionPromotionReview": True,
    }
    if "unit" in claim:
        record_core["unit"] = claim["unit"]
    result = dict(record_core)
    result["eligibilityRecordSha256"] = canonical_sha256(record_core)
    return result


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--calibration-contract", type=Path, required=True)
    parser.add_argument("--admission-contract", type=Path, required=True)
    parser.add_argument("--shadow-contract", type=Path, required=True)
    parser.add_argument("--promotion-contract", type=Path, required=True)
    parser.add_argument("--pack", type=Path)
    parser.add_argument("--admission", type=Path)
    parser.add_argument("--request", type=Path)
    parser.add_argument("--physical-validation", type=Path)
    parser.add_argument("--uncertainty-validation", type=Path)
    parser.add_argument("--shadow-report", type=Path)
    parser.add_argument("--json-out", type=Path)
    args = parser.parse_args(list(argv) if argv is not None else None)
    try:
        calibration_contract = load_json(args.calibration_contract)
        admission_contract = load_json(args.admission_contract)
        shadow_contract = load_json(args.shadow_contract)
        promotion_contract = load_json(args.promotion_contract)
        validate_contract(promotion_contract)
        validate_admission_contract(admission_contract)
        result: Dict[str, Any] = {
            "promotionContractValid": True,
            "schema": CONTRACT_SCHEMA,
        }
        supplied = [args.pack, args.admission, args.request, args.physical_validation,
                    args.uncertainty_validation, args.shadow_report]
        require(all(x is None for x in supplied) or all(x is not None for x in supplied),
                "full promotion evaluation requires all six runtime/report inputs")
        if all(x is not None for x in supplied):
            result["promotion"] = evaluate_promotion(
                load_json(args.request), load_json(args.pack), load_json(args.admission),
                load_json(args.physical_validation), load_json(args.uncertainty_validation),
                load_json(args.shadow_report), calibration_contract, admission_contract,
                shadow_contract, promotion_contract,
            )
        if args.json_out:
            args.json_out.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(json.dumps(result, indent=2, sort_keys=True))
        return 0
    except Exception as exc:
        print(f"FOTOGRAAF_PHYSICAL_PROMOTION_GATE_FAIL: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
