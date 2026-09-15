#!/usr/bin/env python3
"""TruthRaw open-world integration contracts v0.2.

This module connects the v0.1 open-world foundations to concrete TruthRaw
execution domains without modifying the immutable source, canonical v4.7i/v4.7j
bytes, or counterfactual/appearance authority boundaries.

It provides four integration layers:

1. illumination dispatch to scientific vs CICM/Room Capsule consumers,
2. a fail-closed in-memory Calibration Registry resolver,
3. a Structure Evidence -> detail/acutance execution gate,
4. a hash-chained reversible restoration artefact stack.

The Camera-5 200 MP end-to-end proof bundle lives in
``camera5_200mp_evidence_bundle_v02.py``.
"""
from __future__ import annotations

import hashlib
import json
from dataclasses import dataclass, field
from enum import Enum
from typing import Iterable, Optional, Sequence, Tuple

from open_world_foundations_v01 import (
    Applicability,
    CalibrationKind,
    CalibrationRecord,
    CaptureConditions,
    ContractError,
    IlluminationAuthority,
    IlluminationRecord,
    RestorationClass,
    RestorationRecord,
    StructureEvidenceRecord,
    StructureSupportClass,
)


class IlluminationConsumer(str, Enum):
    SCIENTIFIC_SCENE = "SCIENTIFIC_SCENE"
    CICM_RELATIVE_WORLD = "CICM_RELATIVE_WORLD"
    CICM_CALIBRATED_FORWARD = "CICM_CALIBRATED_FORWARD"
    ROOM_CAPSULE_RELATIVE = "ROOM_CAPSULE_RELATIVE"


class OutputAuthority(str, Enum):
    MEASURED = "MEASURED"
    CALIBRATED_ESTIMATE = "CALIBRATED_ESTIMATE"
    INFERRED = "INFERRED"
    COUNTERFACTUAL = "COUNTERFACTUAL"
    APPEARANCE_ONLY = "APPEARANCE_ONLY"


@dataclass(frozen=True)
class IlluminationDispatchDecision:
    allowed: bool
    consumer: IlluminationConsumer
    input_authority: IlluminationAuthority
    output_authority: OutputAuthority
    may_modify_scientific_master: bool
    calibration_required: bool
    reason: str


def dispatch_illumination(
    record: IlluminationRecord,
    consumer: IlluminationConsumer,
    *,
    calibration_applicability: Optional[Applicability] = None,
) -> IlluminationDispatchDecision:
    """Map one validated illumination claim to a TruthRaw consumer.

    CICM and Room Capsule create hypothetical/counterfactual observations even
    when their reference light was physically measured. Their output therefore
    remains COUNTERFACTUAL and can never write back into the scientific master.
    """
    record.validate()

    if consumer is IlluminationConsumer.SCIENTIFIC_SCENE:
        if record.authority is IlluminationAuthority.COUNTERFACTUAL:
            return IlluminationDispatchDecision(
                False,
                consumer,
                record.authority,
                OutputAuthority.COUNTERFACTUAL,
                False,
                False,
                "counterfactual illumination cannot be admitted as scientific-scene evidence",
            )
        mapping = {
            IlluminationAuthority.MEASURED: OutputAuthority.MEASURED,
            IlluminationAuthority.CALIBRATED_ESTIMATE: OutputAuthority.CALIBRATED_ESTIMATE,
            IlluminationAuthority.INFERRED: OutputAuthority.INFERRED,
        }
        return IlluminationDispatchDecision(
            True,
            consumer,
            record.authority,
            mapping[record.authority],
            True,
            record.authority is IlluminationAuthority.CALIBRATED_ESTIMATE,
            "authority preserved; no upgrade above the input illumination claim",
        )

    if consumer is IlluminationConsumer.CICM_CALIBRATED_FORWARD:
        if calibration_applicability is not Applicability.APPLICABLE:
            return IlluminationDispatchDecision(
                False,
                consumer,
                record.authority,
                OutputAuthority.COUNTERFACTUAL,
                False,
                True,
                "calibrated CICM forward prediction requires one applicable certified calibration",
            )
        return IlluminationDispatchDecision(
            True,
            consumer,
            record.authority,
            OutputAuthority.COUNTERFACTUAL,
            False,
            True,
            "calibration is applicable; predicted capture remains counterfactual, never evidence",
        )

    if consumer in (
        IlluminationConsumer.CICM_RELATIVE_WORLD,
        IlluminationConsumer.ROOM_CAPSULE_RELATIVE,
    ):
        return IlluminationDispatchDecision(
            True,
            consumer,
            record.authority,
            OutputAuthority.COUNTERFACTUAL,
            False,
            False,
            "relative relighting is permitted only as a counterfactual/appearance-domain result",
        )

    raise ContractError(f"unsupported illumination consumer: {consumer}")


class CalibrationResolutionStatus(str, Enum):
    RESOLVED = "RESOLVED"
    NOT_FOUND = "NOT_FOUND"
    OUT_OF_DOMAIN = "OUT_OF_DOMAIN"
    UNCERTIFIED = "UNCERTIFIED"
    AMBIGUOUS = "AMBIGUOUS"


@dataclass(frozen=True)
class CalibrationResolution:
    status: CalibrationResolutionStatus
    record: Optional[CalibrationRecord] = None
    candidate_ids: Tuple[str, ...] = field(default_factory=tuple)
    reason: str = ""


class CalibrationRegistry:
    """Fail-closed resolver for immutable calibration records.

    v0.2 deliberately refuses to auto-rank two simultaneously applicable
    calibrations. Such a situation must be resolved explicitly rather than by
    insertion order or newest-file-wins behaviour.
    """

    def __init__(self, records: Iterable[CalibrationRecord] = ()) -> None:
        self._records = tuple(records)
        ids = set()
        for r in self._records:
            r.validate()
            if r.calibration_id in ids:
                raise ContractError(f"duplicate calibration_id: {r.calibration_id}")
            ids.add(r.calibration_id)

    @property
    def records(self) -> Tuple[CalibrationRecord, ...]:
        return self._records

    def resolve(
        self,
        kind: CalibrationKind,
        capture: CaptureConditions,
        *,
        require_holdout: bool = True,
        preferred_calibration_id: Optional[str] = None,
    ) -> CalibrationResolution:
        typed = [r for r in self._records if r.kind is kind]
        if preferred_calibration_id is not None:
            typed = [r for r in typed if r.calibration_id == preferred_calibration_id]
            if not typed:
                return CalibrationResolution(
                    CalibrationResolutionStatus.NOT_FOUND,
                    reason="preferred calibration id does not exist for requested kind",
                )
        if not typed:
            return CalibrationResolution(
                CalibrationResolutionStatus.NOT_FOUND,
                reason="no calibration record exists for requested kind",
            )

        applicable = []
        uncertified = []
        out_of_domain = []
        for r in typed:
            state = r.applicability(capture, require_holdout=require_holdout)
            if state is Applicability.APPLICABLE:
                applicable.append(r)
            elif state is Applicability.UNCERTIFIED:
                uncertified.append(r)
            else:
                out_of_domain.append(r)

        if len(applicable) == 1:
            r = applicable[0]
            return CalibrationResolution(
                CalibrationResolutionStatus.RESOLVED,
                record=r,
                candidate_ids=(r.calibration_id,),
                reason="one and only one certified calibration is applicable",
            )
        if len(applicable) > 1:
            return CalibrationResolution(
                CalibrationResolutionStatus.AMBIGUOUS,
                candidate_ids=tuple(sorted(r.calibration_id for r in applicable)),
                reason="multiple certified calibrations overlap; explicit selection is required",
            )
        if uncertified:
            return CalibrationResolution(
                CalibrationResolutionStatus.UNCERTIFIED,
                candidate_ids=tuple(sorted(r.calibration_id for r in uncertified)),
                reason="matching domain exists but independent hold-out is not passed",
            )
        return CalibrationResolution(
            CalibrationResolutionStatus.OUT_OF_DOMAIN,
            candidate_ids=tuple(sorted(r.calibration_id for r in out_of_domain)),
            reason="records exist but none admit these capture conditions",
        )

    def fingerprint(self) -> str:
        payload = []
        for r in sorted(self._records, key=lambda x: x.calibration_id):
            d = r.domain
            payload.append(
                {
                    "calibration_id": r.calibration_id,
                    "kind": r.kind.value,
                    "domain": {
                        "device": d.device,
                        "physical_camera_id": d.physical_camera_id,
                        "sensor_pixel_mode": d.sensor_pixel_mode,
                        "width": d.width,
                        "height": d.height,
                        "cfa": d.cfa,
                        "iso": [d.iso_min, d.iso_max],
                        "exposure_s": [d.exposure_s_min, d.exposure_s_max],
                        "temperature_c": [d.temperature_c_min, d.temperature_c_max],
                        "focus_distance_diopters": [
                            d.focus_distance_diopters_min,
                            d.focus_distance_diopters_max,
                        ],
                    },
                    "source_evidence_sha256": list(r.source_evidence_sha256),
                    "method": r.method,
                    "uncertainty_description": r.uncertainty_description,
                    "holdout_status": r.holdout_status.value,
                    "holdout_report_sha256": r.holdout_report_sha256,
                }
            )
        blob = json.dumps(payload, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("utf-8")
        return hashlib.sha256(blob).hexdigest()


class DetailExecutionMode(str, Enum):
    ADAPTIVE_APPEARANCE = "ADAPTIVE_APPEARANCE"
    NEUTRAL_ONLY = "NEUTRAL_ONLY"


@dataclass(frozen=True)
class DetailAuthorityDecision:
    support_class: StructureSupportClass
    detail_v47j_mode: DetailExecutionMode
    output_acutance_v47k_mode: DetailExecutionMode
    scientific_master_write_allowed: bool
    measured_detail_claim_allowed_from_output: bool
    provenance_label: str
    reason: str


def gate_detail_and_acutance(structure: StructureEvidenceRecord) -> DetailAuthorityDecision:
    """Connect Structure Evidence to v4.7j/v4.7k without changing canon bytes.

    The gate decides whether adaptive appearance processing may run. It never
    turns the appearance output into scientific-master data. Measured support
    describes the *input evidence*, not the authority of the sharpened output.
    """
    cls = structure.support_class()
    if cls is StructureSupportClass.MEASURED_SUPPORTED:
        return DetailAuthorityDecision(
            cls,
            DetailExecutionMode.ADAPTIVE_APPEARANCE,
            DetailExecutionMode.ADAPTIVE_APPEARANCE,
            False,
            False,
            "appearance_from_measured_supported_structure",
            "measured structure supports adaptive appearance, but the transformed pixels remain appearance",
        )
    if cls is StructureSupportClass.RECONSTRUCTED_SUPPORTED:
        return DetailAuthorityDecision(
            cls,
            DetailExecutionMode.ADAPTIVE_APPEARANCE,
            DetailExecutionMode.ADAPTIVE_APPEARANCE,
            False,
            False,
            "appearance_from_reconstructed_supported_structure",
            "reconstructed structure may be rendered, while provenance must preserve reconstructed authority",
        )
    return DetailAuthorityDecision(
        cls,
        DetailExecutionMode.NEUTRAL_ONLY,
        DetailExecutionMode.NEUTRAL_ONLY,
        False,
        False,
        "neutral_due_to_weak_or_unknown_structure",
        "weak/censored/unknown structure fails closed to neutral appearance rather than amplifying unsupported detail",
    )


def canonical_detail_profile(decision: DetailAuthorityDecision) -> str:
    return "AdaptiveDetailedCrisp" if decision.detail_v47j_mode is DetailExecutionMode.ADAPTIVE_APPEARANCE else "NeutralReference"


def canonical_output_acutance_profile(decision: DetailAuthorityDecision) -> str:
    return "AdaptiveDetail" if decision.output_acutance_v47k_mode is DetailExecutionMode.ADAPTIVE_APPEARANCE else "Neutral"


_SHA256_HEX = set("0123456789abcdef")


def _check_sha256(value: str, field_name: str) -> None:
    v = value.lower()
    if len(v) != 64 or any(ch not in _SHA256_HEX for ch in v):
        raise ContractError(f"{field_name} must be a 64-character hexadecimal SHA-256")


def _canonical_hash(payload: dict) -> str:
    b = json.dumps(payload, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("utf-8")
    return hashlib.sha256(b).hexdigest()


@dataclass(frozen=True)
class RestorationLayerArtifact:
    layer_id: str
    restoration_class: RestorationClass
    root_source_sha256: str
    parent_layer_sha256: Optional[str]
    mask_sha256: str
    payload_sha256: str
    record_evidence_sha256: Tuple[str, ...]
    method: str
    confidence: Optional[float]
    hypothesis_description: Optional[str]
    layer_sha256: str


class RestorationStack:
    """Hash-chained reversible restoration stack.

    The stack is intentionally linear in v0.2. Once a hypothetical/appearance
    layer is present, a later layer may not re-enter evidence-supported
    restoration authority. A future branching DAG may permit returning to an
    earlier clean scientific parent explicitly.
    """

    def __init__(self, root_source_sha256: str) -> None:
        _check_sha256(root_source_sha256, "root_source_sha256")
        self.root_source_sha256 = root_source_sha256.lower()
        self._layers: list[RestorationLayerArtifact] = []

    @property
    def layers(self) -> Tuple[RestorationLayerArtifact, ...]:
        return tuple(self._layers)

    def _has_visual_taint(self) -> bool:
        return any(
            x.restoration_class in (
                RestorationClass.HYPOTHETICAL_VISUAL_RESTORATION,
                RestorationClass.APPEARANCE_ONLY,
            )
            for x in self._layers
        )

    def append(self, record: RestorationRecord, payload_sha256: str) -> RestorationLayerArtifact:
        record.validate()
        _check_sha256(payload_sha256, "restoration payload_sha256")
        if self.root_source_sha256 not in tuple(h.lower() for h in record.source_evidence_sha256):
            raise ContractError("restoration record must remain bound to the root source evidence hash")
        if self._has_visual_taint() and record.restoration_class in (
            RestorationClass.OBSERVED_LOSS,
            RestorationClass.EVIDENCE_SUPPORTED_RECONSTRUCTION,
        ):
            raise ContractError(
                "linear restoration stack cannot upgrade back to evidence-supported authority after hypothetical/appearance content"
            )

        parent = self._layers[-1].layer_sha256 if self._layers else None
        payload = {
            "schema": "TruthRawRestorationLayerArtifact/0.2",
            "layer_id": record.layer_id,
            "restoration_class": record.restoration_class.value,
            "root_source_sha256": self.root_source_sha256,
            "parent_layer_sha256": parent,
            "mask_sha256": record.mask_sha256.lower(),
            "payload_sha256": payload_sha256.lower(),
            "record_evidence_sha256": [h.lower() for h in record.source_evidence_sha256],
            "method": record.method,
            "confidence": record.confidence,
            "hypothesis_description": record.hypothesis_description,
        }
        layer_hash = _canonical_hash(payload)
        artifact = RestorationLayerArtifact(
            layer_id=record.layer_id,
            restoration_class=record.restoration_class,
            root_source_sha256=self.root_source_sha256,
            parent_layer_sha256=parent,
            mask_sha256=record.mask_sha256.lower(),
            payload_sha256=payload_sha256.lower(),
            record_evidence_sha256=tuple(h.lower() for h in record.source_evidence_sha256),
            method=record.method,
            confidence=record.confidence,
            hypothesis_description=record.hypothesis_description,
            layer_sha256=layer_hash,
        )
        self._layers.append(artifact)
        return artifact

    def verify(self) -> bool:
        visual_taint = False
        expected_parent: Optional[str] = None
        for x in self._layers:
            if x.root_source_sha256 != self.root_source_sha256:
                return False
            if x.parent_layer_sha256 != expected_parent:
                return False
            if visual_taint and x.restoration_class in (
                RestorationClass.OBSERVED_LOSS,
                RestorationClass.EVIDENCE_SUPPORTED_RECONSTRUCTION,
            ):
                return False
            payload = {
                "schema": "TruthRawRestorationLayerArtifact/0.2",
                "layer_id": x.layer_id,
                "restoration_class": x.restoration_class.value,
                "root_source_sha256": x.root_source_sha256,
                "parent_layer_sha256": x.parent_layer_sha256,
                "mask_sha256": x.mask_sha256,
                "payload_sha256": x.payload_sha256,
                "record_evidence_sha256": list(x.record_evidence_sha256),
                "method": x.method,
                "confidence": x.confidence,
                "hypothesis_description": x.hypothesis_description,
            }
            if _canonical_hash(payload) != x.layer_sha256:
                return False
            expected_parent = x.layer_sha256
            if x.restoration_class in (
                RestorationClass.HYPOTHETICAL_VISUAL_RESTORATION,
                RestorationClass.APPEARANCE_ONLY,
            ):
                visual_taint = True
        return True

    def manifest(self) -> dict:
        if not self.verify():
            raise ContractError("restoration stack failed chain verification")
        return {
            "schema": "TruthRawRestorationStack/0.2",
            "root_source_sha256": self.root_source_sha256,
            "layer_count": len(self._layers),
            "head_layer_sha256": self._layers[-1].layer_sha256 if self._layers else None,
            "source_overwritten": False,
            "linear_visual_taint": self._has_visual_taint(),
            "layers": [
                {
                    "layer_id": x.layer_id,
                    "restoration_class": x.restoration_class.value,
                    "parent_layer_sha256": x.parent_layer_sha256,
                    "mask_sha256": x.mask_sha256,
                    "payload_sha256": x.payload_sha256,
                    "layer_sha256": x.layer_sha256,
                }
                for x in self._layers
            ],
        }
