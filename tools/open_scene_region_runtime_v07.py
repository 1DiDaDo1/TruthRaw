#!/usr/bin/env python3
"""TruthRaw Open Scene Region Runtime v0.7.

v0.7 composes existing Dynamic Authority and Structure Evidence with explicit
scene-physics, colour-calibration, HDR and conservation/restoration boundaries.
It is an authority/provenance runtime, not a renderer and not a generative
restoration algorithm.

Permanent laws enforced here:
- source/master/authority identities are bound and immutable;
- measured/calibrated/reconstructed/censored/unknown remain distinguishable;
- source-bound colour metadata is not independent physical calibration;
- structure/detail appearance permission is not new measured detail;
- censored HDR support remains a bound, UNKNOWN creates no scientific headroom;
- counterfactual illumination cannot write back into captured-world science;
- restoration/loss compensation cannot overpaint valid measured support;
- appearance/restoration/counterfactual operations create no new evidence.
"""
from __future__ import annotations

import hashlib
import json
from dataclasses import dataclass
from enum import Enum
from typing import Optional, Sequence, Tuple

from open_world_foundations_v01 import ContractError
from open_world_dynamic_authority_v05 import DynamicAuthority, DynamicAuthoritySample
from open_world_structure_runtime_v04 import RuntimeStructureResult, RuntimeStructureStatus
from restoration_authority_v01 import evaluate_site

_SHA256_HEX = set("0123456789abcdef")


def _check_sha256(value: str, name: str) -> str:
    v = str(value).lower()
    if len(v) != 64 or any(c not in _SHA256_HEX for c in v):
        raise ContractError(f"{name} must be a 64-character hexadecimal SHA-256")
    return v


def _canonical_hash(payload: dict) -> str:
    blob = json.dumps(payload, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("utf-8")
    return hashlib.sha256(blob).hexdigest()


class IlluminationAuthority(str, Enum):
    UNKNOWN = "UNKNOWN"
    SOURCE_BOUND_ESTIMATE = "SOURCE_BOUND_ESTIMATE"
    RECONSTRUCTED = "RECONSTRUCTED"
    INDEPENDENTLY_MEASURED = "INDEPENDENTLY_MEASURED"
    COUNTERFACTUAL = "COUNTERFACTUAL"


class ColourCalibrationAuthority(str, Enum):
    UNKNOWN = "UNKNOWN"
    SOURCE_METADATA_BOUND = "SOURCE_METADATA_BOUND"
    INDEPENDENT_HELDOUT_VALIDATED = "INDEPENDENT_HELDOUT_VALIDATED"
    APPEARANCE_ONLY = "APPEARANCE_ONLY"


class HDRChannelStatus(str, Enum):
    EVIDENCE_SUPPORTED_FINITE = "EVIDENCE_SUPPORTED_FINITE"
    RECONSTRUCTED_FINITE_NOT_MEASURED = "RECONSTRUCTED_FINITE_NOT_MEASURED"
    CENSORED_BOUND_ONLY = "CENSORED_BOUND_ONLY"
    NO_SCIENTIFIC_HEADROOM = "NO_SCIENTIFIC_HEADROOM"
    NON_SCIENTIFIC_VIEW_ONLY = "NON_SCIENTIFIC_VIEW_ONLY"


class DetailStatus(str, Enum):
    APPEARANCE_DETAIL_ALLOWED = "APPEARANCE_DETAIL_ALLOWED"
    NEUTRAL_OR_BLOCKED = "NEUTRAL_OR_BLOCKED"
    OUT_OF_DOMAIN = "OUT_OF_DOMAIN"


@dataclass(frozen=True)
class SceneRegionBindingsV07:
    region_id: str
    source_evidence_sha256: str
    scientific_master_sha256: str
    dynamic_authority_artifact_sha256: str
    width: int
    height: int
    physical_frame_count: int = 1
    independent_evidence_count: int = 1

    def validate(self) -> "SceneRegionBindingsV07":
        if not self.region_id.strip():
            raise ContractError("region_id may not be empty")
        _check_sha256(self.source_evidence_sha256, "source_evidence_sha256")
        _check_sha256(self.scientific_master_sha256, "scientific_master_sha256")
        _check_sha256(self.dynamic_authority_artifact_sha256, "dynamic_authority_artifact_sha256")
        if self.width <= 0 or self.height <= 0:
            raise ContractError("scene dimensions must be positive")
        if self.physical_frame_count != 1 or self.independent_evidence_count != 1:
            raise ContractError("v0.7 single-frame scene state must remain one frame / one independent evidence item")
        return self


@dataclass(frozen=True)
class ColourCalibrationBindingV07:
    authority: ColourCalibrationAuthority
    transform_sha256: Optional[str] = None
    calibration_evidence_sha256: Optional[str] = None
    holdout_report_sha256: Optional[str] = None

    def validate(self) -> "ColourCalibrationBindingV07":
        if self.transform_sha256 is not None:
            _check_sha256(self.transform_sha256, "transform_sha256")
        if self.calibration_evidence_sha256 is not None:
            _check_sha256(self.calibration_evidence_sha256, "calibration_evidence_sha256")
        if self.holdout_report_sha256 is not None:
            _check_sha256(self.holdout_report_sha256, "holdout_report_sha256")

        if self.authority is ColourCalibrationAuthority.SOURCE_METADATA_BOUND:
            if self.transform_sha256 is None:
                raise ContractError("SOURCE_METADATA_BOUND colour requires a bound transform hash")
            if self.calibration_evidence_sha256 is not None or self.holdout_report_sha256 is not None:
                raise ContractError("source metadata binding must not masquerade as independent calibration evidence")
        elif self.authority is ColourCalibrationAuthority.INDEPENDENT_HELDOUT_VALIDATED:
            if not (self.transform_sha256 and self.calibration_evidence_sha256 and self.holdout_report_sha256):
                raise ContractError("independent physical colour calibration requires transform, calibration evidence and holdout hashes")
        elif self.authority is ColourCalibrationAuthority.APPEARANCE_ONLY:
            if self.calibration_evidence_sha256 is not None:
                raise ContractError("appearance-only colour may not carry physical calibration evidence")
        return self

    @property
    def full_physical_colour_claim_allowed(self) -> bool:
        return self.authority is ColourCalibrationAuthority.INDEPENDENT_HELDOUT_VALIDATED


@dataclass(frozen=True)
class IlluminationBindingV07:
    authority: IlluminationAuthority
    evidence_sha256: Optional[str] = None

    def validate(self) -> "IlluminationBindingV07":
        if self.evidence_sha256 is not None:
            _check_sha256(self.evidence_sha256, "illumination evidence_sha256")
        if self.authority is IlluminationAuthority.INDEPENDENTLY_MEASURED and not self.evidence_sha256:
            raise ContractError("independently measured illumination requires an evidence hash")
        if self.authority is IlluminationAuthority.COUNTERFACTUAL and self.evidence_sha256 is not None:
            raise ContractError("counterfactual illumination may not masquerade as captured illumination evidence")
        return self

    @property
    def captured_world_writeback_allowed(self) -> bool:
        return self.authority is not IlluminationAuthority.COUNTERFACTUAL


@dataclass(frozen=True)
class RestorationRequestV07:
    requested_role: str
    support_present: bool
    provenance_bound: bool
    retreatable: bool


@dataclass(frozen=True)
class OpenSceneRegionResultV07:
    schema: str
    region_id: str
    pass_contract: bool
    scene_state_sha256: str
    source_evidence_sha256: str
    scientific_master_sha256: str
    dynamic_authority_artifact_sha256: str
    channel_authorities: Tuple[str, str, str]
    hdr_channel_status: Tuple[str, str, str]
    colour_authority: str
    full_physical_colour_claim_allowed: bool
    illumination_authority: str
    captured_world_illumination_writeback_allowed: bool
    detail_status: str
    detail_scientific_writeback_allowed: bool
    restoration_allowed: Tuple[bool, bool, bool]
    restoration_authority_out: Tuple[str, str, str]
    restoration_scientific_writeback_allowed: bool
    creates_new_evidence: bool
    physical_frame_count: int
    independent_evidence_count: int
    claim_boundary: str


def _hdr_status(sample: DynamicAuthoritySample) -> HDRChannelStatus:
    sample.validate()
    if sample.authority in (DynamicAuthority.MEASURED, DynamicAuthority.CALIBRATED_ESTIMATE):
        return HDRChannelStatus.EVIDENCE_SUPPORTED_FINITE
    if sample.authority is DynamicAuthority.RECONSTRUCTED:
        return HDRChannelStatus.RECONSTRUCTED_FINITE_NOT_MEASURED
    if sample.authority is DynamicAuthority.CENSORED:
        return HDRChannelStatus.CENSORED_BOUND_ONLY
    if sample.authority is DynamicAuthority.UNKNOWN:
        return HDRChannelStatus.NO_SCIENTIFIC_HEADROOM
    return HDRChannelStatus.NON_SCIENTIFIC_VIEW_ONLY


def _detail_status(structure: RuntimeStructureResult) -> DetailStatus:
    if structure.status is RuntimeStructureStatus.BLOCKED_UNCERTAINTY_OUT_OF_DOMAIN:
        return DetailStatus.OUT_OF_DOMAIN
    if structure.status is RuntimeStructureStatus.ADMITTED_APPEARANCE_ONLY:
        return DetailStatus.APPEARANCE_DETAIL_ALLOWED
    return DetailStatus.NEUTRAL_OR_BLOCKED


def _restoration_source_authority(authority: DynamicAuthority) -> str:
    if authority is DynamicAuthority.MEASURED:
        return "MEASURED"
    if authority is DynamicAuthority.CALIBRATED_ESTIMATE:
        # Source-bound calibrated Stage-2 data is derived, not raw-code measurement.
        return "RECONSTRUCTED"
    if authority is DynamicAuthority.RECONSTRUCTED:
        return "RECONSTRUCTED"
    if authority is DynamicAuthority.CENSORED:
        return "CENSORED"
    if authority is DynamicAuthority.UNKNOWN:
        return "UNKNOWN"
    return "COUNTERFACTUAL"


def compose_open_scene_region_v07(
    *,
    bindings: SceneRegionBindingsV07,
    channels: Sequence[DynamicAuthoritySample],
    structure: RuntimeStructureResult,
    colour: ColourCalibrationBindingV07,
    illumination: IlluminationBindingV07,
    restoration: Optional[RestorationRequestV07] = None,
) -> OpenSceneRegionResultV07:
    """Compose one authority-bound region state without widening any claim."""
    bindings.validate()
    colour.validate()
    illumination.validate()
    if len(channels) != 3:
        raise ContractError("Open Scene RGB region requires exactly three channel authority samples")
    samples = tuple(s.validate() for s in channels)

    # Scientific scene input may not already contain view-only values.
    if any(s.authority in (DynamicAuthority.COUNTERFACTUAL, DynamicAuthority.APPEARANCE_ONLY) for s in samples):
        raise ContractError("counterfactual/appearance channel samples cannot enter captured-world Open Scene State")

    detail = _detail_status(structure)
    detail_writeback = False  # v0.4 structure contract admits detail/acutance appearance only.

    restoration_decisions = []
    if restoration is None:
        for s in samples:
            restoration_decisions.append({
                "allowed": True,
                "scientific_authority_out": _restoration_source_authority(s.authority),
                "scientific_writeback_allowed": False,
                "classification": "NO_RESTORATION_REQUESTED",
            })
    else:
        for s in samples:
            restoration_decisions.append(
                evaluate_site(
                    source_authority=_restoration_source_authority(s.authority),
                    source_valid=s.authority is not DynamicAuthority.UNKNOWN,
                    requested_role=restoration.requested_role,
                    support_present=restoration.support_present,
                    provenance_bound=restoration.provenance_bound,
                    retreatable=restoration.retreatable,
                )
            )

    hdr = tuple(_hdr_status(s) for s in samples)
    restoration_allowed = tuple(bool(d["allowed"]) for d in restoration_decisions)
    restoration_out = tuple(str(d["scientific_authority_out"]) for d in restoration_decisions)
    restoration_writeback = any(bool(d["scientific_writeback_allowed"]) for d in restoration_decisions)

    payload = {
        "schema": "TruthRawOpenSceneRegion/0.7",
        "region_id": bindings.region_id,
        "source_evidence_sha256": bindings.source_evidence_sha256.lower(),
        "scientific_master_sha256": bindings.scientific_master_sha256.lower(),
        "dynamic_authority_artifact_sha256": bindings.dynamic_authority_artifact_sha256.lower(),
        "physical_frame_count": bindings.physical_frame_count,
        "independent_evidence_count": bindings.independent_evidence_count,
        "channels": [
            {
                "authority": s.authority.value,
                "support": s.support,
                "has_scene_linear_estimate": s.scene_linear_estimate is not None,
                "has_uncertainty_p95": s.uncertainty_p95 is not None,
                "has_censor_bound": s.censor_bound is not None,
            }
            for s in samples
        ],
        "hdr_status": [x.value for x in hdr],
        "colour": {
            "authority": colour.authority.value,
            "full_physical_claim_allowed": colour.full_physical_colour_claim_allowed,
        },
        "illumination": {
            "authority": illumination.authority.value,
            "captured_world_writeback_allowed": illumination.captured_world_writeback_allowed,
        },
        "structure": {
            "status": structure.status.value,
            "binding_sha256": structure.binding_sha256,
            "detail_status": detail.value,
            "detail_scientific_writeback_allowed": detail_writeback,
        },
        "restoration": [
            {
                "allowed": bool(d["allowed"]),
                "scientific_authority_out": str(d["scientific_authority_out"]),
                "classification": str(d["classification"]),
                "scientific_writeback_allowed": bool(d["scientific_writeback_allowed"]),
            }
            for d in restoration_decisions
        ],
        "creates_new_evidence": False,
    }
    scene_sha = _canonical_hash(payload)
    all_restoration_allowed = all(restoration_allowed)
    pass_contract = all_restoration_allowed and not restoration_writeback

    return OpenSceneRegionResultV07(
        schema="TruthRawOpenSceneRegion/0.7",
        region_id=bindings.region_id,
        pass_contract=pass_contract,
        scene_state_sha256=scene_sha,
        source_evidence_sha256=bindings.source_evidence_sha256.lower(),
        scientific_master_sha256=bindings.scientific_master_sha256.lower(),
        dynamic_authority_artifact_sha256=bindings.dynamic_authority_artifact_sha256.lower(),
        channel_authorities=tuple(s.authority.value for s in samples),
        hdr_channel_status=tuple(x.value for x in hdr),
        colour_authority=colour.authority.value,
        full_physical_colour_claim_allowed=colour.full_physical_colour_claim_allowed,
        illumination_authority=illumination.authority.value,
        captured_world_illumination_writeback_allowed=illumination.captured_world_writeback_allowed,
        detail_status=detail.value,
        detail_scientific_writeback_allowed=detail_writeback,
        restoration_allowed=restoration_allowed,
        restoration_authority_out=restoration_out,
        restoration_scientific_writeback_allowed=restoration_writeback,
        creates_new_evidence=False,
        physical_frame_count=bindings.physical_frame_count,
        independent_evidence_count=bindings.independent_evidence_count,
        claim_boundary=(
            "v0.7 binds scene physics, colour, structure, HDR and restoration to the same source/master/Dynamic-Authority lineage. "
            "It does not create new photons/evidence, promote source metadata to independent calibration, turn optical/acutance appearance into measured detail, "
            "recover exact radiance from censoring, or permit counterfactual/restoration/appearance writeback into captured-world evidence."
        ),
    )
