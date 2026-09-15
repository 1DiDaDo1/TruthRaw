#!/usr/bin/env python3
"""TruthRaw open-world integration v0.3.

This layer extends v0.2 with three concrete mechanisms:

1. conservative derivation of Structure Evidence from separately supplied
   measurement/topology/optics/uncertainty/censoring support,
2. immutable binding of a Calibration Registry to the exact native Camera-5
   16320x12288 maximum-resolution tele route,
3. a branching restoration DAG so visual hypotheses on one branch never taint
   a clean sibling branch that returns to an earlier scientific parent.

No function in this module rewrites immutable source evidence. Open-world scope
is unconstrained; only evidence authority is constrained.
"""
from __future__ import annotations

import hashlib
import json
from dataclasses import dataclass, field
from math import isfinite
from typing import Dict, Optional, Tuple

from open_world_foundations_v01 import (
    ContractError,
    HoldoutStatus,
    RestorationClass,
    RestorationRecord,
    StructureEvidenceRecord,
)
from open_world_integration_v02 import CalibrationRegistry


_SHA256_HEX = set("0123456789abcdef")


def _check_sha256(value: str, field_name: str) -> None:
    v = value.lower()
    if len(v) != 64 or any(ch not in _SHA256_HEX for ch in v):
        raise ContractError(f"{field_name} must be a 64-character hexadecimal SHA-256")


def _unit(value: float, field_name: str) -> None:
    if not isfinite(value) or not (0.0 <= value <= 1.0):
        raise ContractError(f"{field_name} must be finite and within [0, 1]")


def _canonical_hash(payload: dict) -> str:
    blob = json.dumps(payload, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("utf-8")
    return hashlib.sha256(blob).hexdigest()


@dataclass(frozen=True)
class StructureMeasurementInputs:
    """Support inputs that already exist before appearance sharpening.

    Values are normalized support/confidence quantities supplied by the
    measurement/reconstruction/optics/uncertainty stages. v0.3 deliberately
    does not infer these quantities from rendered RGB texture.
    """

    region_id: str
    measured_cfa_support: float
    reconstructed_topology_support: float
    optical_mtf_support: float
    uncertainty_confidence: float
    censoring_risk: float
    evidence_sha256: Tuple[str, ...]
    orientation_radians: Optional[float] = None
    spatial_frequency_cyc_per_px: Optional[float] = None

    def validate(self) -> "StructureMeasurementInputs":
        if not self.region_id.strip():
            raise ContractError("structure region_id may not be empty")
        for name in (
            "measured_cfa_support",
            "reconstructed_topology_support",
            "optical_mtf_support",
            "uncertainty_confidence",
            "censoring_risk",
        ):
            _unit(getattr(self, name), name)
        if not self.evidence_sha256:
            raise ContractError("derived Structure Evidence requires admitted evidence hashes")
        for h in self.evidence_sha256:
            _check_sha256(h, "structure evidence_sha256")
        if self.orientation_radians is not None and not isfinite(self.orientation_radians):
            raise ContractError("orientation_radians must be finite")
        if self.spatial_frequency_cyc_per_px is not None:
            if not isfinite(self.spatial_frequency_cyc_per_px) or self.spatial_frequency_cyc_per_px < 0.0:
                raise ContractError("spatial_frequency_cyc_per_px must be finite and >= 0")
        return self


def derive_structure_evidence(inputs: StructureMeasurementInputs) -> StructureEvidenceRecord:
    """Build a conservative StructureEvidenceRecord without a sharpening model.

    The lower-envelope rule is intentionally monotone and fail-closed: output
    support may never exceed CFA/topology support, optical MTF support,
    uncertainty confidence, or the uncensored fraction. No semantic classifier
    and no rendered-image contrast can increase scientific support.
    """
    inputs.validate()
    admissible = min(
        inputs.optical_mtf_support,
        inputs.uncertainty_confidence,
        1.0 - inputs.censoring_risk,
    )
    measured = min(inputs.measured_cfa_support, admissible)
    reconstructed = min(inputs.reconstructed_topology_support, admissible)
    return StructureEvidenceRecord(
        region_id=inputs.region_id,
        measured_support=measured,
        reconstructed_support=reconstructed,
        mtf_support=inputs.optical_mtf_support,
        censoring_risk=inputs.censoring_risk,
        uncertainty=1.0 - inputs.uncertainty_confidence,
        orientation_radians=inputs.orientation_radians,
        spatial_frequency_cyc_per_px=inputs.spatial_frequency_cyc_per_px,
        evidence_sha256=tuple(h.lower() for h in inputs.evidence_sha256),
    ).validate()


@dataclass(frozen=True)
class NativeTeleRoute:
    device: str = "HONOR BKQ-N49"
    physical_camera_id: str = "5"
    sensor_pixel_mode: str = "MAXIMUM_RESOLUTION"
    width: int = 16320
    height: int = 12288
    cfa: str = "BGGR"


@dataclass(frozen=True)
class TeleCalibrationRegistryBinding:
    schema: str
    route: NativeTeleRoute
    registry_fingerprint: str
    calibration_ids: Tuple[str, ...]
    calibration_kinds: Tuple[str, ...]
    binding_sha256: str


def bind_registry_to_native_tele(
    registry: CalibrationRegistry,
    route: NativeTeleRoute = NativeTeleRoute(),
    *,
    require_holdout: bool = True,
) -> TeleCalibrationRegistryBinding:
    """Bind a registry immutably to one exact native-tele measurement route.

    This does not create calibration data. It only certifies that every record
    admitted into this route-specific registry declares the same camera/mode/
    geometry/CFA domain and, by default, has an independent hold-out PASS.
    """
    if not registry.records:
        raise ContractError("native tele registry may not be empty")

    ids = []
    kinds = []
    for record in registry.records:
        record.validate()
        d = record.domain
        if (
            d.device != route.device
            or d.physical_camera_id != route.physical_camera_id
            or d.sensor_pixel_mode != route.sensor_pixel_mode
            or d.width != route.width
            or d.height != route.height
            or d.cfa != route.cfa
        ):
            raise ContractError(
                f"calibration {record.calibration_id} is not bound to the exact native Camera-5 tele route"
            )
        if require_holdout and record.holdout_status is not HoldoutStatus.PASS:
            raise ContractError(
                f"calibration {record.calibration_id} lacks required independent hold-out PASS"
            )
        ids.append(record.calibration_id)
        kinds.append(record.kind.value)

    registry_fingerprint = registry.fingerprint()
    payload = {
        "schema": "TruthRawNativeTeleCalibrationRegistryBinding/0.3",
        "route": {
            "device": route.device,
            "physical_camera_id": route.physical_camera_id,
            "sensor_pixel_mode": route.sensor_pixel_mode,
            "width": route.width,
            "height": route.height,
            "cfa": route.cfa,
        },
        "registry_fingerprint": registry_fingerprint,
        "calibration_ids": sorted(ids),
        "calibration_kinds": sorted(kinds),
    }
    return TeleCalibrationRegistryBinding(
        schema=payload["schema"],
        route=route,
        registry_fingerprint=registry_fingerprint,
        calibration_ids=tuple(payload["calibration_ids"]),
        calibration_kinds=tuple(payload["calibration_kinds"]),
        binding_sha256=_canonical_hash(payload),
    )


@dataclass(frozen=True)
class RestorationGraphNode:
    layer_id: str
    restoration_class: RestorationClass
    root_source_sha256: str
    parent_layer_sha256: Optional[str]
    mask_sha256: str
    payload_sha256: str
    evidence_sha256: Tuple[str, ...]
    method: str
    confidence: Optional[float]
    hypothesis_description: Optional[str]
    layer_sha256: str


class RestorationGraph:
    """Branching, hash-bound and reversible conservation/restoration graph.

    Authority taint follows ancestry, not unrelated sibling branches. A visual
    hypothesis may therefore coexist with a separate evidence-supported branch
    that explicitly returns to the immutable source or another clean parent.
    """

    def __init__(self, root_source_sha256: str) -> None:
        _check_sha256(root_source_sha256, "root_source_sha256")
        self.root_source_sha256 = root_source_sha256.lower()
        self._nodes: Dict[str, RestorationGraphNode] = {}
        self._layer_ids = set()

    @property
    def nodes(self) -> Tuple[RestorationGraphNode, ...]:
        return tuple(self._nodes.values())

    def _ancestry_has_visual_taint(self, parent_layer_sha256: Optional[str]) -> bool:
        current = parent_layer_sha256
        seen = set()
        while current is not None:
            if current in seen:
                raise ContractError("restoration graph cycle detected")
            seen.add(current)
            node = self._nodes.get(current)
            if node is None:
                raise ContractError("restoration parent does not exist")
            if node.restoration_class in (
                RestorationClass.HYPOTHETICAL_VISUAL_RESTORATION,
                RestorationClass.APPEARANCE_ONLY,
            ):
                return True
            current = node.parent_layer_sha256
        return False

    def append(
        self,
        record: RestorationRecord,
        payload_sha256: str,
        *,
        parent_layer_sha256: Optional[str] = None,
    ) -> RestorationGraphNode:
        record.validate()
        _check_sha256(payload_sha256, "restoration payload_sha256")
        if record.layer_id in self._layer_ids:
            raise ContractError(f"duplicate restoration layer_id: {record.layer_id}")
        if self.root_source_sha256 not in tuple(h.lower() for h in record.source_evidence_sha256):
            raise ContractError("restoration record must remain bound to root source evidence")
        if parent_layer_sha256 is not None:
            _check_sha256(parent_layer_sha256, "parent_layer_sha256")
            if parent_layer_sha256 not in self._nodes:
                raise ContractError("restoration parent layer hash not found")

        tainted = self._ancestry_has_visual_taint(parent_layer_sha256)
        if tainted and record.restoration_class in (
            RestorationClass.OBSERVED_LOSS,
            RestorationClass.EVIDENCE_SUPPORTED_RECONSTRUCTION,
        ):
            raise ContractError(
                "evidence-supported restoration cannot descend from a hypothetical/appearance ancestor"
            )

        payload = {
            "schema": "TruthRawRestorationGraphNode/0.3",
            "layer_id": record.layer_id,
            "restoration_class": record.restoration_class.value,
            "root_source_sha256": self.root_source_sha256,
            "parent_layer_sha256": parent_layer_sha256,
            "mask_sha256": record.mask_sha256.lower(),
            "payload_sha256": payload_sha256.lower(),
            "evidence_sha256": [h.lower() for h in record.source_evidence_sha256],
            "method": record.method,
            "confidence": record.confidence,
            "hypothesis_description": record.hypothesis_description,
        }
        layer_sha = _canonical_hash(payload)
        node = RestorationGraphNode(
            layer_id=record.layer_id,
            restoration_class=record.restoration_class,
            root_source_sha256=self.root_source_sha256,
            parent_layer_sha256=parent_layer_sha256,
            mask_sha256=record.mask_sha256.lower(),
            payload_sha256=payload_sha256.lower(),
            evidence_sha256=tuple(h.lower() for h in record.source_evidence_sha256),
            method=record.method,
            confidence=record.confidence,
            hypothesis_description=record.hypothesis_description,
            layer_sha256=layer_sha,
        )
        if layer_sha in self._nodes:
            raise ContractError("duplicate restoration node hash")
        self._nodes[layer_sha] = node
        self._layer_ids.add(record.layer_id)
        return node

    def verify(self) -> bool:
        try:
            for layer_sha, node in self._nodes.items():
                if node.root_source_sha256 != self.root_source_sha256:
                    return False
                if node.parent_layer_sha256 is not None and node.parent_layer_sha256 not in self._nodes:
                    return False
                if self._ancestry_has_visual_taint(node.parent_layer_sha256) and node.restoration_class in (
                    RestorationClass.OBSERVED_LOSS,
                    RestorationClass.EVIDENCE_SUPPORTED_RECONSTRUCTION,
                ):
                    return False
                payload = {
                    "schema": "TruthRawRestorationGraphNode/0.3",
                    "layer_id": node.layer_id,
                    "restoration_class": node.restoration_class.value,
                    "root_source_sha256": node.root_source_sha256,
                    "parent_layer_sha256": node.parent_layer_sha256,
                    "mask_sha256": node.mask_sha256,
                    "payload_sha256": node.payload_sha256,
                    "evidence_sha256": list(node.evidence_sha256),
                    "method": node.method,
                    "confidence": node.confidence,
                    "hypothesis_description": node.hypothesis_description,
                }
                if _canonical_hash(payload) != layer_sha or node.layer_sha256 != layer_sha:
                    return False
            return True
        except ContractError:
            return False

    def manifest(self) -> dict:
        if not self.verify():
            raise ContractError("restoration graph failed verification")
        nodes = []
        for sha, node in sorted(self._nodes.items()):
            nodes.append(
                {
                    "layer_sha256": sha,
                    "layer_id": node.layer_id,
                    "restoration_class": node.restoration_class.value,
                    "parent_layer_sha256": node.parent_layer_sha256,
                    "mask_sha256": node.mask_sha256,
                    "payload_sha256": node.payload_sha256,
                }
            )
        payload = {
            "schema": "TruthRawRestorationGraph/0.3",
            "root_source_sha256": self.root_source_sha256,
            "source_overwritten": False,
            "node_count": len(nodes),
            "nodes": nodes,
        }
        payload["graph_sha256"] = _canonical_hash(payload)
        return payload
