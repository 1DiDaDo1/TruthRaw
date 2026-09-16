#!/usr/bin/env python3
"""TruthRaw-owned HDR projection authority contract v1.5.

This module establishes the first TruthRaw-owned scene-to-display projection
boundary. It deliberately keeps scientific scene authority separate from HDR
presentation encoding.

Laws:
- Scientific Master / Dynamic Authority hashes are immutable inputs.
- MEASURED, CALIBRATED_ESTIMATE and RECONSTRUCTED samples may be projected,
  while preserving their original authority and uncertainty.
- CENSORED samples retain bounds only; they do not acquire exact radiance.
- UNKNOWN samples remain unknown.
- COUNTERFACTUAL and APPEARANCE_ONLY cannot enter this scientific projection
  path as if they were Scientific Master values.
- PQ/display code values, MaxCLL and editor HDR-limit settings are presentation
  quantities and never sensor dynamic-range evidence.
- the scene representation remains open-ended; a finite HDR output is only a
  presentation window over it.
"""
from __future__ import annotations

import hashlib
import json
from dataclasses import dataclass
from enum import Enum
from math import isfinite, log2
from typing import Iterable, Optional, Sequence, Tuple

from tools.open_world_foundations_v01 import ContractError
from tools.open_world_dynamic_authority_v05 import DynamicAuthority, DynamicAuthoritySample

_SHA256_HEX = set("0123456789abcdef")
_SUPPORTED = {
    DynamicAuthority.MEASURED,
    DynamicAuthority.CALIBRATED_ESTIMATE,
    DynamicAuthority.RECONSTRUCTED,
}


def _check_sha256(value: str, name: str) -> str:
    v = value.lower()
    if len(v) != 64 or any(ch not in _SHA256_HEX for ch in v):
        raise ContractError(f"{name} must be a 64-character hexadecimal SHA-256")
    return v


def _canonical_hash(payload: dict) -> str:
    encoded = json.dumps(payload, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


def _positive_ev(value: float, reference_l0: float) -> Optional[float]:
    if not isfinite(value):
        raise ContractError("scene value must be finite")
    if not isfinite(reference_l0) or reference_l0 <= 0.0:
        raise ContractError("reference_l0 must be finite and > 0")
    if value <= 0.0:
        return None
    return log2(value / reference_l0)


class HdrPrimaries(str, Enum):
    P3_D65 = "P3_D65"
    REC709 = "REC709"
    REC2020 = "REC2020"


class HdrTransfer(str, Enum):
    SMPTE_ST_2084_PQ = "SMPTE_ST_2084_PQ"


@dataclass(frozen=True)
class ScientificProjectionBindingV15:
    source_dng_sha256: str
    source_cfa_sha256: str
    scientific_master_sha256: str
    dynamic_authority_sha256: str
    reference_l0: float

    def validate(self) -> "ScientificProjectionBindingV15":
        _check_sha256(self.source_dng_sha256, "source_dng_sha256")
        _check_sha256(self.source_cfa_sha256, "source_cfa_sha256")
        _check_sha256(self.scientific_master_sha256, "scientific_master_sha256")
        _check_sha256(self.dynamic_authority_sha256, "dynamic_authority_sha256")
        if not isfinite(self.reference_l0) or self.reference_l0 <= 0.0:
            raise ContractError("reference_l0 must be finite and > 0")
        return self


@dataclass(frozen=True)
class PiecewiseStopCurveV15:
    """Monotonic scene-EV -> display-EV mapping.

    Both axes are relative coordinates. The mapping is presentation-only and is
    intentionally not interpreted as a physical sensor or scene transfer law.
    Outside the supplied domain, endpoints are held constant and the projection
    reports presentation clipping.
    """

    points: Tuple[Tuple[float, float], ...]
    curve_id: str

    def validate(self) -> "PiecewiseStopCurveV15":
        if len(self.points) < 2:
            raise ContractError("at least two tone-curve points are required")
        if not self.curve_id.strip():
            raise ContractError("curve_id may not be empty")
        last_x = None
        last_y = None
        for x, y in self.points:
            if not isfinite(x) or not isfinite(y):
                raise ContractError("tone-curve coordinates must be finite")
            if last_x is not None and x <= last_x:
                raise ContractError("tone-curve scene EV coordinates must strictly increase")
            if last_y is not None and y < last_y:
                raise ContractError("tone-curve display EV coordinates must be monotonic")
            last_x, last_y = x, y
        return self

    @property
    def sha256(self) -> str:
        self.validate()
        return _canonical_hash({"schema": "TruthRawPiecewiseStopCurve/1.5", "curve_id": self.curve_id, "points": self.points})

    def map_ev(self, scene_ev: float) -> tuple[float, bool, bool]:
        self.validate()
        if not isfinite(scene_ev):
            raise ContractError("scene_ev must be finite")
        pts = self.points
        if scene_ev <= pts[0][0]:
            return pts[0][1], scene_ev < pts[0][0], False
        if scene_ev >= pts[-1][0]:
            return pts[-1][1], False, scene_ev > pts[-1][0]
        for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
            if x0 <= scene_ev <= x1:
                t = (scene_ev - x0) / (x1 - x0)
                return y0 + t * (y1 - y0), False, False
        raise AssertionError("unreachable")


@dataclass(frozen=True)
class HdrPresentationRequestV15:
    primaries: HdrPrimaries
    transfer: HdrTransfer
    bit_depth: int
    chroma: str
    reference_white_nits: float
    target_peak_nits: float
    curve: PiecewiseStopCurveV15
    adobe_interop_hdr_limit_ev: Optional[float] = None

    def validate(self) -> "HdrPresentationRequestV15":
        self.curve.validate()
        if self.bit_depth < 10:
            raise ContractError("TruthRaw HDR projection requires at least 10-bit output")
        if self.chroma not in ("4:4:4", "RGB"):
            raise ContractError("scientific HDR projection contract requires 4:4:4 or RGB transport")
        if not isfinite(self.reference_white_nits) or self.reference_white_nits <= 0.0:
            raise ContractError("reference_white_nits must be finite and > 0")
        if not isfinite(self.target_peak_nits) or not (self.reference_white_nits <= self.target_peak_nits <= 10000.0):
            raise ContractError("target_peak_nits must be between reference white and the 10000-nit PQ ceiling")
        if self.adobe_interop_hdr_limit_ev is not None and not isfinite(self.adobe_interop_hdr_limit_ev):
            raise ContractError("Adobe HDR-limit metadata must be finite when supplied")
        max_display_ev = self.curve.points[-1][1]
        curve_peak = self.reference_white_nits * (2.0 ** max_display_ev)
        if curve_peak > self.target_peak_nits * (1.0 + 1e-12):
            raise ContractError("tone curve requests display luminance above target_peak_nits")
        return self


@dataclass(frozen=True)
class ScientificHeadroomSummaryV15:
    supported_sample_count: int
    censored_sample_count: int
    unknown_sample_count: int
    signed_nonpositive_supported_count: int
    nominal_supported_peak_ev: Optional[float]
    conservative_p95_supported_peak_ev: Optional[float]
    censored_lower_bound_peak_ev: Optional[float]


def summarize_scientific_headroom(samples: Iterable[DynamicAuthoritySample], reference_l0: float) -> ScientificHeadroomSummaryV15:
    if not isfinite(reference_l0) or reference_l0 <= 0.0:
        raise ContractError("reference_l0 must be finite and > 0")
    supported = censored = unknown = nonpositive = 0
    nominal_peak = conservative_peak = censored_peak = None

    for sample in samples:
        sample.validate()
        if sample.authority in (DynamicAuthority.COUNTERFACTUAL, DynamicAuthority.APPEARANCE_ONLY):
            raise ContractError("counterfactual/appearance samples cannot define scientific HDR headroom")
        if sample.authority in _SUPPORTED:
            supported += 1
            value = float(sample.scene_linear_estimate)
            ev = _positive_ev(value, reference_l0)
            if ev is None:
                nonpositive += 1
                continue
            nominal_peak = ev if nominal_peak is None else max(nominal_peak, ev)
            uncertainty = float(sample.uncertainty_p95 or 0.0)
            conservative_value = value - uncertainty
            conservative_ev = _positive_ev(conservative_value, reference_l0)
            if conservative_ev is not None:
                conservative_peak = conservative_ev if conservative_peak is None else max(conservative_peak, conservative_ev)
        elif sample.authority is DynamicAuthority.CENSORED:
            censored += 1
            if sample.censor_bound is not None:
                bound_ev = _positive_ev(float(sample.censor_bound), reference_l0)
                if bound_ev is not None:
                    censored_peak = bound_ev if censored_peak is None else max(censored_peak, bound_ev)
        elif sample.authority is DynamicAuthority.UNKNOWN:
            unknown += 1

    return ScientificHeadroomSummaryV15(
        supported_sample_count=supported,
        censored_sample_count=censored,
        unknown_sample_count=unknown,
        signed_nonpositive_supported_count=nonpositive,
        nominal_supported_peak_ev=nominal_peak,
        conservative_p95_supported_peak_ev=conservative_peak,
        censored_lower_bound_peak_ev=censored_peak,
    )


def pq_code_from_nits(nits: float, bit_depth: int = 10) -> int:
    """Encode absolute luminance with the SMPTE ST 2084/PQ OETF.

    This is a scalar luminance utility, not a complete RGB/YUV AVIF encoder.
    """
    if not isfinite(nits) or not (0.0 <= nits <= 10000.0):
        raise ContractError("PQ luminance must be finite and within 0..10000 nits")
    if bit_depth < 10 or bit_depth > 16:
        raise ContractError("PQ bit depth must be between 10 and 16")
    m1 = 2610.0 / 16384.0
    m2 = 2523.0 / 32.0
    c1 = 3424.0 / 4096.0
    c2 = 2413.0 / 128.0
    c3 = 2392.0 / 128.0
    L = nits / 10000.0
    if L <= 0.0:
        encoded = 0.0
    else:
        p = L ** m1
        encoded = ((c1 + c2 * p) / (1.0 + c3 * p)) ** m2
    ceiling = (1 << bit_depth) - 1
    return max(0, min(ceiling, int(round(encoded * ceiling))))


@dataclass(frozen=True)
class ProjectionSampleResultV15:
    authority: DynamicAuthority
    nominal_scene_ev: Optional[float]
    conservative_p95_scene_ev: Optional[float]
    censor_bound_ev: Optional[float]
    display_ev: Optional[float]
    display_nits: Optional[float]
    pq_code: Optional[int]
    presentation_clipped_low: bool
    presentation_clipped_high: bool
    exact_projection_allowed: bool
    creates_new_sensor_evidence: bool = False
    upgrades_scientific_authority: bool = False


def project_scientific_sample(
    sample: DynamicAuthoritySample,
    binding: ScientificProjectionBindingV15,
    request: HdrPresentationRequestV15,
) -> ProjectionSampleResultV15:
    binding.validate()
    request.validate()
    sample.validate()

    if sample.authority in (DynamicAuthority.COUNTERFACTUAL, DynamicAuthority.APPEARANCE_ONLY):
        raise ContractError("counterfactual/appearance state requires a separate presentation layer")

    if sample.authority is DynamicAuthority.CENSORED:
        bound_ev = None if sample.censor_bound is None else _positive_ev(float(sample.censor_bound), binding.reference_l0)
        return ProjectionSampleResultV15(sample.authority, None, None, bound_ev, None, None, None, False, False, False)
    if sample.authority is DynamicAuthority.UNKNOWN:
        return ProjectionSampleResultV15(sample.authority, None, None, None, None, None, None, False, False, False)
    if sample.authority not in _SUPPORTED:
        raise ContractError("unsupported authority for scientific HDR projection")

    value = float(sample.scene_linear_estimate)
    nominal_ev = _positive_ev(value, binding.reference_l0)
    uncertainty = float(sample.uncertainty_p95 or 0.0)
    conservative_ev = _positive_ev(value - uncertainty, binding.reference_l0)
    if nominal_ev is None:
        return ProjectionSampleResultV15(sample.authority, None, conservative_ev, None, None, None, None, False, False, False)

    display_ev, clipped_low, clipped_high = request.curve.map_ev(nominal_ev)
    nits = request.reference_white_nits * (2.0 ** display_ev)
    if nits > request.target_peak_nits:
        nits = request.target_peak_nits
        clipped_high = True
    nits = max(0.0, nits)
    code = pq_code_from_nits(nits, request.bit_depth)
    return ProjectionSampleResultV15(
        authority=sample.authority,
        nominal_scene_ev=nominal_ev,
        conservative_p95_scene_ev=conservative_ev,
        censor_bound_ev=None,
        display_ev=display_ev,
        display_nits=nits,
        pq_code=code,
        presentation_clipped_low=clipped_low,
        presentation_clipped_high=clipped_high,
        exact_projection_allowed=True,
    )


@dataclass(frozen=True)
class HdrProjectionManifestV15:
    binding: ScientificProjectionBindingV15
    request: HdrPresentationRequestV15
    headroom: ScientificHeadroomSummaryV15
    output_file_sha256: Optional[str] = None
    output_maxcll_nits: Optional[float] = None

    def validate(self) -> "HdrProjectionManifestV15":
        self.binding.validate()
        self.request.validate()
        if self.output_file_sha256 is not None:
            _check_sha256(self.output_file_sha256, "output_file_sha256")
        if self.output_maxcll_nits is not None:
            if not isfinite(self.output_maxcll_nits) or not (0.0 <= self.output_maxcll_nits <= 10000.0):
                raise ContractError("output MaxCLL must be within the PQ presentation range")
        return self

    @property
    def manifest_sha256(self) -> str:
        self.validate()
        return _canonical_hash(
            {
                "schema": "TruthRawHdrProjectionManifest/1.5",
                "science": {
                    "source_dng_sha256": self.binding.source_dng_sha256.lower(),
                    "source_cfa_sha256": self.binding.source_cfa_sha256.lower(),
                    "scientific_master_sha256": self.binding.scientific_master_sha256.lower(),
                    "dynamic_authority_sha256": self.binding.dynamic_authority_sha256.lower(),
                    "reference_l0": self.binding.reference_l0,
                    "headroom": self.headroom.__dict__,
                },
                "presentation": {
                    "primaries": self.request.primaries.value,
                    "transfer": self.request.transfer.value,
                    "bit_depth": self.request.bit_depth,
                    "chroma": self.request.chroma,
                    "reference_white_nits": self.request.reference_white_nits,
                    "target_peak_nits": self.request.target_peak_nits,
                    "curve_id": self.request.curve.curve_id,
                    "curve_sha256": self.request.curve.sha256,
                    "adobe_interop_hdr_limit_ev": self.request.adobe_interop_hdr_limit_ev,
                    "scientific_dynamic_range_from_presentation_metadata": False,
                },
                "output": {
                    "output_file_sha256": None if self.output_file_sha256 is None else self.output_file_sha256.lower(),
                    "output_maxcll_nits": self.output_maxcll_nits,
                },
                "authority": {
                    "source_evidence_immutable": True,
                    "scientific_master_writeback_from_presentation": False,
                    "presentation_can_upgrade_authority": False,
                    "censored_exact_radiance_recovery": False,
                    "fixed_scene_dynamic_range_ceiling_ev": None,
                },
            }
        )
