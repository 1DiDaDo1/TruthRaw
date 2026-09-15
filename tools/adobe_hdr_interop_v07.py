#!/usr/bin/env python3
"""TruthRaw -> Adobe Lightroom HDR interoperability contract v0.7.

This module does not implement an AVIF/JXL/TIFF encoder.  It defines the
scientific boundary for feeding TruthRaw dynamic scene data into Adobe's HDR
editing/output path without pretending that exposure stacking or
counterfactual illumination created new measured evidence.

Core rules:
- exactly one physical source frame remains the evidence root;
- Adobe HDR may be enabled on a single-exposure raw without HDR Merge;
- no fake Merge-to-HDR metadata may be used to force activation;
- counterfactual and appearance-only values are rejected as scientific HDR
  signal;
- UNKNOWN and CENSORED samples do not become recovered radiance;
- HDR headroom is derived from real scene-linear values; a conservative
  evidence-supported headroom uses value - p95 uncertainty;
- a JPEG/AVIF/JXL gain map is presentation metadata only and may never write
  authority back into the Scientific Master;
- the Scientific Master itself keeps no fixed HDR EV ceiling.
"""
from __future__ import annotations

from dataclasses import dataclass
from enum import Enum
from math import isfinite, log2
from typing import Iterable, Optional, Tuple

from open_world_foundations_v01 import ContractError
from open_world_dynamic_authority_v05 import DynamicAuthority, DynamicAuthoritySample


class AdobeHdrRoute(str, Enum):
    """Interoperability routes that do not manufacture independent evidence."""

    SINGLE_EXPOSURE_RAW = "SINGLE_EXPOSURE_RAW"
    TRUTHRAW_SCENE_MASTER_RENDER = "TRUTHRAW_SCENE_MASTER_RENDER"


class AdobeHdrFormat(str, Enum):
    DNG = "DNG"
    AVIF = "AVIF"
    JPEG_XL = "JPEG_XL"
    TIFF = "TIFF"
    PSD = "PSD"
    PNG = "PNG"
    JPEG_GAIN_MAP = "JPEG_GAIN_MAP"


@dataclass(frozen=True)
class AdobeHdrHeadroom:
    sdr_white_scene_value: float
    nominal_max_scene_value: Optional[float]
    conservative_p95_supported_max_scene_value: Optional[float]
    nominal_headroom_ev: Optional[float]
    conservative_p95_supported_headroom_ev: Optional[float]
    contributing_sample_count: int
    ignored_unknown_count: int
    ignored_censored_count: int


@dataclass(frozen=True)
class AdobeHdrInteropPlan:
    schema: str
    route: AdobeHdrRoute
    output_format: AdobeHdrFormat
    physical_frame_count: int
    independent_evidence_count: int
    uses_exposure_merge: bool
    fake_hdr_merge_metadata: bool
    scientific_hdr_signal_uses_counterfactual: bool
    scientific_hdr_signal_uses_appearance_only: bool
    gain_map_is_presentation_only: bool
    scientific_master_writeback_from_adobe: bool
    scientific_master_fixed_hdr_limit_ev: Optional[float]
    adobe_hdr_activation_policy: str
    hdr_exchange_semantics: str
    headroom: AdobeHdrHeadroom


def _validate_sdr_white(value: float) -> float:
    v = float(value)
    if not isfinite(v) or v <= 0.0:
        raise ContractError("sdr_white_scene_value must be finite and > 0")
    return v


def derive_real_hdr_headroom(
    samples: Iterable[DynamicAuthoritySample],
    *,
    sdr_white_scene_value: float,
) -> AdobeHdrHeadroom:
    """Derive HDR headroom from scientific scene data, never from fake relight.

    Nominal headroom uses source-bound/reconstructed scene-linear estimates.
    Conservative headroom uses max(value - p95, 0), so an HDR claim survives the
    sample's p95 uncertainty band.  Censored highlights are intentionally not
    converted into exact radiance: a lower bound can prove clipping exists, but
    it cannot supply the missing highlight value required for a rendered HDR
    sample.
    """

    white = _validate_sdr_white(sdr_white_scene_value)
    nominal_values = []
    conservative_values = []
    unknown = 0
    censored = 0

    for sample in samples:
        sample.validate()
        if sample.authority in (DynamicAuthority.COUNTERFACTUAL, DynamicAuthority.APPEARANCE_ONLY):
            raise ContractError(
                f"{sample.authority.value} may not contribute to the scientific Adobe HDR signal"
            )
        if sample.authority is DynamicAuthority.UNKNOWN:
            unknown += 1
            continue
        if sample.authority is DynamicAuthority.CENSORED:
            censored += 1
            continue
        if sample.authority not in (
            DynamicAuthority.MEASURED,
            DynamicAuthority.CALIBRATED_ESTIMATE,
            DynamicAuthority.RECONSTRUCTED,
        ):
            continue

        value = float(sample.scene_linear_estimate)  # validate() guarantees presence
        p95 = float(sample.uncertainty_p95)  # validate() guarantees presence
        if value <= 0.0:
            continue
        nominal_values.append(value)
        conservative_values.append(max(value - p95, 0.0))

    nominal_max = max(nominal_values) if nominal_values else None
    conservative_max = max(conservative_values) if conservative_values else None

    def headroom(v: Optional[float]) -> Optional[float]:
        if v is None or v <= 0.0:
            return None
        return max(0.0, log2(v / white))

    return AdobeHdrHeadroom(
        sdr_white_scene_value=white,
        nominal_max_scene_value=nominal_max,
        conservative_p95_supported_max_scene_value=conservative_max,
        nominal_headroom_ev=headroom(nominal_max),
        conservative_p95_supported_headroom_ev=headroom(conservative_max),
        contributing_sample_count=len(nominal_values),
        ignored_unknown_count=unknown,
        ignored_censored_count=censored,
    )


def build_adobe_hdr_interop_plan(
    samples: Iterable[DynamicAuthoritySample],
    *,
    route: AdobeHdrRoute,
    output_format: AdobeHdrFormat,
    sdr_white_scene_value: float,
    physical_frame_count: int = 1,
    independent_evidence_count: int = 1,
    uses_exposure_merge: bool = False,
    fake_hdr_merge_metadata: bool = False,
) -> AdobeHdrInteropPlan:
    """Build a fail-closed Lightroom HDR bridge plan.

    SINGLE_EXPOSURE_RAW intentionally keeps the original/raw-DNG route and lets
    Adobe perform its own raw development with HDR editing enabled.  It is not a
    transport for TruthRaw's already reconstructed RGB Scene Master.

    TRUTHRAW_SCENE_MASTER_RENDER is the future rendered-HDR exchange route.  It
    can carry TruthRaw's scene result to Adobe after a separately validated
    color/HDR encoder, but it is no longer a raw sensor file.
    """

    if physical_frame_count != 1 or independent_evidence_count != 1:
        raise ContractError("Adobe HDR interoperability is bound to exactly one physical source frame/evidence item")
    if uses_exposure_merge:
        raise ContractError("exposure merge is outside the single-frame TruthRaw HDR evidence route")
    if fake_hdr_merge_metadata:
        raise ContractError("fake Merge-to-HDR metadata is forbidden")

    if route is AdobeHdrRoute.SINGLE_EXPOSURE_RAW:
        if output_format is not AdobeHdrFormat.DNG:
            raise ContractError("SINGLE_EXPOSURE_RAW route requires DNG")
        activation = (
            "Open the single-exposure raw/DNG in Lightroom and enable Edit in HDR mode; "
            "do not spoof HDR Merge metadata. Adobe may also auto-enable HDR for file classes it recognizes."
        )
        semantics = "ADOBE_RAW_PIPELINE_FROM_SINGLE_PHYSICAL_EXPOSURE"
    elif route is AdobeHdrRoute.TRUTHRAW_SCENE_MASTER_RENDER:
        if output_format is AdobeHdrFormat.DNG:
            raise ContractError("Scene Master rendered RGB may not masquerade as a raw DNG in v0.7")
        activation = (
            "Encode actual HDR pixel data in a validated Adobe-readable HDR exchange file; "
            "Lightroom then edits rendered HDR data rather than a raw mosaic."
        )
        semantics = "TRUTHRAW_RENDERED_SCENE_HDR_EXCHANGE"
    else:
        raise ContractError("unsupported Adobe HDR route")

    sample_tuple: Tuple[DynamicAuthoritySample, ...] = tuple(samples)
    headroom = derive_real_hdr_headroom(sample_tuple, sdr_white_scene_value=sdr_white_scene_value)

    return AdobeHdrInteropPlan(
        schema="TruthRawAdobeHdrInteropPlan/0.7",
        route=route,
        output_format=output_format,
        physical_frame_count=physical_frame_count,
        independent_evidence_count=independent_evidence_count,
        uses_exposure_merge=False,
        fake_hdr_merge_metadata=False,
        scientific_hdr_signal_uses_counterfactual=False,
        scientific_hdr_signal_uses_appearance_only=False,
        gain_map_is_presentation_only=True,
        scientific_master_writeback_from_adobe=False,
        scientific_master_fixed_hdr_limit_ev=None,
        adobe_hdr_activation_policy=activation,
        hdr_exchange_semantics=semantics,
        headroom=headroom,
    )
