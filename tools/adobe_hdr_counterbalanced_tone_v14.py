from __future__ import annotations

from dataclasses import dataclass
from typing import Tuple


@dataclass(frozen=True)
class HdrRenderObservation:
    source_cfa_sha256: str
    file_sha256: str
    decoded_color_sha256: str
    decoded_gainmap_sha256: str
    primaries: str
    transfer: str
    pixel_format: str
    hdr_limit_ev: float
    exposure_ev: float
    highlights: float
    whites: float
    tone_curve: Tuple[Tuple[int, int], ...]
    extended_tone_curve: Tuple[Tuple[int, int], ...]
    maxcll_nits: int
    y_median_code: float
    y_p99_code: float
    y_p999_code: float
    y_p9999_code: float
    y_max_code: int
    y_code_ceiling: int
    y_ceiling_fraction: float

    def validate(self) -> None:
        if len(self.source_cfa_sha256) != 64:
            raise ValueError("source CFA hash required")
        if len(self.file_sha256) != 64:
            raise ValueError("file hash required")
        if not (0 <= self.y_max_code <= self.y_code_ceiling):
            raise ValueError("invalid decoded luma range")
        if not (0.0 <= self.y_ceiling_fraction <= 1.0):
            raise ValueError("invalid ceiling fraction")
        if self.maxcll_nits < 0:
            raise ValueError("invalid MaxCLL")


@dataclass(frozen=True)
class CounterbalancedToneResult:
    same_scientific_source: bool
    same_hdr_transport: bool
    bulk_median_delta_code: float
    p99_delta_code: float
    p999_delta_code: float
    p9999_delta_code: float
    max_code_delta: int
    maxcll_ratio: float
    bulk_darker_tail_brighter: bool
    sparse_output_ceiling_contact: bool
    creates_new_sensor_evidence: bool = False
    recovers_censored_radiance: bool = False
    scientific_master_writeback_allowed: bool = False


def compare_counterbalanced_tone(
    control: HdrRenderObservation,
    counterbalanced: HdrRenderObservation,
    *,
    sparse_ceiling_threshold: float = 1e-3,
) -> CounterbalancedToneResult:
    control.validate()
    counterbalanced.validate()

    same_source = control.source_cfa_sha256 == counterbalanced.source_cfa_sha256
    same_transport = (
        control.primaries == counterbalanced.primaries
        and control.transfer == counterbalanced.transfer
        and control.pixel_format == counterbalanced.pixel_format
        and control.hdr_limit_ev == counterbalanced.hdr_limit_ev
    )
    if not same_source:
        raise ValueError("counterbalanced comparison requires the same CFA authority")
    if not same_transport:
        raise ValueError("counterbalanced comparison requires fixed HDR transport")

    ratio = float("inf") if control.maxcll_nits == 0 else counterbalanced.maxcll_nits / control.maxcll_nits
    median_delta = counterbalanced.y_median_code - control.y_median_code
    p99_delta = counterbalanced.y_p99_code - control.y_p99_code
    p999_delta = counterbalanced.y_p999_code - control.y_p999_code
    p9999_delta = counterbalanced.y_p9999_code - control.y_p9999_code
    max_delta = counterbalanced.y_max_code - control.y_max_code

    return CounterbalancedToneResult(
        same_scientific_source=True,
        same_hdr_transport=True,
        bulk_median_delta_code=median_delta,
        p99_delta_code=p99_delta,
        p999_delta_code=p999_delta,
        p9999_delta_code=p9999_delta,
        max_code_delta=max_delta,
        maxcll_ratio=ratio,
        bulk_darker_tail_brighter=(median_delta < 0 and p99_delta < 0 and p999_delta > 0 and p9999_delta > 0),
        sparse_output_ceiling_contact=(0.0 < counterbalanced.y_ceiling_fraction < sparse_ceiling_threshold),
    )


def decoded_render_is_identical(a: HdrRenderObservation, b: HdrRenderObservation) -> bool:
    """Container/provenance bytes may differ while decoded color and gain-map payloads are identical."""
    a.validate()
    b.validate()
    return (
        a.decoded_color_sha256 == b.decoded_color_sha256
        and a.decoded_gainmap_sha256 == b.decoded_gainmap_sha256
        and a.source_cfa_sha256 == b.source_cfa_sha256
    )


def raw_carrier_chain_is_exact(
    previous_adobe_raw_tile_payload_sha256: str,
    new_adobe_raw_tile_payload_sha256: str,
    *,
    previous_adobe_dng_was_cfa_exact_to_immutable_source: bool,
    tile_count_previous: int,
    tile_count_new: int,
) -> bool:
    """Transitive carrier check: identical compressed raw tile payload + prior decoded-CFA proof."""
    return (
        previous_adobe_dng_was_cfa_exact_to_immutable_source
        and tile_count_previous > 0
        and tile_count_previous == tile_count_new
        and previous_adobe_raw_tile_payload_sha256 == new_adobe_raw_tile_payload_sha256
    )
