#!/usr/bin/env python3
"""TruthRaw empirical Adobe Lightroom HDR round-trip contract v0.9.

This module freezes the first real Lightroom HDR AVIF observations from the
single-exposure HONOR BKQ-N49 tele source used by v0.8. It separates source
truth from presentation expansion:

- the physical source DNG remains one immutable exposure/evidence root;
- unedited HDR exports are presentation observations, not new measurements;
- changing Lightroom Whites/Tone Curve may occupy much more HDR output
  headroom, but that additional brightness is appearance/presentation only;
- Adobe HDR Limit is an output ceiling/window, not a sensor dynamic-range
  measurement;
- exported AVIF gain-map/tone-map structure may never write authority back to
  the Scientific Master.
"""
from __future__ import annotations

from dataclasses import dataclass
from math import isfinite, log2
from typing import Tuple

from open_world_foundations_v01 import ContractError

_HEX = set("0123456789abcdef")
SOURCE_DNG_SHA256 = "7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67"


def _sha(value: str, name: str) -> str:
    v = value.lower()
    if len(v) != 64 or any(c not in _HEX for c in v):
        raise ContractError(f"{name} must be a 64-character hexadecimal SHA-256")
    return v


def _finite(value: float, name: str) -> float:
    v = float(value)
    if not isfinite(v):
        raise ContractError(f"{name} must be finite")
    return v


@dataclass(frozen=True)
class AdobeHdrAvifObservationV09:
    file_name: str
    export_sha256: str
    bytes: int
    source_dng_sha256: str
    width: int
    height: int
    bit_depth: int
    chroma: str
    transfer: str
    primaries: str
    hdr_edit_mode: bool
    hdr_limit_ev: float
    exposure_2012: float
    whites_2012: float
    tone_curve_name: str
    tone_curve_points: Tuple[Tuple[int, int], ...]
    extended_tone_curve_points: Tuple[Tuple[int, int], ...]
    ccv_min_luminance_nits: float
    ccv_max_luminance_nits: float
    ccv_avg_luminance_nits: float
    max_content_light_level_nits: int
    gainmap_stream_present: bool
    tmap_compatible_brand_present: bool
    scene_referred_export_metadata: bool

    def validate(self) -> "AdobeHdrAvifObservationV09":
        if not self.file_name.lower().endswith(".avif"):
            raise ContractError("v0.9 observation must be an AVIF export")
        _sha(self.export_sha256, "export_sha256")
        if _sha(self.source_dng_sha256, "source_dng_sha256") != SOURCE_DNG_SHA256:
            raise ContractError("export is not bound to the frozen real tele source")
        if (self.width, self.height) != (4064, 3056):
            raise ContractError("unexpected exported crop geometry")
        if self.bit_depth != 10 or self.chroma != "4:4:4":
            raise ContractError("v0.9 empirical export must remain 10-bit 4:4:4")
        if self.transfer != "SMPTE_ST_2084_PQ":
            raise ContractError("v0.9 empirical export must use PQ")
        if self.primaries not in ("REC709", "P3_D65"):
            raise ContractError("unexpected HDR export primaries")
        if not self.hdr_edit_mode:
            raise ContractError("HDR edit mode was not enabled")
        if abs(_finite(self.hdr_limit_ev, "hdr_limit_ev") - 8.0) > 1e-9:
            raise ContractError("v0.9 is frozen to the +8 EV HDR-limit trial")
        _finite(self.exposure_2012, "exposure_2012")
        _finite(self.whites_2012, "whites_2012")
        for n, v in (
            ("ccv_min_luminance_nits", self.ccv_min_luminance_nits),
            ("ccv_max_luminance_nits", self.ccv_max_luminance_nits),
            ("ccv_avg_luminance_nits", self.ccv_avg_luminance_nits),
        ):
            if _finite(v, n) < 0.0:
                raise ContractError(f"{n} must be nonnegative")
        if self.ccv_max_luminance_nits < self.ccv_avg_luminance_nits:
            raise ContractError("maximum luminance must not be below average luminance")
        if self.max_content_light_level_nits <= 0:
            raise ContractError("MaxCLL must be positive")
        if self.scene_referred_export_metadata:
            raise ContractError("these Adobe AVIF exports are display/presentation referred")
        return self

    @property
    def scientific_authority_writeback_allowed(self) -> bool:
        return False

    @property
    def creates_new_measured_evidence(self) -> bool:
        return False


@dataclass(frozen=True)
class AdobeHdrPresentationExpansionV09:
    base_file: str
    edited_file: str
    hdr_limit_ev_same: bool
    physical_source_same: bool
    max_luminance_ratio: float
    max_luminance_added_presentation_ev: float
    maxcll_ratio: float
    maxcll_added_presentation_ev: float
    average_luminance_ratio: float
    average_luminance_added_presentation_ev: float
    classification: str
    scientific_master_writeback_allowed: bool
    new_measured_dynamic_range_created: bool


def compare_presentation_expansion_v09(
    base: AdobeHdrAvifObservationV09,
    edited: AdobeHdrAvifObservationV09,
) -> AdobeHdrPresentationExpansionV09:
    a = base.validate()
    b = edited.validate()
    if a.source_dng_sha256.lower() != b.source_dng_sha256.lower():
        raise ContractError("round-trip comparison requires the same physical source")
    if a.primaries != b.primaries:
        raise ContractError("tone-expansion comparison requires the same output primaries")
    if a.tone_curve_name != "Linear" or abs(a.whites_2012) > 1e-12:
        raise ContractError("base export must be the unedited linear-tone observation")
    if b.tone_curve_name != "Custom":
        raise ContractError("edited export must contain the custom tone curve")
    if abs(b.whites_2012 - 2.0) > 1e-12:
        raise ContractError("v0.9 edited observation is frozen to Whites +2")
    if abs(a.exposure_2012) > 1e-12 or abs(b.exposure_2012) > 1e-12:
        raise ContractError("global exposure must remain unchanged in this controlled trial")

    lum_ratio = b.ccv_max_luminance_nits / a.ccv_max_luminance_nits
    cll_ratio = b.max_content_light_level_nits / a.max_content_light_level_nits
    avg_ratio = b.ccv_avg_luminance_nits / a.ccv_avg_luminance_nits
    return AdobeHdrPresentationExpansionV09(
        base_file=a.file_name,
        edited_file=b.file_name,
        hdr_limit_ev_same=abs(a.hdr_limit_ev - b.hdr_limit_ev) <= 1e-12,
        physical_source_same=True,
        max_luminance_ratio=lum_ratio,
        max_luminance_added_presentation_ev=log2(lum_ratio),
        maxcll_ratio=cll_ratio,
        maxcll_added_presentation_ev=log2(cll_ratio),
        average_luminance_ratio=avg_ratio,
        average_luminance_added_presentation_ev=log2(avg_ratio),
        classification="ADOBE_TONE_EXPANSION_PRESENTATION_ONLY_NO_NEW_SENSOR_EVIDENCE",
        scientific_master_writeback_allowed=False,
        new_measured_dynamic_range_created=False,
    )


def compare_unedited_gamut_exports_v09(
    rec709: AdobeHdrAvifObservationV09,
    p3: AdobeHdrAvifObservationV09,
) -> dict:
    a = rec709.validate()
    b = p3.validate()
    if (a.primaries, b.primaries) != ("REC709", "P3_D65"):
        raise ContractError("expected Rec.709 and P3 unedited pair")
    if a.tone_curve_name != "Linear" or b.tone_curve_name != "Linear":
        raise ContractError("gamut comparison requires unedited linear-tone exports")
    if abs(a.whites_2012) > 1e-12 or abs(b.whites_2012) > 1e-12:
        raise ContractError("gamut comparison requires Whites 0")
    delta = abs(a.ccv_max_luminance_nits - b.ccv_max_luminance_nits)
    return {
        "classification": "ADOBE_HDR_GAMUT_EXPORT_PRESENTATION_ONLY",
        "max_luminance_absolute_delta_nits": delta,
        "max_luminance_relative_delta": delta / max(a.ccv_max_luminance_nits, b.ccv_max_luminance_nits),
        "maxcll_equal": a.max_content_light_level_nits == b.max_content_light_level_nits,
        "new_measured_evidence_created": False,
        "scientific_master_writeback_allowed": False,
    }


UNEDITED_REC709_V09 = AdobeHdrAvifObservationV09(
    file_name="IMG_BNC_TRUTHRAW20260907_094449_565.avif",
    export_sha256="4828cb3bcba031a063b170664edc7d9de8ad89572e983112229ee1a0c434592c",
    bytes=1856625,
    source_dng_sha256=SOURCE_DNG_SHA256,
    width=4064,
    height=3056,
    bit_depth=10,
    chroma="4:4:4",
    transfer="SMPTE_ST_2084_PQ",
    primaries="REC709",
    hdr_edit_mode=True,
    hdr_limit_ev=8.0,
    exposure_2012=0.0,
    whites_2012=0.0,
    tone_curve_name="Linear",
    tone_curve_points=((0, 0), (255, 255)),
    extended_tone_curve_points=(),
    ccv_min_luminance_nits=0.006453,
    ccv_max_luminance_nits=1404.717812,
    ccv_avg_luminance_nits=18.191564,
    max_content_light_level_nits=1383,
    gainmap_stream_present=True,
    tmap_compatible_brand_present=True,
    scene_referred_export_metadata=False,
)

UNEDITED_P3_V09 = AdobeHdrAvifObservationV09(
    file_name="IMG_BNC_TRUTHRAW20260907_094449_565 (1).avif",
    export_sha256="e4ddf68cc4f286dcad979a01776f2755b8116235d375b569d7f3ed8b2cc39b31",
    bytes=1691444,
    source_dng_sha256=SOURCE_DNG_SHA256,
    width=4064,
    height=3056,
    bit_depth=10,
    chroma="4:4:4",
    transfer="SMPTE_ST_2084_PQ",
    primaries="P3_D65",
    hdr_edit_mode=True,
    hdr_limit_ev=8.0,
    exposure_2012=0.0,
    whites_2012=0.0,
    tone_curve_name="Linear",
    tone_curve_points=((0, 0), (255, 255)),
    extended_tone_curve_points=(),
    ccv_min_luminance_nits=0.006486,
    ccv_max_luminance_nits=1404.659249,
    ccv_avg_luminance_nits=18.192041,
    max_content_light_level_nits=1383,
    gainmap_stream_present=True,
    tmap_compatible_brand_present=True,
    scene_referred_export_metadata=False,
)

TONE_EXPANDED_REC709_V09 = AdobeHdrAvifObservationV09(
    file_name="IMG_BNC_TRUTHRAW20260907_094449_565 (2).avif",
    export_sha256="60f8cd7a51f8b011f131ecb10932bbda28b03d3444e06d135c2828865fb77d8e",
    bytes=9973759,
    source_dng_sha256=SOURCE_DNG_SHA256,
    width=4064,
    height=3056,
    bit_depth=10,
    chroma="4:4:4",
    transfer="SMPTE_ST_2084_PQ",
    primaries="REC709",
    hdr_edit_mode=True,
    hdr_limit_ev=8.0,
    exposure_2012=0.0,
    whites_2012=2.0,
    tone_curve_name="Custom",
    tone_curve_points=((0, 12), (152, 228), (255, 255)),
    extended_tone_curve_points=((0, 12), (152, 228), (386, 500)),
    ccv_min_luminance_nits=0.726091,
    ccv_max_luminance_nits=5315.807728,
    ccv_avg_luminance_nits=47.547779,
    max_content_light_level_nits=5259,
    gainmap_stream_present=True,
    tmap_compatible_brand_present=True,
    scene_referred_export_metadata=False,
)
