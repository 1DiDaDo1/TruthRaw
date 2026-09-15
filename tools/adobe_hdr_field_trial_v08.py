#!/usr/bin/env python3
"""Real single-exposure HONOR tele -> Adobe Lightroom HDR field-trial contract v0.8.

This module binds one real 4080x3072 HONOR tele source to existing TruthRaw
source evidence and defines what an Adobe HDR round-trip is allowed to prove.

Stage-2 overrange is a source-pipeline diagnostic, not the same thing as Adobe's
presentation-space HDR headroom. HDR editing never creates new measured sensor
evidence; censored highlights remain lower-bound evidence only.
"""
from __future__ import annotations

from dataclasses import dataclass
from math import isfinite, log2
from typing import Optional, Tuple

from open_world_foundations_v01 import ContractError
from adobe_hdr_interop_v07 import AdobeHdrFormat, AdobeHdrRoute

_SHA256_HEX = set("0123456789abcdef")


def _check_sha256(value: str, field_name: str) -> str:
    v = value.lower()
    if len(v) != 64 or any(ch not in _SHA256_HEX for ch in v):
        raise ContractError(f"{field_name} must be a 64-character hexadecimal SHA-256")
    return v


def _finite(value: float, field_name: str) -> float:
    v = float(value)
    if not isfinite(v):
        raise ContractError(f"{field_name} must be finite")
    return v


@dataclass(frozen=True)
class RealTeleHdrCandidateV08:
    file_name: str
    source_sha256: str
    width: int
    height: int
    cfa: str
    white_level: float
    black_levels: Tuple[float, float, float, float]
    iso: int
    exposure_s: float
    f_number: float
    focal_length_mm: float
    physical_camera_id: str
    physical_frame_count: int
    independent_evidence_count: int
    stage2_min: float
    stage2_max: float
    stage2_over1_count: int
    source_white_count: int
    q99_pre_display_exposure: float
    legacy_sdr_display_scalar: float
    uncertainty_decision: str
    uses_exposure_merge: bool = False
    fake_hdr_merge_metadata: bool = False

    def validate(self) -> "RealTeleHdrCandidateV08":
        if not self.file_name.lower().endswith(".dng"):
            raise ContractError("field-trial source must be a DNG")
        _check_sha256(self.source_sha256, "source_sha256")
        if (self.width, self.height, self.cfa) != (4080, 3072, "BGGR"):
            raise ContractError("v0.8 is frozen to the 4080x3072 BGGR tele source")
        if len(self.black_levels) != 4:
            raise ContractError("four Bayer black levels are required")
        for value in self.black_levels:
            _finite(value, "black_level")
        if _finite(self.white_level, "white_level") <= max(self.black_levels):
            raise ContractError("white level must exceed black levels")
        if self.iso <= 0 or _finite(self.exposure_s, "exposure_s") <= 0.0:
            raise ContractError("ISO/exposure must be positive")
        if _finite(self.f_number, "f_number") <= 0.0 or _finite(self.focal_length_mm, "focal_length_mm") <= 0.0:
            raise ContractError("optical metadata must be positive")
        if self.physical_camera_id != "5":
            raise ContractError("v0.8 is frozen to physical camera id 5")
        if self.physical_frame_count != 1 or self.independent_evidence_count != 1:
            raise ContractError("field trial must remain exactly one physical frame/evidence item")
        if self.uses_exposure_merge or self.fake_hdr_merge_metadata:
            raise ContractError("exposure merge and fake HDR-Merge metadata are forbidden")
        lo = _finite(self.stage2_min, "stage2_min")
        hi = _finite(self.stage2_max, "stage2_max")
        if hi < lo:
            raise ContractError("stage2_max must be >= stage2_min")
        n = self.width * self.height
        if not (0 <= self.stage2_over1_count <= n and 0 <= self.source_white_count <= n):
            raise ContractError("sample counts are outside the source raster")
        if _finite(self.q99_pre_display_exposure, "q99_pre_display_exposure") <= 0.0:
            raise ContractError("q99_pre_display_exposure must be > 0")
        if _finite(self.legacy_sdr_display_scalar, "legacy_sdr_display_scalar") <= 0.0:
            raise ContractError("legacy_sdr_display_scalar must be > 0")
        if self.uncertainty_decision != "BACKEND_BOUND_UNCERTAINTY_PROSPECTIVE_PASS":
            raise ContractError("v0.8 requires the frozen prospective-pass tele source")
        return self

    @property
    def sample_count(self) -> int:
        return self.width * self.height

    @property
    def stage2_over1_fraction(self) -> float:
        self.validate()
        return self.stage2_over1_count / self.sample_count

    @property
    def source_white_fraction(self) -> float:
        self.validate()
        return self.source_white_count / self.sample_count


@dataclass(frozen=True)
class RealTeleHeadroomDiagnosticsV08:
    stage2_over_unity_headroom_ev: float
    q99_anchored_headroom_ev: float
    legacy_sdr_preview_peak: float
    legacy_sdr_preview_peak_ev_vs_unity: float
    stage2_over1_fraction: float
    source_white_fraction: float
    exact_censored_radiance_recovery_allowed: bool
    source_white_semantics: str
    interpretation: str


def derive_real_tele_headroom_diagnostics_v08(candidate: RealTeleHdrCandidateV08) -> RealTeleHeadroomDiagnosticsV08:
    c = candidate.validate()
    unity_ev = max(0.0, log2(c.stage2_max)) if c.stage2_max > 0.0 else 0.0
    q99_ev = max(0.0, log2(c.stage2_max / c.q99_pre_display_exposure))
    legacy_peak = c.stage2_max * c.legacy_sdr_display_scalar
    legacy_peak_ev = log2(legacy_peak) if legacy_peak > 0.0 else float("-inf")
    return RealTeleHeadroomDiagnosticsV08(
        stage2_over_unity_headroom_ev=unity_ev,
        q99_anchored_headroom_ev=q99_ev,
        legacy_sdr_preview_peak=legacy_peak,
        legacy_sdr_preview_peak_ev_vs_unity=legacy_peak_ev,
        stage2_over1_fraction=c.stage2_over1_fraction,
        source_white_fraction=c.source_white_fraction,
        exact_censored_radiance_recovery_allowed=False,
        source_white_semantics="CENSORED_LOWER_BOUND_AT_WHITELEVEL_NOT_EXACT_RECOVERED_RADIANCE",
        interpretation=(
            "Source-pipeline overrange diagnostics only. They do not predict Lightroom HDR histogram "
            "headroom and do not create additional measured dynamic range."
        ),
    )


@dataclass(frozen=True)
class LightroomHdrFieldTrialPlanV08:
    schema: str
    candidate: RealTeleHdrCandidateV08
    route: AdobeHdrRoute
    raw_input_format: AdobeHdrFormat
    hdr_activation: str
    automatic_hdr_activation_assumed: bool
    uses_exposure_merge: bool
    fake_hdr_merge_metadata: bool
    preserve_original_dng_bytes: bool
    save_adobe_settings_as_sidecar_or_catalog_metadata: bool
    requested_exports: Tuple[str, ...]
    required_observations: Tuple[str, ...]
    scientific_master_writeback_from_adobe: bool
    presentation_result_may_be_brighter_than_source_normalization: bool
    new_measured_evidence_created_by_hdr_editing: bool
    diagnostics: RealTeleHeadroomDiagnosticsV08


def build_real_tele_lightroom_field_trial_v08(candidate: RealTeleHdrCandidateV08) -> LightroomHdrFieldTrialPlanV08:
    c = candidate.validate()
    return LightroomHdrFieldTrialPlanV08(
        schema="TruthRawAdobeLightroomHdrFieldTrial/0.8",
        candidate=c,
        route=AdobeHdrRoute.SINGLE_EXPOSURE_RAW,
        raw_input_format=AdobeHdrFormat.DNG,
        hdr_activation="MANUAL_EDIT_HDR_ON_REAL_SINGLE_EXPOSURE_DNG",
        automatic_hdr_activation_assumed=False,
        uses_exposure_merge=False,
        fake_hdr_merge_metadata=False,
        preserve_original_dng_bytes=True,
        save_adobe_settings_as_sidecar_or_catalog_metadata=True,
        requested_exports=(
            "ORIGINAL_DNG_UNCHANGED",
            "ADOBE_SETTINGS_XMP_OR_EXPORTED_METADATA",
            "HDR_TIFF_16BIT_IF_AVAILABLE",
            "HDR_AVIF_OR_JPEG_XL_IF_AVAILABLE",
            "GAIN_MAP_JPEG_OPTIONAL_PRESENTATION_TEST",
        ),
        required_observations=(
            "LIGHTROOM_VERSION",
            "HDR_BUTTON_AVAILABLE",
            "HDR_MODE_ENABLED",
            "HDR_LIMIT_EV_SETTING",
            "VISUALIZE_HDR_HISTOGRAM_HEADROOM",
            "DISPLAY_HEADROOM_AVAILABLE_EV_IF_REPORTED",
            "PROFILE_AND_PROCESS_VERSION",
            "EXPOSURE_AND_TONE_SETTINGS",
            "EXPORT_FORMAT_AND_SHA256",
        ),
        scientific_master_writeback_from_adobe=False,
        presentation_result_may_be_brighter_than_source_normalization=True,
        new_measured_evidence_created_by_hdr_editing=False,
        diagnostics=derive_real_tele_headroom_diagnostics_v08(c),
    )


@dataclass(frozen=True)
class LightroomHdrRoundTripObservationV08:
    source_sha256: str
    lightroom_version: str
    hdr_button_available: bool
    hdr_mode_enabled: bool
    hdr_limit_ev_setting: Optional[float]
    visualize_hdr_headroom_ev_observed: Optional[float]
    display_headroom_ev_reported: Optional[float]
    profile_name: str
    process_version: str
    exposure_adjustment_ev: float
    tone_controls_modified: bool
    local_masks_used: bool
    ai_scene_edit_used: bool
    exposure_merge_used: bool
    export_format: Optional[str] = None
    export_sha256: Optional[str] = None
    xmp_or_metadata_sha256: Optional[str] = None

    def validate(self, candidate: RealTeleHdrCandidateV08) -> "LightroomHdrRoundTripObservationV08":
        c = candidate.validate()
        if _check_sha256(self.source_sha256, "source_sha256") != c.source_sha256.lower():
            raise ContractError("Adobe observation is not bound to the frozen real tele source")
        if not self.lightroom_version.strip():
            raise ContractError("Lightroom version must be recorded")
        if self.exposure_merge_used:
            raise ContractError("field trial is invalid if exposure merge was used")
        if self.ai_scene_edit_used:
            raise ContractError("AI scene editing is outside the v0.8 scientific field trial")
        if self.hdr_mode_enabled and not self.hdr_button_available:
            raise ContractError("HDR mode cannot be enabled when the control was unavailable")
        for field_name, value in (
            ("hdr_limit_ev_setting", self.hdr_limit_ev_setting),
            ("visualize_hdr_headroom_ev_observed", self.visualize_hdr_headroom_ev_observed),
            ("display_headroom_ev_reported", self.display_headroom_ev_reported),
        ):
            if value is not None and (not isfinite(float(value)) or float(value) < 0.0):
                raise ContractError(f"{field_name} must be finite and nonnegative")
        _finite(self.exposure_adjustment_ev, "exposure_adjustment_ev")
        if self.export_sha256 is not None:
            _check_sha256(self.export_sha256, "export_sha256")
        if self.xmp_or_metadata_sha256 is not None:
            _check_sha256(self.xmp_or_metadata_sha256, "xmp_or_metadata_sha256")
        return self

    @property
    def scientific_authority_writeback_allowed(self) -> bool:
        return False

    @property
    def observed_adobe_headroom_is_presentation_evidence_only(self) -> bool:
        return True


REAL_TELE_FIELD_TRIAL_CANDIDATE_V08 = RealTeleHdrCandidateV08(
    file_name="IMG_BNC_TRUTHRAW20260907_094449_565.dng",
    source_sha256="7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67",
    width=4080,
    height=3072,
    cfa="BGGR",
    white_level=1023.0,
    black_levels=(64.0, 64.0, 64.0, 64.0),
    iso=638,
    exposure_s=0.009999993,
    f_number=2.6,
    focal_length_mm=22.48,
    physical_camera_id="5",
    physical_frame_count=1,
    independent_evidence_count=1,
    stage2_min=-0.006340971682220697,
    stage2_max=1.2977731227874756,
    stage2_over1_count=62643,
    source_white_count=217,
    q99_pre_display_exposure=0.9892458261670347,
    legacy_sdr_display_scalar=0.72782718001423,
    uncertainty_decision="BACKEND_BOUND_UNCERTAINTY_PROSPECTIVE_PASS",
)
