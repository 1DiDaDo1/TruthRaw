#!/usr/bin/env python3
"""TruthRaw backplane-bound Dynamic Authority runtime v0.6.

v0.6 turns the v0.5 Dynamic Authority contract into a streamed RGB scientific
artifact that can be produced from the existing v0.4 reconstruction/uncertainty
runtime without requiring full-frame RAM residency.

Scientific boundaries:
- the 180-byte Technical Backplane v0.1 remains immutable and unchanged;
- the Dynamic Authority artifact is a sidecar bound cryptographically to that
  backplane and therefore to the same source/master/zero-line/scene-scale lineage;
- Stage-2 direct CFA values default to CALIBRATED_ESTIMATE rather than MEASURED,
  because Stage-2 is a calibrated transform of the raw code value;
- a censored physical CFA sample keeps an explicit inequality and never becomes
  recovered measured data;
- missing channels become RECONSTRUCTED only when v0.4 proves exact measured
  reinjection and an in-domain uncertainty binding; otherwise they are UNKNOWN;
- counterfactual and appearance-only samples are never emitted into this
  Scientific Master sidecar;
- persistence is global-raster ordered and independent of span/chunk size;
- compute-tile geometry is not an authority boundary. Upstream authority-region
  semantics must be canonical and hardware independent.

The module does not claim that the current 4080x3072 tele uncertainty model is
valid for the 16320x12288 maximum-resolution Camera-5 route. v0.4 already fails
that transfer closed; v0.6 preserves the resulting UNKNOWN missing channels.
"""
from __future__ import annotations

import hashlib
import json
import zlib
from dataclasses import dataclass
from enum import Enum
from io import BufferedIOBase
from math import isfinite
from typing import BinaryIO, Iterable, Optional, Sequence, Tuple

from open_world_foundations_v01 import ContractError
from open_world_dynamic_authority_v05 import (
    CensoringKind,
    DynamicAuthority,
    DynamicAuthorityFieldBinding,
    DynamicAuthoritySample,
    DynamicFieldPurpose,
    DynamicAuthorityFieldSummary,
    StreamingDynamicAuthorityAccumulator,
)
from open_world_structure_runtime_v04 import (
    RuntimeStructureResult,
    RuntimeStructureStatus,
    RuntimeStructureTile,
)


_SHA256_HEX = set("0123456789abcdef")
_BACKPLANE_MAGIC = b"TRBACK01"
_BACKPLANE_BYTES = 180
_BACKPLANE_CRC_OFFSET = 176
_CHANNEL_NAMES = ("R", "G", "B")
_CFA = {
    "BGGR": (2, 1, 1, 0),
    "RGGB": (0, 1, 1, 2),
    "GRBG": (1, 0, 2, 1),
    "GBRG": (1, 2, 0, 1),
}


def _canonical_bytes(payload: dict) -> bytes:
    return json.dumps(payload, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("utf-8")


def _canonical_hash(payload: dict) -> str:
    return hashlib.sha256(_canonical_bytes(payload)).hexdigest()


def _check_sha256(value: str, field_name: str) -> str:
    v = value.lower()
    if len(v) != 64 or any(ch not in _SHA256_HEX for ch in v):
        raise ContractError(f"{field_name} must be a 64-character hexadecimal SHA-256")
    return v


def _color_at(cfa: str, x: int, y: int) -> int:
    pattern = _CFA.get(cfa)
    if pattern is None:
        raise ContractError(f"unsupported CFA: {cfa}")
    return pattern[(y & 1) * 2 + (x & 1)]


def _sample_payload(sample: DynamicAuthoritySample) -> dict:
    return {
        "authority": sample.authority.value,
        "scene_linear_estimate": sample.scene_linear_estimate,
        "uncertainty_p95": sample.uncertainty_p95,
        "support": sample.support,
        "source_sample_index": sample.source_sample_index,
        "censoring_kind": sample.censoring_kind.value if sample.censoring_kind else None,
        "censor_bound": sample.censor_bound,
        "parent_record_id": sample.parent_record_id,
    }


@dataclass(frozen=True)
class TechnicalBackplaneV01Identity:
    serialized_sha256: str
    source_evidence_sha256: str
    scientific_master_sha256: str
    zero_line_sha256: str
    scene_scale_sha256: str
    physical_frame_count: int
    independent_evidence_count: int
    room_status: Tuple[int, ...]
    claim_status: int
    forbidden_flags: int


def parse_technical_backplane_v01(data: bytes) -> TechnicalBackplaneV01Identity:
    """Parse and validate the exact native Technical Backplane v0.1 layout.

    This mirrors the native 180-byte layout and CRC contract. It validates the
    embedded identity bindings but, like the native module, does not prove that
    those SHA-256 values match external files; Archivist admission owns that.
    """
    if len(data) != _BACKPLANE_BYTES:
        raise ContractError("Technical Backplane v0.1 must be exactly 180 bytes")
    if data[:8] != _BACKPLANE_MAGIC:
        raise ContractError("Technical Backplane magic mismatch")
    version = int.from_bytes(data[8:10], "little")
    declared_bytes = int.from_bytes(data[10:12], "little")
    if version != 1 or declared_bytes != _BACKPLANE_BYTES:
        raise ContractError("unsupported Technical Backplane version/size")
    expected_crc = int.from_bytes(data[_BACKPLANE_CRC_OFFSET:_BACKPLANE_BYTES], "little")
    got_crc = zlib.crc32(data[:_BACKPLANE_CRC_OFFSET]) & 0xFFFFFFFF
    if got_crc != expected_crc:
        raise ContractError("Technical Backplane CRC32 mismatch")
    if any(data[165:_BACKPLANE_CRC_OFFSET]):
        raise ContractError("Technical Backplane reserved bytes must remain zero")

    forbidden = int.from_bytes(data[12:16], "little")
    source = data[16:48].hex()
    master = data[48:80].hex()
    zero_line = data[80:112].hex()
    scene_scale = data[112:144].hex()
    physical = int.from_bytes(data[144:148], "little")
    independent = int.from_bytes(data[148:152], "little")
    rooms = tuple(int(x) for x in data[152:164])
    claim = int(data[164])

    for name, value in (
        ("source_evidence_sha256", source),
        ("scientific_master_sha256", master),
        ("zero_line_sha256", zero_line),
        ("scene_scale_sha256", scene_scale),
    ):
        _check_sha256(value, name)
        if int(value, 16) == 0:
            raise ContractError(f"Technical Backplane {name} may not be all-zero")
    if physical != 1 or independent != 1:
        raise ContractError("Technical Backplane must bind exactly one physical frame and one independent evidence item")
    if forbidden != 0:
        raise ContractError("Technical Backplane has forbidden scientific mutation/evidence-promotion flags")
    if any(x > 4 for x in rooms):
        raise ContractError("Technical Backplane contains an invalid room status")
    if claim > 3:
        raise ContractError("Technical Backplane contains an invalid claim status")

    return TechnicalBackplaneV01Identity(
        serialized_sha256=hashlib.sha256(data).hexdigest(),
        source_evidence_sha256=source,
        scientific_master_sha256=master,
        zero_line_sha256=zero_line,
        scene_scale_sha256=scene_scale,
        physical_frame_count=physical,
        independent_evidence_count=independent,
        room_status=rooms,
        claim_status=claim,
        forbidden_flags=forbidden,
    )


class CensorRelation(str, Enum):
    SCENE_VALUE_GE_BOUND = "SCENE_VALUE_GE_BOUND"
    SCENE_VALUE_LE_BOUND = "SCENE_VALUE_LE_BOUND"


@dataclass(frozen=True)
class CensorConstraint:
    kind: CensoringKind
    relation: CensorRelation
    bound: float

    def validate(self) -> "CensorConstraint":
        if not isfinite(float(self.bound)):
            raise ContractError("censor bound must be finite")
        if self.kind is CensoringKind.HIGHLIGHT_SATURATION and self.relation is not CensorRelation.SCENE_VALUE_GE_BOUND:
            raise ContractError("highlight saturation must preserve a lower-bound inequality")
        if self.kind is CensoringKind.SHADOW_FLOOR and self.relation is not CensorRelation.SCENE_VALUE_LE_BOUND:
            raise ContractError("shadow-floor censoring must preserve an upper-bound inequality")
        return self


@dataclass(frozen=True)
class DynamicAuthorityChannelRecord:
    sample: DynamicAuthoritySample
    censor_relation: Optional[CensorRelation] = None

    def validate(self) -> "DynamicAuthorityChannelRecord":
        self.sample.validate()
        if self.sample.authority is DynamicAuthority.CENSORED:
            if self.censor_relation is None or self.sample.censor_bound is None:
                raise ContractError("CENSORED channel records require explicit inequality relation and bound")
        elif self.censor_relation is not None:
            raise ContractError("censor_relation is valid only for CENSORED channel records")
        return self


@dataclass(frozen=True)
class DynamicAuthorityPixelRecord:
    channels: Tuple[DynamicAuthorityChannelRecord, DynamicAuthorityChannelRecord, DynamicAuthorityChannelRecord]

    def validate(self) -> "DynamicAuthorityPixelRecord":
        if len(self.channels) != 3:
            raise ContractError("RGB Dynamic Authority pixel must contain exactly three channel records")
        for channel in self.channels:
            channel.validate()
        return self


@dataclass(frozen=True)
class RuntimeDynamicTileV06:
    structure_tile: RuntimeStructureTile
    structure_result: RuntimeStructureResult
    measured_uncertainty_p95_cfa: Sequence[float]
    censor_constraints_cfa: Sequence[Optional[CensorConstraint]]
    direct_stage2_authority: DynamicAuthority = DynamicAuthority.CALIBRATED_ESTIMATE

    def validate(self) -> int:
        n = self.structure_tile.validate_shape()
        if len(self.measured_uncertainty_p95_cfa) != n:
            raise ContractError("measured p95 CFA uncertainty must match tile sample count")
        if len(self.censor_constraints_cfa) != n:
            raise ContractError("censor constraints must match tile sample count")
        if self.direct_stage2_authority not in (DynamicAuthority.MEASURED, DynamicAuthority.CALIBRATED_ESTIMATE):
            raise ContractError("direct Stage-2 authority must be MEASURED or CALIBRATED_ESTIMATE")
        for i, p95 in enumerate(self.measured_uncertainty_p95_cfa):
            if not isfinite(float(p95)) or float(p95) < 0.0:
                raise ContractError("measured p95 CFA uncertainty must be finite and nonnegative")
            censored = bool(self.structure_tile.censored_cfa[i])
            constraint = self.censor_constraints_cfa[i]
            if censored and constraint is None:
                raise ContractError("every censored CFA sample needs an explicit censor inequality")
            if not censored and constraint is not None:
                raise ContractError("uncensored CFA sample may not carry a censor constraint")
            if constraint is not None:
                constraint.validate()
        _check_sha256(self.structure_result.binding_sha256, "v0.4 structure result binding_sha256")
        return n


def _measured_reinjection_exact(tile: RuntimeStructureTile) -> bool:
    for yy in range(tile.tile_height):
        for xx in range(tile.tile_width):
            i = yy * tile.tile_width + xx
            c = _color_at(tile.cfa, tile.tile_x + xx, tile.tile_y + yy)
            if float(tile.reconstructed_rgb[3 * i + c]) != float(tile.stage2_cfa[i]):
                return False
    return True


def _reconstruction_authorized(result: RuntimeStructureResult) -> bool:
    if result.status in (
        RuntimeStructureStatus.BLOCKED_UNCERTAINTY_OUT_OF_DOMAIN,
        RuntimeStructureStatus.BLOCKED_BACKEND_BINDING_MISMATCH,
        RuntimeStructureStatus.BLOCKED_MEASURED_REINJECTION_MISMATCH,
    ):
        return False
    if result.record is None:
        return False
    if not result.diagnostics.exact_measured_reinjection:
        return False
    if not result.diagnostics.uncertainty_domain_certified:
        return False
    return result.record.reconstructed_support > 0.0


def build_dynamic_authority_rows_v06(runtime: RuntimeDynamicTileV06) -> Tuple[Tuple[int, int, Tuple[DynamicAuthorityPixelRecord, ...]], ...]:
    """Build row spans from one v0.4 authority region.

    The returned spans contain no execution-tile metadata. They can therefore be
    persisted in global raster order independent of hardware chunking. The caller
    must interleave horizontal regions so each global scanline is emitted left to
    right before the next scanline.
    """
    runtime.validate()
    tile = runtime.structure_tile
    if not _measured_reinjection_exact(tile):
        raise ContractError("Scientific Master does not exactly preserve the measured CFA component")

    reconstruction_ok = _reconstruction_authorized(runtime.structure_result)
    reconstructed_support = runtime.structure_result.record.reconstructed_support if reconstruction_ok else 0.0
    rows = []
    for yy in range(tile.tile_height):
        pixels = []
        global_y = tile.tile_y + yy
        for xx in range(tile.tile_width):
            i = yy * tile.tile_width + xx
            global_x = tile.tile_x + xx
            global_source_index = global_y * tile.source_width + global_x
            measured_channel = _color_at(tile.cfa, global_x, global_y)
            channels = []
            if bool(tile.censored_cfa[i]):
                constraint = runtime.censor_constraints_cfa[i]
                assert constraint is not None
                for c in range(3):
                    if c == measured_channel:
                        sample = DynamicAuthoritySample(
                            authority=DynamicAuthority.CENSORED,
                            scene_linear_estimate=None,
                            support=0.0,
                            source_sample_index=global_source_index,
                            censoring_kind=constraint.kind,
                            censor_bound=float(constraint.bound),
                        )
                        channels.append(DynamicAuthorityChannelRecord(sample, constraint.relation))
                    else:
                        channels.append(DynamicAuthorityChannelRecord(DynamicAuthoritySample(DynamicAuthority.UNKNOWN, None)))
            else:
                for c in range(3):
                    if c == measured_channel:
                        sample = DynamicAuthoritySample(
                            authority=runtime.direct_stage2_authority,
                            scene_linear_estimate=float(tile.stage2_cfa[i]),
                            uncertainty_p95=float(runtime.measured_uncertainty_p95_cfa[i]),
                            support=1.0,
                            source_sample_index=global_source_index,
                        )
                    elif reconstruction_ok:
                        sample = DynamicAuthoritySample(
                            authority=DynamicAuthority.RECONSTRUCTED,
                            scene_linear_estimate=float(tile.reconstructed_rgb[3 * i + c]),
                            uncertainty_p95=float(tile.uncertainty_p95_rgb[3 * i + c]),
                            support=float(reconstructed_support),
                        )
                    else:
                        sample = DynamicAuthoritySample(DynamicAuthority.UNKNOWN, None)
                    channels.append(DynamicAuthorityChannelRecord(sample))
            pixels.append(DynamicAuthorityPixelRecord((channels[0], channels[1], channels[2])).validate())
        rows.append((global_y, tile.tile_x, tuple(pixels)))
    return tuple(rows)


@dataclass(frozen=True)
class DynamicRuntimeArtifactSummaryV06:
    schema: str
    frame_id: str
    width: int
    height: int
    pixel_count: int
    technical_backplane_sha256: str
    source_evidence_sha256: str
    scientific_master_sha256: str
    zero_line_sha256: str
    scene_scale_sha256: str
    channel_summaries: Tuple[DynamicAuthorityFieldSummary, DynamicAuthorityFieldSummary, DynamicAuthorityFieldSummary]
    field_content_sha256: str
    artifact_sha256: str
    lineage_binding_sha256: str
    fixed_dynamic_range_limit_ev: Optional[float]
    scientific_master_writeback_from_sidecar: bool


class DynamicAuthorityArtifactWriterV06:
    """Stream one RGB Dynamic Authority sidecar in global raster order.

    The writer retains only v0.5 accumulators and the caller-provided span. It
    does not retain a full-frame pixel vector. The exact emitted bytes and
    scientific content digest are independent of append-span size as long as
    the same pixel records are supplied in the same global raster order.
    """

    def __init__(
        self,
        technical_backplane_bytes: bytes,
        *,
        frame_id: str,
        width: int,
        height: int,
        reference_l0: float,
        sink: BinaryIO,
    ) -> None:
        self.backplane = parse_technical_backplane_v01(technical_backplane_bytes)
        if not frame_id.strip():
            raise ContractError("frame_id may not be empty")
        self.binding = DynamicAuthorityFieldBinding(
            frame_id=frame_id,
            width=width,
            height=height,
            reference_l0=reference_l0,
            source_evidence_sha256=self.backplane.source_evidence_sha256,
            scientific_master_sha256=self.backplane.scientific_master_sha256,
            purpose=DynamicFieldPurpose.SCIENTIFIC_MASTER,
        ).validate()
        if not hasattr(sink, "write"):
            raise ContractError("sink must provide a binary write() method")
        self.sink = sink
        self._artifact_hash = hashlib.sha256()
        self._next = 0
        self._acc = tuple(StreamingDynamicAuthorityAccumulator(self.binding) for _ in range(3))
        header = {
            "type": "header",
            "schema": "TruthRawDynamicAuthorityArtifact/0.6",
            "frame_id": frame_id,
            "width": width,
            "height": height,
            "reference_l0": reference_l0,
            "encoding": "NDJSON_RGB_PIXEL_RECORDS_GLOBAL_RASTER_ORDER",
            "technical_backplane_sha256": self.backplane.serialized_sha256,
            "source_evidence_sha256": self.backplane.source_evidence_sha256,
            "scientific_master_sha256": self.backplane.scientific_master_sha256,
            "zero_line_sha256": self.backplane.zero_line_sha256,
            "scene_scale_sha256": self.backplane.scene_scale_sha256,
            "physical_frame_count": self.backplane.physical_frame_count,
            "independent_evidence_count": self.backplane.independent_evidence_count,
            "fixed_dynamic_range_limit_ev": None,
            "counterfactual_or_appearance_writeback_allowed": False,
        }
        self._write_line(header)

    def _write_line(self, payload: dict) -> None:
        data = _canonical_bytes(payload) + b"\n"
        self.sink.write(data)
        self._artifact_hash.update(data)

    def validate_runtime_tile_lineage(self, runtime: RuntimeDynamicTileV06) -> None:
        tile = runtime.structure_tile
        if tile.source_width != self.binding.width or tile.source_height != self.binding.height:
            raise ContractError("runtime tile source geometry does not match Dynamic Authority artifact geometry")
        if tile.source_evidence_sha256.lower() != self.backplane.source_evidence_sha256:
            raise ContractError("runtime tile source evidence does not match Technical Backplane")
        if tile.scientific_master_sha256.lower() != self.backplane.scientific_master_sha256:
            raise ContractError("runtime tile Scientific Master does not match Technical Backplane")

    def append_span(self, y: int, x_start: int, pixels: Sequence[DynamicAuthorityPixelRecord]) -> None:
        if y < 0 or y >= self.binding.height or x_start < 0 or x_start > self.binding.width:
            raise ContractError("span coordinates outside declared frame")
        if x_start + len(pixels) > self.binding.width:
            raise ContractError("span may not cross a scanline boundary")
        start = y * self.binding.width + x_start
        if start != self._next:
            raise ContractError("Dynamic Authority spans must be contiguous in global raster order")
        channel_samples = [[], [], []]
        for pixel in pixels:
            pixel.validate()
            for c in range(3):
                channel_samples[c].append(pixel.channels[c].sample)
        for c in range(3):
            self._acc[c].append_chunk(start, channel_samples[c])

        for local, pixel in enumerate(pixels):
            i = start + local
            channel_payload = {}
            for c, name in enumerate(_CHANNEL_NAMES):
                record = pixel.channels[c]
                p = _sample_payload(record.sample)
                p["censor_relation"] = record.censor_relation.value if record.censor_relation else None
                channel_payload[name] = p
            self._write_line({"type": "pixel", "i": i, "channels": channel_payload})
            self._next += 1

    def finalize(self) -> DynamicRuntimeArtifactSummaryV06:
        expected = self.binding.width * self.binding.height
        if self._next != expected:
            raise ContractError(f"Dynamic Authority artifact incomplete: got {self._next} pixels, expected {expected}")
        summaries = tuple(acc.finalize() for acc in self._acc)
        field_payload = {
            "schema": "TruthRawDynamicAuthorityRGBField/0.6",
            "frame_id": self.binding.frame_id,
            "width": self.binding.width,
            "height": self.binding.height,
            "technical_backplane_sha256": self.backplane.serialized_sha256,
            "channel_content_sha256": {name: summaries[i].content_sha256 for i, name in enumerate(_CHANNEL_NAMES)},
            "scientific_policy_sha256": {name: summaries[i].scientific_policy_sha256 for i, name in enumerate(_CHANNEL_NAMES)},
            "fixed_dynamic_range_limit_ev": None,
        }
        field_content_sha = _canonical_hash(field_payload)
        trailer = {
            "type": "summary",
            "schema": "TruthRawDynamicAuthorityArtifactSummary/0.6",
            "pixel_count": self._next,
            "field_content_sha256": field_content_sha,
            "channel_content_sha256": field_payload["channel_content_sha256"],
            "scientific_policy_sha256": field_payload["scientific_policy_sha256"],
            "scientific_master_writeback_from_sidecar": False,
        }
        self._write_line(trailer)
        artifact_sha = self._artifact_hash.hexdigest()
        lineage_payload = {
            "schema": "TruthRawDynamicAuthorityBackplaneBinding/0.6",
            "technical_backplane_sha256": self.backplane.serialized_sha256,
            "source_evidence_sha256": self.backplane.source_evidence_sha256,
            "scientific_master_sha256": self.backplane.scientific_master_sha256,
            "zero_line_sha256": self.backplane.zero_line_sha256,
            "scene_scale_sha256": self.backplane.scene_scale_sha256,
            "physical_frame_count": self.backplane.physical_frame_count,
            "independent_evidence_count": self.backplane.independent_evidence_count,
            "field_content_sha256": field_content_sha,
            "artifact_sha256": artifact_sha,
            "scientific_policy_sha256": field_payload["scientific_policy_sha256"],
            "sidecar_modifies_scientific_master": False,
        }
        return DynamicRuntimeArtifactSummaryV06(
            schema="TruthRawDynamicAuthorityArtifactSummary/0.6",
            frame_id=self.binding.frame_id,
            width=self.binding.width,
            height=self.binding.height,
            pixel_count=self._next,
            technical_backplane_sha256=self.backplane.serialized_sha256,
            source_evidence_sha256=self.backplane.source_evidence_sha256,
            scientific_master_sha256=self.backplane.scientific_master_sha256,
            zero_line_sha256=self.backplane.zero_line_sha256,
            scene_scale_sha256=self.backplane.scene_scale_sha256,
            channel_summaries=(summaries[0], summaries[1], summaries[2]),
            field_content_sha256=field_content_sha,
            artifact_sha256=artifact_sha,
            lineage_binding_sha256=_canonical_hash(lineage_payload),
            fixed_dynamic_range_limit_ev=None,
            scientific_master_writeback_from_sidecar=False,
        )
