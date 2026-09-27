#!/usr/bin/env python3
"""D.RAW calibration identity and observation-binding contract v0.1.

This module does not create calibration. It binds an existing calibration
record, its validity domain, payload and hold-out report to one observation
capture context with deterministic identities.

It deliberately extends (rather than mutates) the existing
open_world_foundations_v01 CalibrationDomain / CalibrationRecord contracts.
"""
from __future__ import annotations

from dataclasses import dataclass
import hashlib
import json
from typing import Optional

from open_world_foundations_v01 import (
    Applicability,
    CalibrationDomain,
    CalibrationRecord,
    CaptureConditions,
    ContractError,
    HoldoutStatus,
)

_HEX = set("0123456789abcdef")


def _sha256(value: str, name: str) -> str:
    v = value.lower()
    if len(v) != 64 or any(ch not in _HEX for ch in v):
        raise ContractError(f"{name} must be 64 lowercase/uppercase hex characters")
    return v


def _nonempty(value: str, name: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise ContractError(f"{name} may not be empty")
    return value


def _float_hex(value: Optional[float]) -> Optional[str]:
    return None if value is None else float(value).hex()


def _canonical_sha256(payload: dict) -> str:
    encoded = json.dumps(
        payload,
        sort_keys=True,
        separators=(",", ":"),
        ensure_ascii=False,
        allow_nan=False,
    ).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


@dataclass(frozen=True)
class BindingDomain:
    """Calibration validity domain plus D.RAW source/readout identity.

    The historical CalibrationDomain already gates device, physical camera,
    pixel mode, raster/CFA and numeric ranges. D.RAW adds explicit source,
    sample and readout domain IDs so superficially similar rasters cannot
    silently share a calibration.
    """

    calibration_domain: CalibrationDomain
    source_route_id: str
    sample_domain_id: str
    readout_domain_id: str

    def validate(self) -> "BindingDomain":
        self.calibration_domain.validate()
        _nonempty(self.source_route_id, "source_route_id")
        _nonempty(self.sample_domain_id, "sample_domain_id")
        _nonempty(self.readout_domain_id, "readout_domain_id")
        return self

    def canonical_payload(self) -> dict:
        self.validate()
        d = self.calibration_domain
        return {
            "schema": "D.RAW/CalibrationBindingDomain/0.1",
            "device": d.device,
            "physical_camera_id": d.physical_camera_id,
            "sensor_pixel_mode": d.sensor_pixel_mode,
            "width": d.width,
            "height": d.height,
            "cfa": d.cfa,
            "source_route_id": self.source_route_id,
            "sample_domain_id": self.sample_domain_id,
            "readout_domain_id": self.readout_domain_id,
            "iso_min_hex": _float_hex(d.iso_min),
            "iso_max_hex": _float_hex(d.iso_max),
            "exposure_s_min_hex": _float_hex(d.exposure_s_min),
            "exposure_s_max_hex": _float_hex(d.exposure_s_max),
            "temperature_c_min_hex": _float_hex(d.temperature_c_min),
            "temperature_c_max_hex": _float_hex(d.temperature_c_max),
            "focus_distance_diopters_min_hex": _float_hex(
                d.focus_distance_diopters_min
            ),
            "focus_distance_diopters_max_hex": _float_hex(
                d.focus_distance_diopters_max
            ),
        }

    def sha256(self) -> str:
        return _canonical_sha256(self.canonical_payload())


@dataclass(frozen=True)
class BoundCalibration:
    record: CalibrationRecord
    binding_domain: BindingDomain
    calibration_payload_sha256: str

    def validate(self) -> "BoundCalibration":
        self.record.validate()
        self.binding_domain.validate()
        _sha256(self.calibration_payload_sha256, "calibration_payload_sha256")

        # One record may not claim a different foundation domain than the one
        # whose D.RAW binding identity is being hashed.
        if self.record.domain != self.binding_domain.calibration_domain:
            raise ContractError("record.domain must equal binding_domain.calibration_domain")
        return self

    def record_payload(self) -> dict:
        self.validate()
        evidence = tuple(sorted(self.record.source_evidence_sha256))
        if len(evidence) != len(set(evidence)):
            raise ContractError("calibration source evidence hashes must be unique")
        return {
            "schema": "D.RAW/BoundCalibrationRecord/0.1",
            "calibration_id": self.record.calibration_id,
            "kind": self.record.kind.value,
            "binding_domain_sha256": self.binding_domain.sha256(),
            "source_evidence_sha256": list(evidence),
            "method": self.record.method,
            "uncertainty_description": self.record.uncertainty_description,
            "calibration_payload_sha256": _sha256(
                self.calibration_payload_sha256,
                "calibration_payload_sha256",
            ),
            "holdout_status": self.record.holdout_status.value,
            "holdout_report_sha256": (
                None
                if self.record.holdout_report_sha256 is None
                else _sha256(
                    self.record.holdout_report_sha256,
                    "holdout_report_sha256",
                )
            ),
        }

    def record_sha256(self) -> str:
        return _canonical_sha256(self.record_payload())


@dataclass(frozen=True)
class CaptureBindingContext:
    observation_id: str
    source_evidence_sha256: str
    capture: CaptureConditions
    source_route_id: str
    sample_domain_id: str
    readout_domain_id: str

    def validate(self) -> "CaptureBindingContext":
        _nonempty(self.observation_id, "observation_id")
        _sha256(self.source_evidence_sha256, "source_evidence_sha256")
        _nonempty(self.capture.device, "capture.device")
        _nonempty(self.capture.physical_camera_id, "capture.physical_camera_id")
        _nonempty(self.capture.sensor_pixel_mode, "capture.sensor_pixel_mode")
        if self.capture.width <= 0 or self.capture.height <= 0:
            raise ContractError("capture dimensions must be positive")
        _nonempty(self.capture.cfa, "capture.cfa")
        _nonempty(self.source_route_id, "capture source_route_id")
        _nonempty(self.sample_domain_id, "capture sample_domain_id")
        _nonempty(self.readout_domain_id, "capture readout_domain_id")
        return self

    def canonical_payload(self) -> dict:
        self.validate()
        c = self.capture
        return {
            "schema": "D.RAW/CalibrationCaptureContext/0.1",
            "observation_id": self.observation_id,
            "source_evidence_sha256": self.source_evidence_sha256.lower(),
            "device": c.device,
            "physical_camera_id": c.physical_camera_id,
            "sensor_pixel_mode": c.sensor_pixel_mode,
            "width": c.width,
            "height": c.height,
            "cfa": c.cfa,
            "source_route_id": self.source_route_id,
            "sample_domain_id": self.sample_domain_id,
            "readout_domain_id": self.readout_domain_id,
            "iso_hex": _float_hex(c.iso),
            "exposure_s_hex": _float_hex(c.exposure_s),
            "temperature_c_hex": _float_hex(c.temperature_c),
            "focus_distance_diopters_hex": _float_hex(
                c.focus_distance_diopters
            ),
        }

    def sha256(self) -> str:
        return _canonical_sha256(self.canonical_payload())


@dataclass(frozen=True)
class CalibrationBindingDecision:
    applicability: Applicability
    admitted: bool
    observation_id: str
    source_evidence_sha256: str
    calibration_record_sha256: str
    validity_domain_sha256: str
    calibration_payload_sha256: str
    holdout_report_sha256: Optional[str]
    capture_context_sha256: str
    binding_sha256: str
    creates_new_evidence: bool = False
    source_evidence_mutated: bool = False
    cross_observation_relation_granted: bool = False
    calibration_transfer_implied: bool = False
    physical_frame_count_increment: int = 0
    independent_evidence_count_increment: int = 0


def evaluate_binding(
    calibration: BoundCalibration,
    capture: CaptureBindingContext,
) -> CalibrationBindingDecision:
    calibration.validate()
    capture.validate()

    d = calibration.binding_domain
    c = capture
    identity_match = (
        c.source_route_id == d.source_route_id
        and c.sample_domain_id == d.sample_domain_id
        and c.readout_domain_id == d.readout_domain_id
    )

    if identity_match:
        applicability = calibration.record.applicability(
            capture.capture,
            require_holdout=True,
        )
    else:
        applicability = Applicability.OUT_OF_DOMAIN

    admitted = applicability is Applicability.APPLICABLE

    record_sha = calibration.record_sha256()
    domain_sha = d.sha256()
    context_sha = c.sha256()
    holdout = calibration.record.holdout_report_sha256
    binding_payload = {
        "schema": "D.RAW/CalibrationObservationBinding/0.1",
        "observation_id": c.observation_id,
        "source_evidence_sha256": c.source_evidence_sha256.lower(),
        "calibration_record_sha256": record_sha,
        "validity_domain_sha256": domain_sha,
        "calibration_payload_sha256": calibration.calibration_payload_sha256.lower(),
        "holdout_report_sha256": (
            None if holdout is None else holdout.lower()
        ),
        "capture_context_sha256": context_sha,
        "applicability": applicability.value,
        "admitted": admitted,
        "creates_new_evidence": False,
        "source_evidence_mutated": False,
        "cross_observation_relation_granted": False,
        "calibration_transfer_implied": False,
        "physical_frame_count_increment": 0,
        "independent_evidence_count_increment": 0,
    }

    return CalibrationBindingDecision(
        applicability=applicability,
        admitted=admitted,
        observation_id=c.observation_id,
        source_evidence_sha256=c.source_evidence_sha256.lower(),
        calibration_record_sha256=record_sha,
        validity_domain_sha256=domain_sha,
        calibration_payload_sha256=calibration.calibration_payload_sha256.lower(),
        holdout_report_sha256=None if holdout is None else holdout.lower(),
        capture_context_sha256=context_sha,
        binding_sha256=_canonical_sha256(binding_payload),
    )
