#!/usr/bin/env python3
"""TruthRaw open-world Dynamic Authority Field v0.5.

The represented scene may be radiometrically open-ended, but evidence is not.
This module makes that distinction machine-readable per scene sample.

Core rules:
- no fixed 0..1 or fixed-EV scene container is imposed;
- signed scene-linear estimators remain representable;
- TruthRange T=log2(L/L0) exists only for positive light values and is never
  silently clamped;
- measured, calibrated, reconstructed, censored, unknown, counterfactual and
  appearance-only states remain distinct;
- counterfactual/appearance fields cannot be written back as Scientific Master
  authority;
- streaming content hashing is independent of chunk size when the same samples
  are emitted in the same global raster order;
- hardware planning changes only tile size/concurrency, never scientific policy.

"Open-ended" is an architectural/radiometric statement, not a claim that a
finite sensor measured infinite dynamic range or that finite numeric storage is
mathematically infinite.
"""
from __future__ import annotations

import hashlib
import json
from dataclasses import dataclass
from enum import Enum
from math import isfinite, log2
from typing import Dict, Iterable, Optional, Sequence, Tuple

from open_world_foundations_v01 import ContractError


_SHA256_HEX = set("0123456789abcdef")


def _check_sha256(value: str, field_name: str) -> str:
    v = value.lower()
    if len(v) != 64 or any(ch not in _SHA256_HEX for ch in v):
        raise ContractError(f"{field_name} must be a 64-character hexadecimal SHA-256")
    return v


def _unit(value: float, field_name: str) -> None:
    if not isfinite(value) or not (0.0 <= value <= 1.0):
        raise ContractError(f"{field_name} must be finite and within [0, 1]")


def _canonical_bytes(payload: dict) -> bytes:
    return json.dumps(payload, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("utf-8")


def _canonical_hash(payload: dict) -> str:
    return hashlib.sha256(_canonical_bytes(payload)).hexdigest()


class DynamicAuthority(str, Enum):
    MEASURED = "MEASURED"
    CALIBRATED_ESTIMATE = "CALIBRATED_ESTIMATE"
    RECONSTRUCTED = "RECONSTRUCTED"
    CENSORED = "CENSORED"
    UNKNOWN = "UNKNOWN"
    COUNTERFACTUAL = "COUNTERFACTUAL"
    APPEARANCE_ONLY = "APPEARANCE_ONLY"


class DynamicFieldPurpose(str, Enum):
    SCIENTIFIC_MASTER = "SCIENTIFIC_MASTER"
    COUNTERFACTUAL_VIEW = "COUNTERFACTUAL_VIEW"
    APPEARANCE_VIEW = "APPEARANCE_VIEW"


class CensoringKind(str, Enum):
    HIGHLIGHT_SATURATION = "HIGHLIGHT_SATURATION"
    SHADOW_FLOOR = "SHADOW_FLOOR"
    RANGE_LIMIT = "RANGE_LIMIT"
    OTHER = "OTHER"


@dataclass(frozen=True)
class DynamicAuthoritySample:
    authority: DynamicAuthority
    scene_linear_estimate: Optional[float]
    uncertainty_p95: Optional[float] = None
    support: float = 0.0
    source_sample_index: Optional[int] = None
    censoring_kind: Optional[CensoringKind] = None
    censor_bound: Optional[float] = None
    parent_record_id: Optional[str] = None

    def validate(self) -> "DynamicAuthoritySample":
        _unit(float(self.support), "support")
        if self.scene_linear_estimate is not None and not isfinite(float(self.scene_linear_estimate)):
            raise ContractError("scene_linear_estimate must be finite when present")
        if self.uncertainty_p95 is not None:
            if not isfinite(float(self.uncertainty_p95)) or float(self.uncertainty_p95) < 0.0:
                raise ContractError("uncertainty_p95 must be finite and nonnegative")
        if self.censor_bound is not None and not isfinite(float(self.censor_bound)):
            raise ContractError("censor_bound must be finite when present")
        if self.source_sample_index is not None and self.source_sample_index < 0:
            raise ContractError("source_sample_index must be nonnegative")

        if self.authority is DynamicAuthority.MEASURED:
            if self.scene_linear_estimate is None or self.uncertainty_p95 is None or self.source_sample_index is None:
                raise ContractError("MEASURED requires value, p95 uncertainty and source_sample_index")
            if self.support <= 0.0:
                raise ContractError("MEASURED requires positive support")
            if self.censoring_kind is not None or self.parent_record_id is not None:
                raise ContractError("MEASURED may not descend from censored/counterfactual state")
        elif self.authority is DynamicAuthority.CALIBRATED_ESTIMATE:
            if self.scene_linear_estimate is None or self.uncertainty_p95 is None or self.source_sample_index is None:
                raise ContractError("CALIBRATED_ESTIMATE requires source-bound value and uncertainty")
            if self.support <= 0.0:
                raise ContractError("CALIBRATED_ESTIMATE requires positive support")
        elif self.authority is DynamicAuthority.RECONSTRUCTED:
            if self.scene_linear_estimate is None or self.uncertainty_p95 is None:
                raise ContractError("RECONSTRUCTED requires value and p95 uncertainty")
            if self.support <= 0.0:
                raise ContractError("RECONSTRUCTED requires positive support")
        elif self.authority is DynamicAuthority.CENSORED:
            if self.censoring_kind is None:
                raise ContractError("CENSORED requires an explicit censoring_kind")
            if self.support != 0.0:
                raise ContractError("CENSORED may carry a bound but not positive recovered-value support")
        elif self.authority is DynamicAuthority.UNKNOWN:
            if self.support != 0.0:
                raise ContractError("UNKNOWN may not carry positive support")
            if self.censoring_kind is not None:
                raise ContractError("use CENSORED when a censoring mechanism is known")
        elif self.authority in (DynamicAuthority.COUNTERFACTUAL, DynamicAuthority.APPEARANCE_ONLY):
            if self.scene_linear_estimate is None or not self.parent_record_id:
                raise ContractError(f"{self.authority.value} requires a value and parent_record_id")
            if self.censoring_kind is not None:
                raise ContractError("counterfactual/appearance samples may not masquerade as source censoring")
        return self


def truthrange_ev(scene_linear_value: float, reference_l0: float) -> Optional[float]:
    """Return T=log2(L/L0) for positive light, without any EV clamp.

    Signed/zero estimators stay in scene-linear form and have no positive-light
    TruthRange coordinate. This avoids turning numerical negative estimators
    into fictitious physical negative radiance.
    """
    if not isfinite(scene_linear_value):
        raise ContractError("scene_linear_value must be finite")
    if not isfinite(reference_l0) or reference_l0 <= 0.0:
        raise ContractError("reference_l0 must be finite and > 0")
    if scene_linear_value <= 0.0:
        return None
    return log2(scene_linear_value / reference_l0)


@dataclass(frozen=True)
class DynamicAuthorityFieldBinding:
    frame_id: str
    width: int
    height: int
    reference_l0: float
    source_evidence_sha256: str
    scientific_master_sha256: str
    purpose: DynamicFieldPurpose = DynamicFieldPurpose.SCIENTIFIC_MASTER

    def validate(self) -> "DynamicAuthorityFieldBinding":
        if not self.frame_id.strip():
            raise ContractError("frame_id may not be empty")
        if self.width <= 0 or self.height <= 0:
            raise ContractError("field dimensions must be positive")
        if not isfinite(self.reference_l0) or self.reference_l0 <= 0.0:
            raise ContractError("reference_l0 must be finite and > 0")
        _check_sha256(self.source_evidence_sha256, "source_evidence_sha256")
        _check_sha256(self.scientific_master_sha256, "scientific_master_sha256")
        return self

    @property
    def scientific_policy_sha256(self) -> str:
        self.validate()
        return _canonical_hash(
            {
                "schema": "TruthRawDynamicAuthorityPolicy/0.5",
                "frame_id": self.frame_id,
                "width": self.width,
                "height": self.height,
                "reference_l0": self.reference_l0,
                "source_evidence_sha256": self.source_evidence_sha256.lower(),
                "scientific_master_sha256": self.scientific_master_sha256.lower(),
                "purpose": self.purpose.value,
                "truthrange": "T=log2(L/L0), positive-light only, no clamp",
                "signed_scene_linear_preserved": True,
                "fixed_dynamic_range_limit_ev": None,
                "source_evidence_immutable": True,
                "authority_upgrade_without_new_evidence": False,
            }
        )

    def validate_sample_authority(self, sample: DynamicAuthoritySample) -> None:
        sample.validate()
        if self.purpose is DynamicFieldPurpose.SCIENTIFIC_MASTER:
            if sample.authority in (DynamicAuthority.COUNTERFACTUAL, DynamicAuthority.APPEARANCE_ONLY):
                raise ContractError("counterfactual/appearance samples cannot enter the Scientific Master field")
        elif self.purpose is DynamicFieldPurpose.COUNTERFACTUAL_VIEW:
            if sample.authority is not DynamicAuthority.COUNTERFACTUAL:
                raise ContractError("COUNTERFACTUAL_VIEW contains only counterfactual output samples")
        elif self.purpose is DynamicFieldPurpose.APPEARANCE_VIEW:
            if sample.authority is not DynamicAuthority.APPEARANCE_ONLY:
                raise ContractError("APPEARANCE_VIEW contains only appearance-only output samples")


@dataclass(frozen=True)
class DynamicRangeSlice:
    minimum_ev: Optional[float]
    maximum_ev: Optional[float]
    span_ev: Optional[float]
    positive_sample_count: int


@dataclass(frozen=True)
class DynamicAuthorityFieldSummary:
    schema: str
    frame_id: str
    width: int
    height: int
    purpose: DynamicFieldPurpose
    reference_l0: float
    sample_count: int
    counts: Dict[str, int]
    representation: DynamicRangeSlice
    evidence_supported: DynamicRangeSlice
    reconstruction_supported: DynamicRangeSlice
    scientific_supported: DynamicRangeSlice
    signed_nonpositive_estimate_count: int
    scientific_master_write_allowed: bool
    fixed_dynamic_range_limit_ev: Optional[float]
    content_sha256: str
    scientific_policy_sha256: str


class StreamingDynamicAuthorityAccumulator:
    """Bounded-memory, raster-order accumulator for a Dynamic Authority Field.

    Chunk boundaries are deliberately excluded from the content digest. Feeding
    the same global sample sequence as 32-pixel, 256-pixel or 4096-pixel chunks
    yields the same field content SHA and scientific summary.
    """

    def __init__(self, binding: DynamicAuthorityFieldBinding) -> None:
        self.binding = binding.validate()
        self._next = 0
        self._hash = hashlib.sha256()
        self._counts = {a.value: 0 for a in DynamicAuthority}
        self._ranges = {
            "representation": [None, None, 0],
            "evidence": [None, None, 0],
            "reconstruction": [None, None, 0],
            "scientific": [None, None, 0],
        }
        self._nonpositive = 0

    @staticmethod
    def _update_range(slot: list, ev: Optional[float]) -> None:
        if ev is None:
            return
        slot[0] = ev if slot[0] is None else min(slot[0], ev)
        slot[1] = ev if slot[1] is None else max(slot[1], ev)
        slot[2] += 1

    def append_chunk(self, start_linear_index: int, samples: Sequence[DynamicAuthoritySample]) -> None:
        if start_linear_index != self._next:
            raise ContractError("Dynamic Authority chunks must be contiguous in global raster order")
        total = self.binding.width * self.binding.height
        if self._next + len(samples) > total:
            raise ContractError("Dynamic Authority chunk exceeds declared field sample count")

        for sample in samples:
            self.binding.validate_sample_authority(sample)
            idx = self._next
            self._counts[sample.authority.value] += 1
            ev = None
            if sample.scene_linear_estimate is not None:
                ev = truthrange_ev(float(sample.scene_linear_estimate), self.binding.reference_l0)
                if ev is None:
                    self._nonpositive += 1
            self._update_range(self._ranges["representation"], ev)
            if sample.authority in (DynamicAuthority.MEASURED, DynamicAuthority.CALIBRATED_ESTIMATE):
                self._update_range(self._ranges["evidence"], ev)
                self._update_range(self._ranges["scientific"], ev)
            elif sample.authority is DynamicAuthority.RECONSTRUCTED:
                self._update_range(self._ranges["reconstruction"], ev)
                self._update_range(self._ranges["scientific"], ev)

            payload = {
                "i": idx,
                "authority": sample.authority.value,
                "scene_linear_estimate": sample.scene_linear_estimate,
                "uncertainty_p95": sample.uncertainty_p95,
                "support": sample.support,
                "source_sample_index": sample.source_sample_index,
                "censoring_kind": sample.censoring_kind.value if sample.censoring_kind else None,
                "censor_bound": sample.censor_bound,
                "parent_record_id": sample.parent_record_id,
            }
            self._hash.update(_canonical_bytes(payload))
            self._hash.update(b"\n")
            self._next += 1

    @staticmethod
    def _slice(slot: list) -> DynamicRangeSlice:
        lo, hi, count = slot
        span = None if lo is None or hi is None else hi - lo
        return DynamicRangeSlice(lo, hi, span, int(count))

    def finalize(self) -> DynamicAuthorityFieldSummary:
        expected = self.binding.width * self.binding.height
        if self._next != expected:
            raise ContractError(f"field incomplete: got {self._next} samples, expected {expected}")
        purpose = self.binding.purpose
        return DynamicAuthorityFieldSummary(
            schema="TruthRawDynamicAuthorityFieldSummary/0.5",
            frame_id=self.binding.frame_id,
            width=self.binding.width,
            height=self.binding.height,
            purpose=purpose,
            reference_l0=self.binding.reference_l0,
            sample_count=self._next,
            counts=dict(self._counts),
            representation=self._slice(self._ranges["representation"]),
            evidence_supported=self._slice(self._ranges["evidence"]),
            reconstruction_supported=self._slice(self._ranges["reconstruction"]),
            scientific_supported=self._slice(self._ranges["scientific"]),
            signed_nonpositive_estimate_count=self._nonpositive,
            scientific_master_write_allowed=(purpose is DynamicFieldPurpose.SCIENTIFIC_MASTER),
            fixed_dynamic_range_limit_ev=None,
            content_sha256=self._hash.hexdigest(),
            scientific_policy_sha256=self.binding.scientific_policy_sha256,
        )


@dataclass(frozen=True)
class HardwareBudget:
    working_memory_bytes: int
    logical_cores: int
    bytes_per_working_pixel: int = 64
    halo_pixels: int = 16
    usable_fraction: float = 0.50
    max_concurrency: int = 8

    def validate(self) -> "HardwareBudget":
        if self.working_memory_bytes <= 0 or self.logical_cores <= 0:
            raise ContractError("working memory and logical cores must be positive")
        if self.bytes_per_working_pixel <= 0 or self.halo_pixels < 0 or self.max_concurrency <= 0:
            raise ContractError("invalid hardware workspace parameters")
        if not isfinite(self.usable_fraction) or not (0.05 <= self.usable_fraction <= 0.90):
            raise ContractError("usable_fraction must be within [0.05, 0.90]")
        return self


@dataclass(frozen=True)
class DynamicExecutionPlan:
    tile_core_pixels: int
    halo_pixels: int
    concurrency: int
    estimated_peak_working_bytes: int
    available_working_bytes: int
    scientific_policy_sha256: str
    execution_plan_sha256: str


def plan_dynamic_execution(
    binding: DynamicAuthorityFieldBinding,
    budget: HardwareBudget,
) -> DynamicExecutionPlan:
    """Choose bounded compute geometry without changing scientific semantics."""
    binding.validate()
    budget.validate()
    available = int(budget.working_memory_bytes * budget.usable_fraction)
    limit_core = max(32, min(binding.width, binding.height))
    candidates = [c for c in (32, 64, 128, 256, 512, 1024, 2048) if c <= limit_core]
    if not candidates:
        candidates = [limit_core]

    best = None
    for core in candidates:
        per_tile = (core + 2 * budget.halo_pixels) ** 2 * budget.bytes_per_working_pixel
        for concurrency in range(1, min(budget.logical_cores, budget.max_concurrency) + 1):
            peak = per_tile * concurrency
            if peak > available:
                continue
            # Throughput proxy only. It changes scheduling, never evidence logic.
            score = core * core * concurrency
            candidate = (score, core, concurrency, peak)
            if best is None or candidate > best:
                best = candidate
    if best is None:
        raise ContractError("hardware budget cannot hold the minimum bounded workspace")

    _, core, concurrency, peak = best
    policy_sha = binding.scientific_policy_sha256
    plan_payload = {
        "schema": "TruthRawDynamicExecutionPlan/0.5",
        "tile_core_pixels": core,
        "halo_pixels": budget.halo_pixels,
        "concurrency": concurrency,
        "estimated_peak_working_bytes": peak,
        "available_working_bytes": available,
        "scientific_policy_sha256": policy_sha,
    }
    return DynamicExecutionPlan(
        tile_core_pixels=core,
        halo_pixels=budget.halo_pixels,
        concurrency=concurrency,
        estimated_peak_working_bytes=peak,
        available_working_bytes=available,
        scientific_policy_sha256=policy_sha,
        execution_plan_sha256=_canonical_hash(plan_payload),
    )
