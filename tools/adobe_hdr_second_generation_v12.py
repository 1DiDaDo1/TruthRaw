#!/usr/bin/env python3
"""Adobe second-generation HDR render observation contract v1.2.

This module records a real second-generation Adobe HDR AVIF observation without
silently asserting which intermediate (TIFF or Adobe-rewritten DNG) was its
parent. Embedded metadata can preserve RawFileName and prior Camera Raw state
without uniquely proving the immediate parent route.

Scientific boundary:
- Adobe HDR output is presentation evidence only.
- Same HDR Limit / Exposure / Whites values do not imply render equivalence.
- Tone-curve control points and encoded output measurements are part of the
  presentation state and must be compared explicitly.
- No Adobe render can create new measured CFA authority or exact radiance for
  censored source samples.
"""
from __future__ import annotations

from dataclasses import dataclass
from math import isfinite, log2
from typing import Optional, Tuple

from open_world_foundations_v01 import ContractError

TonePoint = Tuple[int, int]


def _check_sha256(value: str, name: str) -> str:
    v = value.lower()
    if len(v) != 64 or any(c not in "0123456789abcdef" for c in v):
        raise ContractError(f"{name} must be a 64-character hexadecimal SHA-256")
    return v


@dataclass(frozen=True)
class AdobeHdrRenderObservationV12:
    file_name: str
    sha256: str
    bytes: int
    width: int
    height: int
    bit_depth: int
    chroma: str
    transfer: str
    primaries: str
    hdr_edit_mode: bool
    hdr_limit_ev: float
    exposure_2012_ev: float
    whites_2012: float
    tone_curve_points: Tuple[TonePoint, ...]
    extended_tone_curve_points: Tuple[TonePoint, ...]
    maxcll_nits: float
    decoded_luma_max_code: int
    decoded_luma_p99_code: int
    code_ceiling: int = 1023
    immediate_parent_route: Optional[str] = None

    def validate(self) -> "AdobeHdrRenderObservationV12":
        if not self.file_name.lower().endswith(".avif"):
            raise ContractError("v1.2 observation must be an AVIF")
        _check_sha256(self.sha256, "sha256")
        if self.bytes <= 0 or self.width <= 0 or self.height <= 0:
            raise ContractError("bytes and geometry must be positive")
        if self.bit_depth != 10 or self.chroma != "4:4:4":
            raise ContractError("frozen v1.2 observation expects 10-bit 4:4:4 AVIF")
        if self.transfer != "SMPTE_ST_2084_PQ":
            raise ContractError("frozen v1.2 observation expects explicit ST-2084/PQ")
        if not self.hdr_edit_mode:
            raise ContractError("HDR edit mode must be active")
        for name, value in (
            ("hdr_limit_ev", self.hdr_limit_ev),
            ("exposure_2012_ev", self.exposure_2012_ev),
            ("whites_2012", self.whites_2012),
            ("maxcll_nits", self.maxcll_nits),
        ):
            if not isfinite(float(value)):
                raise ContractError(f"{name} must be finite")
        if self.hdr_limit_ev < 0.0 or self.maxcll_nits <= 0.0:
            raise ContractError("HDR limit/maxCLL must be positive/nonnegative")
        if not (0 <= self.decoded_luma_p99_code <= self.decoded_luma_max_code <= self.code_ceiling):
            raise ContractError("decoded luma codes are outside the declared code ceiling")
        if len(self.tone_curve_points) < 2 or len(self.extended_tone_curve_points) < 2:
            raise ContractError("tone curves require at least two points")
        return self


@dataclass(frozen=True)
class AdobeHdrRenderComparisonV12:
    same_nominal_hdr_limit: bool
    same_nominal_exposure: bool
    same_nominal_whites: bool
    same_tone_curve: bool
    same_extended_tone_curve: bool
    maxcll_ratio_b_over_a: float
    maxcll_delta_ev_b_minus_a: float
    luma_peak_code_delta_b_minus_a: int
    nominal_controls_are_sufficient_render_identity: bool
    scientific_authority_changed: bool
    interpretation: str


def compare_adobe_hdr_renders_v12(
    a: AdobeHdrRenderObservationV12,
    b: AdobeHdrRenderObservationV12,
) -> AdobeHdrRenderComparisonV12:
    aa = a.validate()
    bb = b.validate()
    ratio = bb.maxcll_nits / aa.maxcll_nits
    return AdobeHdrRenderComparisonV12(
        same_nominal_hdr_limit=aa.hdr_limit_ev == bb.hdr_limit_ev,
        same_nominal_exposure=aa.exposure_2012_ev == bb.exposure_2012_ev,
        same_nominal_whites=aa.whites_2012 == bb.whites_2012,
        same_tone_curve=aa.tone_curve_points == bb.tone_curve_points,
        same_extended_tone_curve=aa.extended_tone_curve_points == bb.extended_tone_curve_points,
        maxcll_ratio_b_over_a=ratio,
        maxcll_delta_ev_b_minus_a=log2(ratio),
        luma_peak_code_delta_b_minus_a=bb.decoded_luma_max_code - aa.decoded_luma_max_code,
        nominal_controls_are_sufficient_render_identity=False,
        scientific_authority_changed=False,
        interpretation=(
            "Adobe presentation renders may differ even when HDR Limit, Exposure and Whites match. "
            "Tone-curve shape and encoded output measurements must remain explicit; no difference "
            "changes Scientific Master or measured CFA authority."
        ),
    )


def parent_route_is_proven_v12(observation: AdobeHdrRenderObservationV12) -> bool:
    observation.validate()
    return observation.immediate_parent_route in {
        "REIMPORTED_ADOBE_TIFF",
        "REIMPORTED_ADOBE_DNG_REWRITE",
        "ORIGINAL_SOURCE_DNG",
    }


OLD_TONE_EXPANDED_REC709_V09 = AdobeHdrRenderObservationV12(
    file_name="IMG_BNC_TRUTHRAW20260907_094449_565 (2).avif",
    sha256="60f8cd7a51f8b011f131ecb10932bbda28b03d3444e06d135c2828865fb77d8e",
    bytes=9973759,
    width=4064,
    height=3056,
    bit_depth=10,
    chroma="4:4:4",
    transfer="SMPTE_ST_2084_PQ",
    primaries="REC709",
    hdr_edit_mode=True,
    hdr_limit_ev=8.0,
    exposure_2012_ev=0.0,
    whites_2012=2.0,
    tone_curve_points=((0, 12), (152, 228), (255, 255)),
    extended_tone_curve_points=((0, 12), (152, 228), (386, 500)),
    maxcll_nits=5259.0,
    decoded_luma_max_code=956,
    decoded_luma_p99_code=842,
    immediate_parent_route="ORIGINAL_SOURCE_DNG",
)


UNEDITED_P3_V09 = AdobeHdrRenderObservationV12(
    file_name="IMG_BNC_TRUTHRAW20260907_094449_565 (1).avif",
    sha256="e4ddf68cc4f286dcad979a01776f2755b8116235d375b569d7f3ed8b2cc39b31",
    bytes=1691444,
    width=4064,
    height=3056,
    bit_depth=10,
    chroma="4:4:4",
    transfer="SMPTE_ST_2084_PQ",
    primaries="P3_D65",
    hdr_edit_mode=True,
    hdr_limit_ev=8.0,
    exposure_2012_ev=0.0,
    whites_2012=0.0,
    tone_curve_points=((0, 0), (255, 255)),
    extended_tone_curve_points=((0, 0), (500, 500)),
    maxcll_nits=1383.0,
    decoded_luma_max_code=815,
    decoded_luma_p99_code=696,
    immediate_parent_route="ORIGINAL_SOURCE_DNG",
)


SECOND_GENERATION_P3_V12 = AdobeHdrRenderObservationV12(
    file_name="IMG_BNC_TRUTHRAW20260907_094449_565 (4).avif",
    sha256="552a463095103d9d74baec594994a438c070e9c57beadae1b2f503b847d5bc9f",
    bytes=10641173,
    width=4064,
    height=3056,
    bit_depth=10,
    chroma="4:4:4",
    transfer="SMPTE_ST_2084_PQ",
    primaries="P3_D65",
    hdr_edit_mode=True,
    hdr_limit_ev=8.0,
    exposure_2012_ev=0.0,
    whites_2012=2.0,
    tone_curve_points=((0, 0), (133, 196), (255, 255)),
    extended_tone_curve_points=((0, 0), (133, 196), (332, 446), (500, 500)),
    maxcll_nits=2878.0,
    decoded_luma_max_code=889,
    decoded_luma_p99_code=841,
    immediate_parent_route=None,
)
