#!/usr/bin/env python3
"""TruthNegative v0.1 scientific-contract foundation.

No image reconstruction occurs here. This module only builds and validates
machine-readable manifests for the authority-separated TruthNegative pipeline.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from dataclasses import dataclass, asdict
from enum import Enum
from typing import Any, Dict


class Authority(str, Enum):
    MEASURED = "MEASURED"
    CALIBRATED_ESTIMATE = "CALIBRATED_ESTIMATE"
    RECONSTRUCTED = "RECONSTRUCTED"
    CENSORED = "CENSORED"
    UNKNOWN = "UNKNOWN"
    COUNTERFACTUAL = "COUNTERFACTUAL"
    APPEARANCE_ONLY = "APPEARANCE_ONLY"


class SamplingModelStatus(str, Enum):
    UNRESOLVED = "UNRESOLVED"
    RESEARCH_NORMALIZED_IMAGE_PLANE = "RESEARCH_NORMALIZED_IMAGE_PLANE"
    CALIBRATED_FORWARD_MODEL = "CALIBRATED_FORWARD_MODEL"


@dataclass(frozen=True)
class Grid:
    width: int
    height: int

    @property
    def sample_count(self) -> int:
        return self.width * self.height

    def validate(self) -> None:
        if self.width <= 0 or self.height <= 0:
            raise ValueError("grid dimensions must be positive")


def _is_sha256(value: str) -> bool:
    if len(value) != 64:
        return False
    try:
        int(value, 16)
    except ValueError:
        return False
    return True


def build_manifest(
    *,
    source_sha256: str,
    source_width: int,
    source_height: int,
    target_width: int,
    target_height: int,
    sampling_model_status: SamplingModelStatus = SamplingModelStatus.UNRESOLVED,
) -> Dict[str, Any]:
    source = Grid(source_width, source_height)
    target = Grid(target_width, target_height)
    source.validate()
    target.validate()

    if not _is_sha256(source_sha256):
        raise ValueError("source_sha256 must be exactly 64 hexadecimal characters")

    manifest: Dict[str, Any] = {
        "schema": "TruthNegativeFoundation/0.1",
        "authority": "RECONSTRUCTED_SCIENTIFIC_INTERMEDIATE_CONTRACT",
        "sourceEvidence": {
            "sha256": source_sha256.lower(),
            "immutable": True,
            "grid": asdict(source),
            "sampleCount": source.sample_count,
            "authority": Authority.MEASURED.value,
        },
        "reconstructionDomain": {
            "type": "CONTINUOUS_OR_LATENT_CAMERA_SCENE_FIELD",
            "samplingModelStatus": sampling_model_status.value,
            "createsNewEvidence": False,
            "defaultNewSupportAuthority": Authority.RECONSTRUCTED.value,
        },
        "projectionGrid": {
            "grid": asdict(target),
            "sampleCount": target.sample_count,
            "requestedRepresentationOnly": True,
            "impliesPhysicalSensorGeometry": False,
            "measuredClaimCount": 0,
            "newSupportAuthority": Authority.RECONSTRUCTED.value,
        },
        "evidenceCounts": {
            "physicalFrameCount": 1,
            "independentEvidenceCount": 1,
        },
        "permissions": {
            "scientificMasterWritebackAllowed": False,
            "appearanceWritebackAllowed": False,
            "counterfactualWritebackAllowed": False,
        },
        "laws": {
            "measuredMayBeRelabelledFromReconstruction": False,
            "sourceMayBeMutated": False,
            "targetResolutionMayUpgradeEvidenceAuthority": False,
        },
    }
    validate_manifest(manifest)
    manifest["manifestSha256"] = manifest_digest(manifest)
    return manifest


def validate_manifest(manifest: Dict[str, Any]) -> None:
    if manifest.get("schema") != "TruthNegativeFoundation/0.1":
        raise ValueError("unexpected schema")

    source = manifest["sourceEvidence"]
    reconstruction = manifest["reconstructionDomain"]
    projection = manifest["projectionGrid"]
    counts = manifest["evidenceCounts"]
    laws = manifest["laws"]

    if source.get("immutable") is not True:
        raise ValueError("source evidence must be immutable")
    if not _is_sha256(source.get("sha256", "")):
        raise ValueError("source SHA-256 invalid")
    if reconstruction.get("createsNewEvidence") is not False:
        raise ValueError("TruthNegative must not create new evidence")
    if counts.get("physicalFrameCount") != 1:
        raise ValueError("physicalFrameCount must remain 1")
    if counts.get("independentEvidenceCount") != 1:
        raise ValueError("independentEvidenceCount must remain 1")
    if projection.get("impliesPhysicalSensorGeometry") is not False:
        raise ValueError("projection grid must not imply physical sensor geometry")
    if laws.get("measuredMayBeRelabelledFromReconstruction") is not False:
        raise ValueError("reconstruction may never be relabelled measured")
    if laws.get("sourceMayBeMutated") is not False:
        raise ValueError("source mutation is forbidden")
    if laws.get("targetResolutionMayUpgradeEvidenceAuthority") is not False:
        raise ValueError("target resolution cannot upgrade evidence authority")

    status = SamplingModelStatus(reconstruction["samplingModelStatus"])
    measured_claim_count = int(projection.get("measuredClaimCount", 0))
    if measured_claim_count < 0:
        raise ValueError("measuredClaimCount cannot be negative")
    if status == SamplingModelStatus.UNRESOLVED and measured_claim_count != 0:
        raise ValueError(
            "unresolved sampling model forbids measured claims on the projection grid"
        )

    for key in ("sourceEvidence", "projectionGrid"):
        grid = manifest[key]["grid"]
        Grid(int(grid["width"]), int(grid["height"])).validate()

    projected_samples = int(projection["sampleCount"])
    expected_projected = (
        int(projection["grid"]["width"]) * int(projection["grid"]["height"])
    )
    if projected_samples != expected_projected:
        raise ValueError("projection sample count mismatch")

    source_samples = int(source["sampleCount"])
    expected_source = int(source["grid"]["width"]) * int(source["grid"]["height"])
    if source_samples != expected_source:
        raise ValueError("source sample count mismatch")


def manifest_digest(manifest: Dict[str, Any]) -> str:
    clean = dict(manifest)
    clean.pop("manifestSha256", None)
    payload = json.dumps(clean, sort_keys=True, separators=(",", ":")).encode("utf-8")
    return hashlib.sha256(payload).hexdigest()


def _self_test() -> None:
    m = build_manifest(
        source_sha256="0" * 64,
        source_width=4080,
        source_height=3072,
        target_width=16320,
        target_height=12288,
    )
    assert m["sourceEvidence"]["sampleCount"] == 12_533_760
    assert m["projectionGrid"]["sampleCount"] == 200_540_160
    assert m["projectionGrid"]["measuredClaimCount"] == 0
    assert m["evidenceCounts"] == {
        "physicalFrameCount": 1,
        "independentEvidenceCount": 1,
    }
    validate_manifest(m)
    print("TruthNegativeFoundation/0.1 self-test PASS")


def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("--self-test", action="store_true")
    p.add_argument("--source-sha256")
    p.add_argument("--source-width", type=int)
    p.add_argument("--source-height", type=int)
    p.add_argument("--target-width", type=int)
    p.add_argument("--target-height", type=int)
    p.add_argument("--output")
    args = p.parse_args()

    if args.self_test:
        _self_test()
        return

    required = [
        args.source_sha256,
        args.source_width,
        args.source_height,
        args.target_width,
        args.target_height,
    ]
    if any(v is None for v in required):
        p.error("source SHA/dimensions and target dimensions are required")

    manifest = build_manifest(
        source_sha256=args.source_sha256,
        source_width=args.source_width,
        source_height=args.source_height,
        target_width=args.target_width,
        target_height=args.target_height,
    )
    text = json.dumps(manifest, indent=2) + "\n"
    if args.output:
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(text)
    else:
        print(text, end="")


if __name__ == "__main__":
    main()
