#!/usr/bin/env python3
"""TruthRaw open-world scientific foundation contracts v0.1.

This module is deliberately small and dependency-free. It does not perform
image reconstruction. It defines machine-checkable contracts for four
cross-cutting TruthRaw domains that must remain separate from source evidence:

1. illumination authority,
2. calibration applicability,
3. structure/detail evidence,
4. conservation/restoration layers.

The fifth open-world foundation (Camera 5 200 MP CFA identity) lives in
``camera5_200mp_cfa_identity_v01.py`` because it operates on large binary
capture artefacts.

Scientific rule: representation may be open-ended; evidence authority may not
silently increase.
"""
from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum
from math import isfinite
from typing import Iterable, Optional, Tuple


class ContractError(ValueError):
    """Raised when a record would violate TruthRaw scientific boundaries."""


class IlluminationAuthority(str, Enum):
    MEASURED = "MEASURED"
    CALIBRATED_ESTIMATE = "CALIBRATED_ESTIMATE"
    INFERRED = "INFERRED"
    COUNTERFACTUAL = "COUNTERFACTUAL"


class CalibrationKind(str, Enum):
    BLACK_OFFSET = "BLACK_OFFSET"
    LINEARITY = "LINEARITY"
    GAIN_NOISE = "GAIN_NOISE"
    PRNU = "PRNU"
    LENS_SHADING = "LENS_SHADING"
    OPTICS_PSF_MTF = "OPTICS_PSF_MTF"
    COLOR = "COLOR"
    ILLUMINANT = "ILLUMINANT"
    SPECTRAL_RESPONSE = "SPECTRAL_RESPONSE"


class HoldoutStatus(str, Enum):
    UNTESTED = "UNTESTED"
    PASS = "PASS"
    FAIL = "FAIL"


class Applicability(str, Enum):
    APPLICABLE = "APPLICABLE"
    OUT_OF_DOMAIN = "OUT_OF_DOMAIN"
    UNCERTIFIED = "UNCERTIFIED"


class StructureSupportClass(str, Enum):
    MEASURED_SUPPORTED = "MEASURED_SUPPORTED"
    RECONSTRUCTED_SUPPORTED = "RECONSTRUCTED_SUPPORTED"
    CENSORED_OR_WEAK = "CENSORED_OR_WEAK"
    UNKNOWN = "UNKNOWN"


class RestorationClass(str, Enum):
    OBSERVED_LOSS = "OBSERVED_LOSS"
    EVIDENCE_SUPPORTED_RECONSTRUCTION = "EVIDENCE_SUPPORTED_RECONSTRUCTION"
    HYPOTHETICAL_VISUAL_RESTORATION = "HYPOTHETICAL_VISUAL_RESTORATION"
    APPEARANCE_ONLY = "APPEARANCE_ONLY"


_SHA256_HEX = set("0123456789abcdef")


def _check_sha256(value: str, field_name: str = "sha256") -> None:
    v = value.lower()
    if len(v) != 64 or any(ch not in _SHA256_HEX for ch in v):
        raise ContractError(f"{field_name} must be a 64-character hexadecimal SHA-256")


def _check_unit_interval(value: float, field_name: str) -> None:
    if not isfinite(value) or not (0.0 <= value <= 1.0):
        raise ContractError(f"{field_name} must be finite and within [0, 1]")


def _check_range(lo: Optional[float], hi: Optional[float], field_name: str) -> None:
    if lo is not None and not isfinite(lo):
        raise ContractError(f"{field_name}.min must be finite when present")
    if hi is not None and not isfinite(hi):
        raise ContractError(f"{field_name}.max must be finite when present")
    if lo is not None and hi is not None and lo > hi:
        raise ContractError(f"{field_name}.min may not exceed .max")


def _inside(value: Optional[float], lo: Optional[float], hi: Optional[float]) -> bool:
    if value is None:
        return lo is None and hi is None
    if lo is not None and value < lo:
        return False
    if hi is not None and value > hi:
        return False
    return True


@dataclass(frozen=True)
class IlluminationRecord:
    """Authority-typed illumination state for an open scene.

    ``spatial_scope`` is descriptive only. It may be a tile, object, camera
    frustum, room, street, landscape region, sky dome, or another open-world
    domain. The contract intentionally has no sealed-room/world-size limit.
    """

    record_id: str
    authority: IlluminationAuthority
    spatial_scope: str
    evidence_sha256: Tuple[str, ...] = field(default_factory=tuple)
    calibration_id: Optional[str] = None
    inference_method: Optional[str] = None
    counterfactual_parent_id: Optional[str] = None
    spectral_description: Optional[str] = None
    relative_uncertainty: Optional[float] = None

    def validate(self) -> "IlluminationRecord":
        if not self.record_id.strip():
            raise ContractError("illumination record_id may not be empty")
        if not self.spatial_scope.strip():
            raise ContractError("illumination spatial_scope may not be empty")
        for h in self.evidence_sha256:
            _check_sha256(h, "illumination evidence_sha256")
        if self.relative_uncertainty is not None:
            if not isfinite(self.relative_uncertainty) or self.relative_uncertainty < 0:
                raise ContractError("relative_uncertainty must be finite and >= 0")

        if self.authority is IlluminationAuthority.MEASURED:
            if not self.evidence_sha256:
                raise ContractError("MEASURED illumination requires admitted evidence hashes")
            if self.counterfactual_parent_id is not None:
                raise ContractError("MEASURED illumination cannot descend from a counterfactual state")
        elif self.authority is IlluminationAuthority.CALIBRATED_ESTIMATE:
            if not self.evidence_sha256 or not self.calibration_id:
                raise ContractError("CALIBRATED_ESTIMATE requires evidence and calibration_id")
        elif self.authority is IlluminationAuthority.INFERRED:
            if not self.evidence_sha256 or not self.inference_method:
                raise ContractError("INFERRED illumination requires evidence and inference_method")
        elif self.authority is IlluminationAuthority.COUNTERFACTUAL:
            if not self.counterfactual_parent_id:
                raise ContractError("COUNTERFACTUAL illumination requires counterfactual_parent_id")
        return self


@dataclass(frozen=True)
class CalibrationDomain:
    """Validity domain for one physical calibration record."""

    device: str
    physical_camera_id: str
    sensor_pixel_mode: str
    width: int
    height: int
    cfa: str
    iso_min: Optional[float] = None
    iso_max: Optional[float] = None
    exposure_s_min: Optional[float] = None
    exposure_s_max: Optional[float] = None
    temperature_c_min: Optional[float] = None
    temperature_c_max: Optional[float] = None
    focus_distance_diopters_min: Optional[float] = None
    focus_distance_diopters_max: Optional[float] = None

    def validate(self) -> "CalibrationDomain":
        if not self.device or not self.physical_camera_id or not self.sensor_pixel_mode:
            raise ContractError("device, physical_camera_id and sensor_pixel_mode are required")
        if self.width <= 0 or self.height <= 0:
            raise ContractError("calibration dimensions must be positive")
        if not self.cfa:
            raise ContractError("calibration CFA must be explicit")
        _check_range(self.iso_min, self.iso_max, "iso")
        _check_range(self.exposure_s_min, self.exposure_s_max, "exposure_s")
        _check_range(self.temperature_c_min, self.temperature_c_max, "temperature_c")
        _check_range(
            self.focus_distance_diopters_min,
            self.focus_distance_diopters_max,
            "focus_distance_diopters",
        )
        return self


@dataclass(frozen=True)
class CaptureConditions:
    device: str
    physical_camera_id: str
    sensor_pixel_mode: str
    width: int
    height: int
    cfa: str
    iso: Optional[float] = None
    exposure_s: Optional[float] = None
    temperature_c: Optional[float] = None
    focus_distance_diopters: Optional[float] = None


@dataclass(frozen=True)
class CalibrationRecord:
    calibration_id: str
    kind: CalibrationKind
    domain: CalibrationDomain
    source_evidence_sha256: Tuple[str, ...]
    method: str
    uncertainty_description: str
    holdout_status: HoldoutStatus = HoldoutStatus.UNTESTED
    holdout_report_sha256: Optional[str] = None

    def validate(self) -> "CalibrationRecord":
        if not self.calibration_id.strip():
            raise ContractError("calibration_id may not be empty")
        self.domain.validate()
        if not self.source_evidence_sha256:
            raise ContractError("calibration requires immutable source evidence hashes")
        for h in self.source_evidence_sha256:
            _check_sha256(h, "calibration source_evidence_sha256")
        if not self.method.strip() or not self.uncertainty_description.strip():
            raise ContractError("calibration method and uncertainty_description are required")
        if self.holdout_report_sha256 is not None:
            _check_sha256(self.holdout_report_sha256, "holdout_report_sha256")
        if self.holdout_status is HoldoutStatus.PASS and self.holdout_report_sha256 is None:
            raise ContractError("holdout PASS requires a hashed holdout report")
        return self

    def applicability(self, capture: CaptureConditions, require_holdout: bool = True) -> Applicability:
        self.validate()
        d = self.domain
        identity_match = (
            capture.device == d.device
            and capture.physical_camera_id == d.physical_camera_id
            and capture.sensor_pixel_mode == d.sensor_pixel_mode
            and capture.width == d.width
            and capture.height == d.height
            and capture.cfa == d.cfa
        )
        ranges_match = (
            _inside(capture.iso, d.iso_min, d.iso_max)
            and _inside(capture.exposure_s, d.exposure_s_min, d.exposure_s_max)
            and _inside(capture.temperature_c, d.temperature_c_min, d.temperature_c_max)
            and _inside(
                capture.focus_distance_diopters,
                d.focus_distance_diopters_min,
                d.focus_distance_diopters_max,
            )
        )
        if not identity_match or not ranges_match:
            return Applicability.OUT_OF_DOMAIN
        if require_holdout and self.holdout_status is not HoldoutStatus.PASS:
            return Applicability.UNCERTIFIED
        return Applicability.APPLICABLE


@dataclass(frozen=True)
class StructureEvidenceRecord:
    """Per-region structural evidence without inventing a sharpening policy."""

    region_id: str
    measured_support: float
    reconstructed_support: float
    mtf_support: float
    censoring_risk: float
    uncertainty: float
    orientation_radians: Optional[float] = None
    spatial_frequency_cyc_per_px: Optional[float] = None
    evidence_sha256: Tuple[str, ...] = field(default_factory=tuple)

    def validate(self) -> "StructureEvidenceRecord":
        if not self.region_id.strip():
            raise ContractError("structure region_id may not be empty")
        for name in ("measured_support", "reconstructed_support", "mtf_support", "censoring_risk"):
            _check_unit_interval(getattr(self, name), name)
        if not isfinite(self.uncertainty) or self.uncertainty < 0:
            raise ContractError("structure uncertainty must be finite and >= 0")
        if self.orientation_radians is not None and not isfinite(self.orientation_radians):
            raise ContractError("orientation_radians must be finite")
        if self.spatial_frequency_cyc_per_px is not None:
            if not isfinite(self.spatial_frequency_cyc_per_px) or self.spatial_frequency_cyc_per_px < 0:
                raise ContractError("spatial_frequency_cyc_per_px must be finite and >= 0")
        for h in self.evidence_sha256:
            _check_sha256(h, "structure evidence_sha256")
        return self

    def support_class(self) -> StructureSupportClass:
        """Return only an epistemic class; do not synthesize detail gain.

        Thresholds are deliberately limited to exact boundary conditions so the
        contract does not smuggle in an uncalibrated sharpening aesthetic.
        """
        self.validate()
        if self.censoring_risk >= 1.0:
            return StructureSupportClass.CENSORED_OR_WEAK
        if self.measured_support > 0.0 and self.mtf_support > 0.0:
            return StructureSupportClass.MEASURED_SUPPORTED
        if self.reconstructed_support > 0.0:
            return StructureSupportClass.RECONSTRUCTED_SUPPORTED
        if self.uncertainty > 0.0 or self.censoring_risk > 0.0:
            return StructureSupportClass.CENSORED_OR_WEAK
        return StructureSupportClass.UNKNOWN


@dataclass(frozen=True)
class RestorationRecord:
    """A reversible conservation/restoration layer.

    ``replaces_source`` is intentionally present only so the validator can
    reject any attempt to overwrite immutable evidence.
    """

    layer_id: str
    restoration_class: RestorationClass
    source_evidence_sha256: Tuple[str, ...]
    mask_sha256: str
    method: str
    replaces_source: bool = False
    confidence: Optional[float] = None
    hypothesis_description: Optional[str] = None

    def validate(self) -> "RestorationRecord":
        if not self.layer_id.strip():
            raise ContractError("restoration layer_id may not be empty")
        if self.replaces_source:
            raise ContractError("restoration may never replace immutable source evidence")
        _check_sha256(self.mask_sha256, "restoration mask_sha256")
        for h in self.source_evidence_sha256:
            _check_sha256(h, "restoration source_evidence_sha256")
        if not self.method.strip():
            raise ContractError("restoration method is required")
        if self.confidence is not None:
            _check_unit_interval(self.confidence, "restoration confidence")

        if self.restoration_class is RestorationClass.OBSERVED_LOSS:
            if not self.source_evidence_sha256:
                raise ContractError("OBSERVED_LOSS requires source evidence")
        elif self.restoration_class is RestorationClass.EVIDENCE_SUPPORTED_RECONSTRUCTION:
            if not self.source_evidence_sha256 or self.confidence is None:
                raise ContractError("EVIDENCE_SUPPORTED_RECONSTRUCTION requires evidence and confidence")
        elif self.restoration_class is RestorationClass.HYPOTHETICAL_VISUAL_RESTORATION:
            if not self.hypothesis_description:
                raise ContractError("HYPOTHETICAL_VISUAL_RESTORATION requires a hypothesis description")
        return self


def validate_open_world_bundle(
    illumination: Iterable[IlluminationRecord] = (),
    calibrations: Iterable[CalibrationRecord] = (),
    structure: Iterable[StructureEvidenceRecord] = (),
    restoration: Iterable[RestorationRecord] = (),
) -> None:
    """Validate a bundle without imposing a world-size boundary.

    The function intentionally validates authority/provenance only. It does
    not require all records to refer to one room or finite enclosure; an open
    street, landscape, sky, building exterior/interior transition, or arbitrary
    scene graph is valid as long as each claim keeps its own evidence status.
    """
    for r in illumination:
        r.validate()
    for r in calibrations:
        r.validate()
    for r in structure:
        r.validate()
    for r in restoration:
        r.validate()
