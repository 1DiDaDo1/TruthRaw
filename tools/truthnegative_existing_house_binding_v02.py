#!/usr/bin/env python3
"""TruthNegative v0.2 existing-house binding contract.

This module binds TruthNegative to an already existing Scientific Master.
It does not reconstruct pixels and it does not create a second scientific state.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from enum import Enum
from typing import Any, Dict


class IngressClass(str, Enum):
    NATIVE_CERTIFIED_SOURCE = "NATIVE_CERTIFIED_SOURCE"
    GATEHOUSE_DECODED_MEASUREMENT_HANDOFF = "GATEHOUSE_DECODED_MEASUREMENT_HANDOFF"


CURRENT_MASTER_ROLE = "V47I_RECONSTRUCTED_CAMERA_SCENE"
CURRENT_MASTER_ENCODING = "IEEE754_BINARY32_CAMERA_NATIVE_RGB"


def _sha256(value: str) -> bool:
    if not isinstance(value, str) or len(value) != 64:
        return False
    try:
        int(value, 16)
    except ValueError:
        return False
    return True


def build_binding(
    *,
    source_sha256: str,
    scientific_master_sha256: str,
    ingress_class: IngressClass,
    source_width: int,
    source_height: int,
    projection_width: int,
    projection_height: int,
) -> Dict[str, Any]:
    if not _sha256(source_sha256):
        raise ValueError("invalid source SHA-256")
    if not _sha256(scientific_master_sha256):
        raise ValueError("invalid Scientific Master SHA-256")
    for name, value in {
        "source_width": source_width,
        "source_height": source_height,
        "projection_width": projection_width,
        "projection_height": projection_height,
    }.items():
        if value <= 0:
            raise ValueError(f"{name} must be positive")

    manifest: Dict[str, Any] = {
        "schema": "TruthNegativeExistingHouseBinding/0.2",
        "truthNegativeCore": {
            "createsSecondScientificWorld": False,
            "role": "SCIENTIFIC_NEGATIVE_BINDING_TO_EXISTING_SCIENTIFIC_MASTER",
            "sourceEvidenceSha256": source_sha256.lower(),
            "ingressClass": ingress_class.value,
            "scientificMasterSha256": scientific_master_sha256.lower(),
            "scientificMasterRole": CURRENT_MASTER_ROLE,
            "scientificMasterEncoding": CURRENT_MASTER_ENCODING,
            "authority": "RECONSTRUCTED_SCIENTIFIC_STATE_BINDING",
        },
        "precision": {
            "sourceEvidence": "EXACT_INTEGER_OR_PACKED",
            "branchSensitiveReconstructionCompute": "F64",
            "calibrationOptimizationCovariance": "F64",
            "scientificMasterStorage": "F32_CONDITIONALLY_ALLOWED_AFTER_F64_COMPUTE",
            "higherPrecision": "REFERENCE_VALIDATOR",
            "precisionUpgradesEvidenceAuthority": False,
        },
        "sourceGrid": {
            "width": source_width,
            "height": source_height,
            "sampleCount": source_width * source_height,
            "authority": "MEASURED_IN_SOURCE_DOMAIN_ONLY",
        },
        "sensorNegativeProjection": {
            "width": projection_width,
            "height": projection_height,
            "sampleCount": projection_width * projection_height,
            "createsNewEvidence": False,
            "impliesPhysicalSensorGeometry": False,
            "newTargetSupportAuthority": "RECONSTRUCTED",
            "measuredTargetClaimCount": 0,
            "isOriginalRaw": False,
            "isScientificMasterReplacement": False,
        },
        "evidenceCounts": {
            "physicalFrameCount": 1,
            "independentEvidenceCount": 1,
        },
        "separation": {
            "reconstructedCfaMayBeRelabelledOriginalMeasuredCfa": False,
            "appearanceMayWriteBack": False,
            "honorRouteMayBeProvenByTruthNegative": False,
        },
    }
    validate_binding(manifest)
    manifest["bindingSha256"] = digest(manifest)
    return manifest


def validate_binding(m: Dict[str, Any]) -> None:
    if m.get("schema") != "TruthNegativeExistingHouseBinding/0.2":
        raise ValueError("unexpected schema")

    core = m["truthNegativeCore"]
    precision = m["precision"]
    proj = m["sensorNegativeProjection"]
    counts = m["evidenceCounts"]
    sep = m["separation"]

    if core.get("createsSecondScientificWorld") is not False:
        raise ValueError("TruthNegative must bind the existing reconstructed world")
    if core.get("scientificMasterRole") != CURRENT_MASTER_ROLE:
        raise ValueError("unexpected Scientific Master role")
    if core.get("scientificMasterEncoding") != CURRENT_MASTER_ENCODING:
        raise ValueError("unexpected current Scientific Master encoding")
    if not _sha256(core.get("sourceEvidenceSha256", "")):
        raise ValueError("invalid source binding")
    if not _sha256(core.get("scientificMasterSha256", "")):
        raise ValueError("invalid Scientific Master binding")
    IngressClass(core["ingressClass"])

    if precision.get("precisionUpgradesEvidenceAuthority") is not False:
        raise ValueError("precision may not upgrade evidence authority")
    if precision.get("branchSensitiveReconstructionCompute") != "F64":
        raise ValueError("branch-sensitive reconstruction must retain F64 policy")

    if proj.get("createsNewEvidence") is not False:
        raise ValueError("projection may not create evidence")
    if proj.get("impliesPhysicalSensorGeometry") is not False:
        raise ValueError("projection may not imply physical sensor geometry")
    if proj.get("newTargetSupportAuthority") != "RECONSTRUCTED":
        raise ValueError("new dense support must remain reconstructed")
    if int(proj.get("measuredTargetClaimCount", 0)) != 0:
        raise ValueError("v0.2 forbids target-lattice measured claims")
    if proj.get("isOriginalRaw") is not False:
        raise ValueError("reconstructed projection is not original RAW")
    if proj.get("isScientificMasterReplacement") is not False:
        raise ValueError("sensor projection cannot replace Scientific Master")

    if counts != {"physicalFrameCount": 1, "independentEvidenceCount": 1}:
        raise ValueError("evidence counts changed")

    if sep.get("reconstructedCfaMayBeRelabelledOriginalMeasuredCfa") is not False:
        raise ValueError("reconstructed CFA may not become original evidence")
    if sep.get("appearanceMayWriteBack") is not False:
        raise ValueError("appearance writeback forbidden")
    if sep.get("honorRouteMayBeProvenByTruthNegative") is not False:
        raise ValueError("TruthNegative cannot prove the HONOR route")


def digest(m: Dict[str, Any]) -> str:
    clean = dict(m)
    clean.pop("bindingSha256", None)
    blob = json.dumps(clean, sort_keys=True, separators=(",", ":")).encode()
    return hashlib.sha256(blob).hexdigest()


def _self_test() -> None:
    m = build_binding(
        source_sha256="1" * 64,
        scientific_master_sha256="2" * 64,
        ingress_class=IngressClass.NATIVE_CERTIFIED_SOURCE,
        source_width=4080,
        source_height=3072,
        projection_width=16320,
        projection_height=12288,
    )
    assert m["sourceGrid"]["sampleCount"] == 12_533_760
    assert m["sensorNegativeProjection"]["sampleCount"] == 200_540_160
    assert m["sensorNegativeProjection"]["newTargetSupportAuthority"] == "RECONSTRUCTED"
    assert m["truthNegativeCore"]["createsSecondScientificWorld"] is False
    validate_binding(m)
    print("TruthNegativeExistingHouseBinding/0.2 self-test PASS")


def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("--self-test", action="store_true")
    p.add_argument("--source-sha256")
    p.add_argument("--scientific-master-sha256")
    p.add_argument(
        "--ingress-class",
        choices=[x.value for x in IngressClass],
        default=IngressClass.NATIVE_CERTIFIED_SOURCE.value,
    )
    p.add_argument("--source-width", type=int)
    p.add_argument("--source-height", type=int)
    p.add_argument("--projection-width", type=int)
    p.add_argument("--projection-height", type=int)
    args = p.parse_args()

    if args.self_test:
        _self_test()
        return

    required = [
        args.source_sha256,
        args.scientific_master_sha256,
        args.source_width,
        args.source_height,
        args.projection_width,
        args.projection_height,
    ]
    if any(x is None for x in required):
        p.error("all source/master/grid arguments are required")

    out = build_binding(
        source_sha256=args.source_sha256,
        scientific_master_sha256=args.scientific_master_sha256,
        ingress_class=IngressClass(args.ingress_class),
        source_width=args.source_width,
        source_height=args.source_height,
        projection_width=args.projection_width,
        projection_height=args.projection_height,
    )
    print(json.dumps(out, indent=2))


if __name__ == "__main__":
    main()
