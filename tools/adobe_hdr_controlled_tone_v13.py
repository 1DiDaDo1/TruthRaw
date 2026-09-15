from __future__ import annotations

from dataclasses import dataclass
from math import log2
from typing import Sequence


@dataclass(frozen=True)
class HdrRenderObservation:
    file_name: str
    source_cfa_sha256: str
    primaries: str
    transfer: str
    pixel_format: str
    hdr_limit_ev: float
    exposure_ev: float
    whites: float
    scalar_settings_fingerprint: str
    tone_curve: tuple[tuple[int, int], ...]
    extended_tone_curve: tuple[tuple[int, int], ...]
    maxcll_nits: float
    decoded_luma_max_code: int
    decoded_luma_code_ceiling: int
    decoded_luma_ceiling_fraction: float

    def __post_init__(self) -> None:
        if not self.file_name:
            raise ValueError("file_name is required")
        if len(self.source_cfa_sha256) != 64:
            raise ValueError("source_cfa_sha256 must be a SHA-256 hex digest")
        if self.transfer != "SMPTE_ST_2084_PQ":
            raise ValueError("v1.3 comparison is scoped to PQ HDR renders")
        if self.maxcll_nits <= 0:
            raise ValueError("maxcll_nits must be positive")
        if self.decoded_luma_code_ceiling <= 0:
            raise ValueError("decoded_luma_code_ceiling must be positive")
        if not 0 <= self.decoded_luma_max_code <= self.decoded_luma_code_ceiling:
            raise ValueError("decoded luma maximum is outside code range")
        if not 0.0 <= self.decoded_luma_ceiling_fraction <= 1.0:
            raise ValueError("decoded_luma_ceiling_fraction must be in [0,1]")


@dataclass(frozen=True)
class ControlledToneComparison:
    controlled_tone_only: bool
    classification: str
    maxcll_ratio_b_over_a: float
    maxcll_delta_ev_b_over_a: float
    a_ceiling_saturated: bool
    b_ceiling_saturated: bool
    scientific_authority_changed: bool
    creates_new_measured_dynamic_range: bool


def compare_controlled_tone_pair(
    a: HdrRenderObservation,
    b: HdrRenderObservation,
    *,
    ceiling_saturation_fraction_threshold: float = 0.01,
) -> ControlledToneComparison:
    """Compare two HDR renders while enforcing a one-variable tone-curve test.

    The pair is considered controlled only when source CFA identity, output gamut,
    transfer function, pixel format and all non-curve Lightroom scalar settings
    match. Tone curves are allowed to differ. Output differences remain presentation
    evidence and never create scientific/source authority.
    """

    if not 0.0 <= ceiling_saturation_fraction_threshold <= 1.0:
        raise ValueError("invalid ceiling_saturation_fraction_threshold")

    same_non_curve_state = (
        a.source_cfa_sha256 == b.source_cfa_sha256
        and a.primaries == b.primaries
        and a.transfer == b.transfer
        and a.pixel_format == b.pixel_format
        and a.hdr_limit_ev == b.hdr_limit_ev
        and a.exposure_ev == b.exposure_ev
        and a.whites == b.whites
        and a.scalar_settings_fingerprint == b.scalar_settings_fingerprint
        and a.decoded_luma_code_ceiling == b.decoded_luma_code_ceiling
    )
    tone_differs = (
        a.tone_curve != b.tone_curve
        or a.extended_tone_curve != b.extended_tone_curve
    )
    controlled = bool(same_non_curve_state and tone_differs)

    a_sat = a.decoded_luma_ceiling_fraction >= ceiling_saturation_fraction_threshold
    b_sat = b.decoded_luma_ceiling_fraction >= ceiling_saturation_fraction_threshold

    if controlled and (a_sat or b_sat):
        classification = "CONTROLLED_TONE_ONLY_WITH_PRESENTATION_CEILING_SATURATION"
    elif controlled:
        classification = "CONTROLLED_TONE_ONLY_PRESENTATION_COMPARISON"
    else:
        classification = "NOT_A_CONTROLLED_TONE_ONLY_COMPARISON"

    ratio = b.maxcll_nits / a.maxcll_nits
    return ControlledToneComparison(
        controlled_tone_only=controlled,
        classification=classification,
        maxcll_ratio_b_over_a=ratio,
        maxcll_delta_ev_b_over_a=log2(ratio),
        a_ceiling_saturated=a_sat,
        b_ceiling_saturated=b_sat,
        scientific_authority_changed=False,
        creates_new_measured_dynamic_range=False,
    )


def curve_from_pairs(pairs: Sequence[Sequence[int]]) -> tuple[tuple[int, int], ...]:
    return tuple((int(x), int(y)) for x, y in pairs)
