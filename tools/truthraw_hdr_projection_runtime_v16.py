#!/usr/bin/env python3
"""TruthRaw streaming HDR projection runtime v1.6.

This module turns the v1.5 authority boundary into a bounded, deterministic
per-pixel RGB->PQ projection runtime without changing scientific authority.

It is deliberately *not* an AVIF encoder and it does not manufacture a
Scientific Master. A real render is admissible only when the caller supplies
an exact Scientific Master hash, Dynamic Authority Field hash, source DNG/CFA
hashes and a hash-bound colorimetric transform.

Core laws:
- source/Scientific-Master/Dynamic-Authority lineage is immutable input;
- only MEASURED, CALIBRATED_ESTIMATE and RECONSTRUCTED RGB channels can produce
  an exact scientific projection sample;
- CENSORED and UNKNOWN channels are withheld from exact projection;
- COUNTERFACTUAL and APPEARANCE_ONLY are rejected from this scientific path;
- the tone curve acts on display luminance only and never upgrades authority;
- negative/out-of-gamut and peak clipping are explicit presentation events;
- uncertainty travels with the projected record and is never converted into
  synthetic scene detail;
- finite PQ output is only a presentation window over an open-ended scene.
"""
from __future__ import annotations

import hashlib
import json
from dataclasses import dataclass
from math import isfinite
from typing import Iterable, Optional, Sequence, Tuple

from tools.open_world_foundations_v01 import ContractError
from tools.open_world_dynamic_authority_v05 import DynamicAuthority
from tools.truthraw_hdr_projection_v15 import (
    HdrPresentationRequestV15,
    HdrPrimaries,
    ScientificProjectionBindingV15,
    pq_code_from_nits,
)

_SUPPORTED = {
    DynamicAuthority.MEASURED,
    DynamicAuthority.CALIBRATED_ESTIMATE,
    DynamicAuthority.RECONSTRUCTED,
}

_LUMA_WEIGHTS = {
    HdrPrimaries.REC709: (0.2126, 0.7152, 0.0722),
    HdrPrimaries.P3_D65: (0.2289745640697488, 0.6917385218365064, 0.07928691409374483),
    HdrPrimaries.REC2020: (0.2627, 0.6780, 0.0593),
}


def _canonical_bytes(payload: dict) -> bytes:
    return json.dumps(payload, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("utf-8")


def _check_sha256(value: str, name: str) -> str:
    v = value.lower()
    if len(v) != 64 or any(ch not in "0123456789abcdef" for ch in v):
        raise ContractError(f"{name} must be a 64-character hexadecimal SHA-256")
    return v


def _matrix_vector(m: Tuple[Tuple[float, float, float], ...], v: Tuple[float, float, float]) -> Tuple[float, float, float]:
    return tuple(sum(row[j] * v[j] for j in range(3)) for row in m)  # type: ignore[return-value]


def _dot(a: Tuple[float, float, float], b: Tuple[float, float, float]) -> float:
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


@dataclass(frozen=True)
class ColorimetricProjectionBindingV16:
    """Hash-bound matrix from Scientific-Master RGB to target linear RGB.

    The matrix may be source-metadata-bound or physically calibrated; v1.6 does
    not promote its color authority. `authority_label` is provenance, not a
    quality score.
    """

    transform_sha256: str
    target_primaries: HdrPrimaries
    matrix_rgb_to_target: Tuple[Tuple[float, float, float], Tuple[float, float, float], Tuple[float, float, float]]
    authority_label: str

    def validate(self) -> "ColorimetricProjectionBindingV16":
        _check_sha256(self.transform_sha256, "transform_sha256")
        if not self.authority_label.strip():
            raise ContractError("color authority label may not be empty")
        if len(self.matrix_rgb_to_target) != 3 or any(len(row) != 3 for row in self.matrix_rgb_to_target):
            raise ContractError("color transform must be 3x3")
        for row in self.matrix_rgb_to_target:
            for value in row:
                if not isfinite(float(value)):
                    raise ContractError("color transform matrix must be finite")
        return self


@dataclass(frozen=True)
class ScientificRgbPixelV16:
    rgb: Tuple[float, float, float]
    authority: Tuple[DynamicAuthority, DynamicAuthority, DynamicAuthority]
    uncertainty_p95: Tuple[Optional[float], Optional[float], Optional[float]]

    def validate(self) -> "ScientificRgbPixelV16":
        if len(self.rgb) != 3 or len(self.authority) != 3 or len(self.uncertainty_p95) != 3:
            raise ContractError("RGB pixel requires exactly three channels")
        for value in self.rgb:
            if not isfinite(float(value)):
                raise ContractError("scientific RGB values must be finite")
        for authority, uncertainty in zip(self.authority, self.uncertainty_p95):
            if authority in (DynamicAuthority.COUNTERFACTUAL, DynamicAuthority.APPEARANCE_ONLY):
                raise ContractError("counterfactual/appearance channels cannot enter scientific HDR projection")
            if authority in _SUPPORTED:
                if uncertainty is None or not isfinite(float(uncertainty)) or float(uncertainty) < 0.0:
                    raise ContractError("supported scientific channels require finite nonnegative p95 uncertainty")
            elif authority in (DynamicAuthority.CENSORED, DynamicAuthority.UNKNOWN):
                if uncertainty is not None and (not isfinite(float(uncertainty)) or float(uncertainty) < 0.0):
                    raise ContractError("uncertainty must be finite/nonnegative when present")
            else:
                raise ContractError("unsupported authority class")
        return self

    @property
    def exact_projection_allowed(self) -> bool:
        self.validate()
        return all(a in _SUPPORTED for a in self.authority)


@dataclass(frozen=True)
class ProjectionRuntimeBindingV16:
    science: ScientificProjectionBindingV15
    color: ColorimetricProjectionBindingV16

    def validate(self, request: HdrPresentationRequestV15) -> "ProjectionRuntimeBindingV16":
        self.science.validate()
        self.color.validate()
        request.validate()
        if self.color.target_primaries is not request.primaries:
            raise ContractError("color transform target primaries must match HDR request primaries")
        return self


@dataclass(frozen=True)
class ProjectedRgbPixelV16:
    source_authority: Tuple[DynamicAuthority, DynamicAuthority, DynamicAuthority]
    source_uncertainty_p95: Tuple[Optional[float], Optional[float], Optional[float]]
    target_linear_rgb: Optional[Tuple[float, float, float]]
    scene_luminance: Optional[float]
    scene_ev: Optional[float]
    display_luminance_nits: Optional[float]
    display_rgb_nits: Optional[Tuple[float, float, float]]
    pq_rgb_codes: Optional[Tuple[int, int, int]]
    presentation_clipped_low: bool
    presentation_clipped_high: bool
    negative_gamut_clip: bool
    channel_peak_clip: bool
    exact_projection_allowed: bool
    creates_new_sensor_evidence: bool = False
    upgrades_scientific_authority: bool = False
    scientific_master_writeback_allowed: bool = False


def project_rgb_pixel_v16(
    pixel: ScientificRgbPixelV16,
    binding: ProjectionRuntimeBindingV16,
    request: HdrPresentationRequestV15,
) -> ProjectedRgbPixelV16:
    pixel.validate()
    binding.validate(request)

    if not pixel.exact_projection_allowed:
        return ProjectedRgbPixelV16(
            source_authority=pixel.authority,
            source_uncertainty_p95=pixel.uncertainty_p95,
            target_linear_rgb=None,
            scene_luminance=None,
            scene_ev=None,
            display_luminance_nits=None,
            display_rgb_nits=None,
            pq_rgb_codes=None,
            presentation_clipped_low=False,
            presentation_clipped_high=False,
            negative_gamut_clip=False,
            channel_peak_clip=False,
            exact_projection_allowed=False,
        )

    target_rgb = _matrix_vector(binding.color.matrix_rgb_to_target, pixel.rgb)
    weights = _LUMA_WEIGHTS[request.primaries]
    scene_y = _dot(weights, target_rgb)
    if not isfinite(scene_y) or scene_y <= 0.0:
        # Signed color coordinates are valid scientific values, but non-positive
        # display luminance has no positive-light PQ projection coordinate.
        return ProjectedRgbPixelV16(
            source_authority=pixel.authority,
            source_uncertainty_p95=pixel.uncertainty_p95,
            target_linear_rgb=target_rgb,
            scene_luminance=scene_y,
            scene_ev=None,
            display_luminance_nits=None,
            display_rgb_nits=None,
            pq_rgb_codes=None,
            presentation_clipped_low=False,
            presentation_clipped_high=False,
            negative_gamut_clip=any(v < 0.0 for v in target_rgb),
            channel_peak_clip=False,
            exact_projection_allowed=False,
        )

    # reference_l0 is explicitly the scene-luminance gauge for this bound
    # Scientific Master, not source black/white level and not display white.
    from math import log2

    scene_ev = log2(scene_y / binding.science.reference_l0)
    display_ev, clipped_low, clipped_high = request.curve.map_ev(scene_ev)
    display_y = request.reference_white_nits * (2.0 ** display_ev)
    if display_y > request.target_peak_nits:
        display_y = request.target_peak_nits
        clipped_high = True

    scale = display_y / scene_y
    rgb_nits = tuple(v * scale for v in target_rgb)

    negative_clip = any(v < 0.0 for v in rgb_nits)
    if negative_clip:
        clipped = tuple(max(0.0, v) for v in rgb_nits)
        clipped_y = _dot(weights, clipped)
        if clipped_y <= 0.0:
            return ProjectedRgbPixelV16(
                pixel.authority,
                pixel.uncertainty_p95,
                target_rgb,
                scene_y,
                scene_ev,
                display_y,
                None,
                None,
                clipped_low,
                clipped_high,
                True,
                False,
                False,
            )
        # Preserve requested display luminance after presentation-only gamut clip.
        rescale = display_y / clipped_y
        rgb_nits = tuple(v * rescale for v in clipped)

    peak_clip = any(v > request.target_peak_nits for v in rgb_nits)
    if peak_clip:
        rgb_nits = tuple(min(request.target_peak_nits, max(0.0, v)) for v in rgb_nits)
        clipped_high = True

    pq = tuple(pq_code_from_nits(v, request.bit_depth) for v in rgb_nits)
    return ProjectedRgbPixelV16(
        source_authority=pixel.authority,
        source_uncertainty_p95=pixel.uncertainty_p95,
        target_linear_rgb=target_rgb,
        scene_luminance=scene_y,
        scene_ev=scene_ev,
        display_luminance_nits=display_y,
        display_rgb_nits=rgb_nits,  # type: ignore[arg-type]
        pq_rgb_codes=pq,  # type: ignore[arg-type]
        presentation_clipped_low=clipped_low,
        presentation_clipped_high=clipped_high,
        negative_gamut_clip=negative_clip,
        channel_peak_clip=peak_clip,
        exact_projection_allowed=True,
    )


@dataclass(frozen=True)
class ProjectionRuntimeSummaryV16:
    schema: str
    pixel_count: int
    exact_projected_count: int
    withheld_authority_count: int
    nonpositive_luminance_count: int
    negative_gamut_clip_count: int
    channel_peak_clip_count: int
    presentation_low_clip_count: int
    presentation_high_clip_count: int
    pq_code_max: Optional[int]
    content_sha256: str
    scientific_master_writeback_allowed: bool
    fixed_scene_dynamic_range_ceiling_ev: Optional[float]


class StreamingHdrProjectionAccumulatorV16:
    """Hash deterministic projected records in global raster order.

    `push_many` boundaries are execution details only. Feeding the same pixel
    sequence in different chunk sizes produces the same content hash.
    """

    def __init__(self, binding: ProjectionRuntimeBindingV16, request: HdrPresentationRequestV15) -> None:
        self.binding = binding.validate(request)
        self.request = request
        self._hash = hashlib.sha256()
        self._n = 0
        self._exact = 0
        self._withheld = 0
        self._nonpositive = 0
        self._neg = 0
        self._peak = 0
        self._low = 0
        self._high = 0
        self._pq_max: Optional[int] = None

    def push(self, pixel: ScientificRgbPixelV16) -> ProjectedRgbPixelV16:
        result = project_rgb_pixel_v16(pixel, self.binding, self.request)
        record = {
            "i": self._n,
            "authority": [a.value for a in result.source_authority],
            "uncertainty_p95": result.source_uncertainty_p95,
            "target_linear_rgb": result.target_linear_rgb,
            "scene_luminance": result.scene_luminance,
            "scene_ev": result.scene_ev,
            "display_luminance_nits": result.display_luminance_nits,
            "display_rgb_nits": result.display_rgb_nits,
            "pq_rgb_codes": result.pq_rgb_codes,
            "clip_low": result.presentation_clipped_low,
            "clip_high": result.presentation_clipped_high,
            "negative_gamut_clip": result.negative_gamut_clip,
            "channel_peak_clip": result.channel_peak_clip,
            "exact_projection_allowed": result.exact_projection_allowed,
        }
        self._hash.update(_canonical_bytes(record) + b"\n")
        self._n += 1
        if result.exact_projection_allowed and result.pq_rgb_codes is not None:
            self._exact += 1
            local_max = max(result.pq_rgb_codes)
            self._pq_max = local_max if self._pq_max is None else max(self._pq_max, local_max)
        else:
            self._withheld += 1
            if result.scene_luminance is not None and result.scene_luminance <= 0.0:
                self._nonpositive += 1
        self._neg += int(result.negative_gamut_clip)
        self._peak += int(result.channel_peak_clip)
        self._low += int(result.presentation_clipped_low)
        self._high += int(result.presentation_clipped_high)
        return result

    def push_many(self, pixels: Iterable[ScientificRgbPixelV16]) -> None:
        for pixel in pixels:
            self.push(pixel)

    def finish(self) -> ProjectionRuntimeSummaryV16:
        if self._n <= 0:
            raise ContractError("projection runtime cannot finalize an empty image")
        return ProjectionRuntimeSummaryV16(
            schema="TruthRawHdrProjectionRuntime/1.6",
            pixel_count=self._n,
            exact_projected_count=self._exact,
            withheld_authority_count=self._withheld,
            nonpositive_luminance_count=self._nonpositive,
            negative_gamut_clip_count=self._neg,
            channel_peak_clip_count=self._peak,
            presentation_low_clip_count=self._low,
            presentation_high_clip_count=self._high,
            pq_code_max=self._pq_max,
            content_sha256=self._hash.hexdigest(),
            scientific_master_writeback_allowed=False,
            fixed_scene_dynamic_range_ceiling_ev=None,
        )
