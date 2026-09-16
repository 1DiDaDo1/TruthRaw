#!/usr/bin/env python3
"""TruthRaw streamed full-frame Open Scene State v0.8.

v0.8 aggregates validated Open Scene Region v0.7 decisions into one bounded-
memory full-frame sidecar. Input chunking is an execution concern only: adjacent
spans carrying the same region-state identity are canonically coalesced before
hashing, so splitting one scientific region into different hardware chunks does
not change the resulting state identity.

This module stores/aggregates authority metadata only. It does not alter pixels,
create evidence, perform restoration, relight the scene or change the Scientific
Master.
"""
from __future__ import annotations

import hashlib
import json
from collections import Counter
from dataclasses import dataclass
from typing import Dict, Optional

from open_world_foundations_v01 import ContractError
from open_scene_region_runtime_v07 import OpenSceneRegionResultV07

_SHA256_HEX = set("0123456789abcdef")


def _check_sha256(value: str, name: str) -> str:
    v = str(value).lower()
    if len(v) != 64 or any(c not in _SHA256_HEX for c in v):
        raise ContractError(f"{name} must be a 64-character hexadecimal SHA-256")
    return v


def _canonical_bytes(payload: dict) -> bytes:
    return json.dumps(payload, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("utf-8")


@dataclass(frozen=True)
class OpenSceneStateBindingV08:
    frame_id: str
    width: int
    height: int
    source_evidence_sha256: str
    scientific_master_sha256: str
    dynamic_authority_artifact_sha256: str
    physical_frame_count: int = 1
    independent_evidence_count: int = 1

    def validate(self) -> "OpenSceneStateBindingV08":
        if not self.frame_id.strip():
            raise ContractError("frame_id may not be empty")
        if self.width <= 0 or self.height <= 0:
            raise ContractError("full-frame dimensions must be positive")
        _check_sha256(self.source_evidence_sha256, "source_evidence_sha256")
        _check_sha256(self.scientific_master_sha256, "scientific_master_sha256")
        _check_sha256(self.dynamic_authority_artifact_sha256, "dynamic_authority_artifact_sha256")
        if self.physical_frame_count != 1 or self.independent_evidence_count != 1:
            raise ContractError("Open Scene State v0.8 is bound to one physical frame / one independent evidence item")
        return self


@dataclass(frozen=True)
class OpenSceneStateSummaryV08:
    schema: str
    frame_id: str
    width: int
    height: int
    pixel_count: int
    source_evidence_sha256: str
    scientific_master_sha256: str
    dynamic_authority_artifact_sha256: str
    scientific_region_run_count: int
    channel_authority_counts: Dict[str, int]
    hdr_status_counts: Dict[str, int]
    colour_authority_pixel_counts: Dict[str, int]
    illumination_authority_pixel_counts: Dict[str, int]
    detail_status_pixel_counts: Dict[str, int]
    full_physical_colour_claim_pixel_count: int
    counterfactual_illumination_pixel_count: int
    creates_new_evidence: bool
    scientific_master_writeback_allowed: bool
    physical_frame_count: int
    independent_evidence_count: int
    content_sha256: str
    policy_sha256: str


class StreamingOpenSceneStateV08:
    """Canonical run-length stream of full-frame v0.7 scene-region identities."""

    def __init__(self, binding: OpenSceneStateBindingV08) -> None:
        self.binding = binding.validate()
        self._next = 0
        self._content = hashlib.sha256()
        self._pending_sha: Optional[str] = None
        self._pending_start = 0
        self._pending_length = 0
        self._runs = 0
        self._channel = Counter()
        self._hdr = Counter()
        self._colour = Counter()
        self._illumination = Counter()
        self._detail = Counter()
        self._full_physical_colour_pixels = 0
        self._counterfactual_illumination_pixels = 0

    @property
    def total_pixels(self) -> int:
        return self.binding.width * self.binding.height

    @property
    def policy_sha256(self) -> str:
        payload = {
            "schema": "TruthRawOpenSceneStatePolicy/0.8",
            "frame_id": self.binding.frame_id,
            "width": self.binding.width,
            "height": self.binding.height,
            "source_evidence_sha256": self.binding.source_evidence_sha256.lower(),
            "scientific_master_sha256": self.binding.scientific_master_sha256.lower(),
            "dynamic_authority_artifact_sha256": self.binding.dynamic_authority_artifact_sha256.lower(),
            "physical_frame_count": 1,
            "independent_evidence_count": 1,
            "chunking_changes_scientific_identity": False,
            "blocked_restoration_regions_admitted": False,
            "creates_new_evidence": False,
            "scientific_master_writeback_allowed": False,
        }
        return hashlib.sha256(_canonical_bytes(payload)).hexdigest()

    def _validate_region(self, region: OpenSceneRegionResultV07) -> None:
        if not region.pass_contract:
            raise ContractError("blocked Open Scene Region v0.7 result cannot enter full-frame state")
        if region.source_evidence_sha256.lower() != self.binding.source_evidence_sha256.lower():
            raise ContractError("region/source evidence identity mismatch")
        if region.scientific_master_sha256.lower() != self.binding.scientific_master_sha256.lower():
            raise ContractError("region/Scientific Master identity mismatch")
        if region.dynamic_authority_artifact_sha256.lower() != self.binding.dynamic_authority_artifact_sha256.lower():
            raise ContractError("region/Dynamic Authority artifact identity mismatch")
        if region.physical_frame_count != 1 or region.independent_evidence_count != 1:
            raise ContractError("region evidence count mismatch")
        _check_sha256(region.scene_state_sha256, "region scene_state_sha256")
        if region.creates_new_evidence:
            raise ContractError("Open Scene Region may not create new evidence")
        if region.detail_scientific_writeback_allowed or region.restoration_scientific_writeback_allowed:
            raise ContractError("appearance/restoration scientific writeback is forbidden")
        if not all(region.restoration_allowed):
            raise ContractError("region with blocked restoration decision cannot enter full-frame Open Scene State")

    def _flush_pending(self) -> None:
        if self._pending_sha is None or self._pending_length <= 0:
            return
        payload = {
            "start": self._pending_start,
            "length": self._pending_length,
            "region_scene_state_sha256": self._pending_sha,
        }
        self._content.update(_canonical_bytes(payload))
        self._content.update(b"\n")
        self._runs += 1
        self._pending_sha = None
        self._pending_length = 0

    def append_span(self, start_linear_index: int, pixel_count: int, region: OpenSceneRegionResultV07) -> None:
        if start_linear_index != self._next:
            raise ContractError("Open Scene spans must be contiguous in global raster order")
        if pixel_count <= 0:
            raise ContractError("Open Scene span pixel_count must be positive")
        if self._next + pixel_count > self.total_pixels:
            raise ContractError("Open Scene span exceeds declared full-frame pixel count")
        self._validate_region(region)

        region_sha = region.scene_state_sha256.lower()
        if self._pending_sha == region_sha:
            self._pending_length += pixel_count
        else:
            self._flush_pending()
            self._pending_sha = region_sha
            self._pending_start = start_linear_index
            self._pending_length = pixel_count

        for a in region.channel_authorities:
            self._channel[a] += pixel_count
        for h in region.hdr_channel_status:
            self._hdr[h] += pixel_count
        self._colour[region.colour_authority] += pixel_count
        self._illumination[region.illumination_authority] += pixel_count
        self._detail[region.detail_status] += pixel_count
        if region.full_physical_colour_claim_allowed:
            self._full_physical_colour_pixels += pixel_count
        if not region.captured_world_illumination_writeback_allowed:
            self._counterfactual_illumination_pixels += pixel_count
        self._next += pixel_count

    def finalize(self) -> OpenSceneStateSummaryV08:
        if self._next != self.total_pixels:
            raise ContractError(f"Open Scene State incomplete: got {self._next} pixels, expected {self.total_pixels}")
        self._flush_pending()
        return OpenSceneStateSummaryV08(
            schema="TruthRawOpenSceneStateSummary/0.8",
            frame_id=self.binding.frame_id,
            width=self.binding.width,
            height=self.binding.height,
            pixel_count=self._next,
            source_evidence_sha256=self.binding.source_evidence_sha256.lower(),
            scientific_master_sha256=self.binding.scientific_master_sha256.lower(),
            dynamic_authority_artifact_sha256=self.binding.dynamic_authority_artifact_sha256.lower(),
            scientific_region_run_count=self._runs,
            channel_authority_counts=dict(sorted(self._channel.items())),
            hdr_status_counts=dict(sorted(self._hdr.items())),
            colour_authority_pixel_counts=dict(sorted(self._colour.items())),
            illumination_authority_pixel_counts=dict(sorted(self._illumination.items())),
            detail_status_pixel_counts=dict(sorted(self._detail.items())),
            full_physical_colour_claim_pixel_count=self._full_physical_colour_pixels,
            counterfactual_illumination_pixel_count=self._counterfactual_illumination_pixels,
            creates_new_evidence=False,
            scientific_master_writeback_allowed=False,
            physical_frame_count=1,
            independent_evidence_count=1,
            content_sha256=self._content.hexdigest(),
            policy_sha256=self.policy_sha256,
        )
