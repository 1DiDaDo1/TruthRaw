#!/usr/bin/env python3
"""TruthRaw real Scientific Master identity binding v1.8.

v1.8 closes exactly one gate left open by the v1.7 legacy LinearRaw bridge:
the immutable tele source now has a deterministically recomputed float Scientific
Master identity and self-gauge.  It does *not* manufacture the still-missing
Dynamic Authority Field identity and therefore does not yet complete the v1.6
real HDR projection binding.

The scientific identity was recomputed twice from the exact source DNG with the
archived frozen native Scientific Master streaming implementation.  The two runs
were byte-for-byte identical in their reported master SHA and run diagnostics.
The historical v0.4 16-bit LinearRaw compatibility payload remains a different,
quantized derivative and is never substituted for the Scientific Master.
"""
from __future__ import annotations

import hashlib
import json
from dataclasses import dataclass
from math import isfinite
from typing import Dict, Optional


class ScientificMasterBindingError(ValueError):
    """Fail-closed v1.8 scientific-lineage validation error."""


SOURCE_DNG_FILENAME_V18 = "IMG_BNC_TRUTHRAW20260907_094449_565.dng"
SOURCE_DNG_SHA256_V18 = "7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67"
SOURCE_CFA_SHA256_V18 = "883cbe13fd2a5afe7e9f3f4147df3a8ed3be681340a3c0422933f42d0f4d719c"
SCIENTIFIC_MASTER_SHA256_V18 = "a86034da7b6f9663640ee3b4478ccf4294d23b720f2b087d083dca38f5fb4640"
REFERENCE_L0_V18 = 0.12564234435558319
GAUGE_ID_V18 = "SELF_GAUGE_STAGE2_Q0.500000"
RECONSTRUCTION_BACKEND_V18 = "research_edge_aware_support_limited_measured_preserving_v47i"
SCENE_SCALE_ID_V18 = "TRUTHRANGE_SELF_GAUGE_STAGE2_V0_2"
SOURCE_BOUND_P3_TRANSFORM_SHA256_V18 = "2105712a1be9950089976ba358afd4b062347680d5e6d8c500462c2e8d541533"

# Historical compatibility payloads are intentionally forbidden substitutes.
LEGACY_LINEARRAW_FILE_SHA256_V18 = "347cd68ade21f99607028b23ed7cdc0c6b498885d236e9c6443624228747766f"
LEGACY_LINEARRAW_PIXEL_PAYLOAD_SHA256_V18 = "71b42d01c2e0a54e0807ad671529d7dddebb3e0982c1d7246515b12b49db26eb"
LEGACY_LINEARRAW_DEQUANTIZED_F32_SHA256_V18 = "0def5ca38d3339e48e81435df68007745b2cdfeacd8e36d72ead06dafb44acd0"

# Exact archived implementation used for the two local recomputations.
RECOMPUTATION_ARCHIVE_COMMIT_V18 = "4f842d8fe86eca2b5808afb10fbb9f11a2631fed"
RECOMPUTATION_IMPLEMENTATION_SHA256_V18: Dict[str, str] = {
    "scientific_master_streaming_binding_v0_2.cpp": "14b3d7b1f3cacf032729c6af3f12f29679af5c3ce12db27790b18f43bb4ae5b9",
    "scientific_master_streaming_binding_v0_2.h": "77e429fdadea7b2589a264462a7b450cd692da0bde45f8073773a2679a4e599a",
    "scientific_master_streaming_binding_v0_1.cpp": "e4e22dd2a579868d432f0edd2d3c4b19d5f09f68d61ac89f70bba8f343853a44",
    "scientific_master_digest_v0_1.cpp": "70dfd24b86f9472a98cded66ecc1130da6dd7a838322fadd55ff5152d22bbbdf",
    "truthrange_latent_v0_2.cpp": "da6176962a0977b4e00e3cc21e6e96f0fda7f301b9d9b65ab5ec32a98635b939",
    "tile_native_dng_source_v0_1.cpp": "09b8153ef60977b48fa67b4bc5d6e7a723a47401de2f8092b06a83656bed84d1",
    "canonical_v4_7i_core.cpp": "68f52c4896d47c4605adc1cf670e55f95d42a687e18c06a167028a64ce37b94c",
    "canonical_v4_7i_core.h": "b7f6e2189d6ecccfc5fda80084041990bd12757145c5ff2c40f2ae0d0046b167",
}

_SHA_HEX = set("0123456789abcdef")


def _check_sha256(value: str, name: str) -> str:
    v = value.lower()
    if len(v) != 64 or any(ch not in _SHA_HEX for ch in v):
        raise ScientificMasterBindingError(f"{name} must be a 64-character hexadecimal SHA-256")
    return v


def _check_commit(value: str, name: str) -> str:
    v = value.lower()
    if len(v) != 40 or any(ch not in _SHA_HEX for ch in v):
        raise ScientificMasterBindingError(f"{name} must be a 40-character hexadecimal Git commit SHA")
    return v


def _canonical_hash(payload: dict) -> str:
    raw = json.dumps(payload, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("utf-8")
    return hashlib.sha256(raw).hexdigest()


@dataclass(frozen=True)
class ScientificMasterRunV18:
    scientific_master_sha256: str
    reference_l0: float
    gauge_id: str
    reconstruction_backend: str
    scene_scale_id: str
    self_gauge_eligible_samples: int
    master_tiles_processed: int
    stage2_gauge_scan_passes: int
    logical_workspace_peak_bytes: int
    logical_resident_upper_bound: int
    raw_payload_bytes_read: int
    tile_read_calls: int

    def validate(self) -> "ScientificMasterRunV18":
        _check_sha256(self.scientific_master_sha256, "scientific_master_sha256")
        if not isfinite(self.reference_l0) or self.reference_l0 <= 0.0:
            raise ScientificMasterBindingError("reference_l0 must be finite and > 0")
        if not self.gauge_id.strip() or not self.reconstruction_backend.strip() or not self.scene_scale_id.strip():
            raise ScientificMasterBindingError("gauge/backend/scene-scale identifiers may not be empty")
        for name in (
            "self_gauge_eligible_samples",
            "master_tiles_processed",
            "stage2_gauge_scan_passes",
            "logical_workspace_peak_bytes",
            "logical_resident_upper_bound",
            "raw_payload_bytes_read",
            "tile_read_calls",
        ):
            if int(getattr(self, name)) <= 0:
                raise ScientificMasterBindingError(f"{name} must be positive")
        return self


FROZEN_REAL_RUN_V18 = ScientificMasterRunV18(
    scientific_master_sha256=SCIENTIFIC_MASTER_SHA256_V18,
    reference_l0=REFERENCE_L0_V18,
    gauge_id=GAUGE_ID_V18,
    reconstruction_backend=RECONSTRUCTION_BACKEND_V18,
    scene_scale_id=SCENE_SCALE_ID_V18,
    self_gauge_eligible_samples=8002727,
    master_tiles_processed=3072,
    stage2_gauge_scan_passes=2,
    logical_workspace_peak_bytes=138932,
    logical_resident_upper_bound=1295156,
    raw_payload_bytes_read=59808528,
    tile_read_calls=6144,
)


@dataclass(frozen=True)
class ScientificMasterRecomputationObservationV18:
    source_dng_sha256: str
    source_cfa_sha256: str
    archive_commit_sha: str
    implementation_sha256: Dict[str, str]
    first_run: ScientificMasterRunV18
    second_run: ScientificMasterRunV18

    def validate(self) -> "ScientificMasterRecomputationObservationV18":
        if _check_sha256(self.source_dng_sha256, "source_dng_sha256") != SOURCE_DNG_SHA256_V18:
            raise ScientificMasterBindingError("observation is not bound to the frozen v1.8 source DNG")
        if _check_sha256(self.source_cfa_sha256, "source_cfa_sha256") != SOURCE_CFA_SHA256_V18:
            raise ScientificMasterBindingError("observation is not bound to the frozen decoded CFA payload")
        if _check_commit(self.archive_commit_sha, "archive_commit_sha") != RECOMPUTATION_ARCHIVE_COMMIT_V18:
            raise ScientificMasterBindingError("recomputation archive commit mismatch")
        if self.implementation_sha256 != RECOMPUTATION_IMPLEMENTATION_SHA256_V18:
            raise ScientificMasterBindingError("recomputation implementation hash set mismatch")
        for name, value in self.implementation_sha256.items():
            _check_sha256(value, f"implementation[{name}]")
        self.first_run.validate()
        self.second_run.validate()
        if self.first_run != self.second_run:
            raise ScientificMasterBindingError("two independent recomputation runs are not identical")
        if self.first_run != FROZEN_REAL_RUN_V18:
            raise ScientificMasterBindingError("recomputation result differs from the frozen real run")
        if self.first_run.scientific_master_sha256 in {
            self.source_dng_sha256,
            self.source_cfa_sha256,
            LEGACY_LINEARRAW_FILE_SHA256_V18,
            LEGACY_LINEARRAW_PIXEL_PAYLOAD_SHA256_V18,
            LEGACY_LINEARRAW_DEQUANTIZED_F32_SHA256_V18,
        }:
            raise ScientificMasterBindingError("Scientific Master identity may not alias source or legacy compatibility identities")
        return self

    @property
    def deterministic_recomputation_proven(self) -> bool:
        self.validate()
        return True

    @property
    def scientific_master_sha256(self) -> str:
        self.validate()
        return self.first_run.scientific_master_sha256

    @property
    def observation_sha256(self) -> str:
        self.validate()
        return _canonical_hash(
            {
                "schema": "TruthRawScientificMasterRecomputationObservation/1.8",
                "source_dng_sha256": self.source_dng_sha256,
                "source_cfa_sha256": self.source_cfa_sha256,
                "archive_commit_sha": self.archive_commit_sha,
                "implementation_sha256": self.implementation_sha256,
                "run": self.first_run.__dict__,
                "independent_repeat_count": 2,
                "repeat_byte_identical": True,
            }
        )


REAL_RECOMPUTATION_OBSERVATION_V18 = ScientificMasterRecomputationObservationV18(
    source_dng_sha256=SOURCE_DNG_SHA256_V18,
    source_cfa_sha256=SOURCE_CFA_SHA256_V18,
    archive_commit_sha=RECOMPUTATION_ARCHIVE_COMMIT_V18,
    implementation_sha256=dict(RECOMPUTATION_IMPLEMENTATION_SHA256_V18),
    first_run=FROZEN_REAL_RUN_V18,
    second_run=FROZEN_REAL_RUN_V18,
)


@dataclass(frozen=True)
class RealScientificMasterBindingV18:
    source_dng_filename: str
    source_dng_sha256: str
    source_cfa_sha256: str
    scientific_master_sha256: str
    reference_l0: float
    gauge_id: str
    reconstruction_backend: str
    scene_scale_id: str
    source_bound_p3_transform_sha256: str
    recomputation_observation_sha256: str
    dynamic_authority_sha256: Optional[str] = None

    def validate(self) -> "RealScientificMasterBindingV18":
        REAL_RECOMPUTATION_OBSERVATION_V18.validate()
        if self.source_dng_filename != SOURCE_DNG_FILENAME_V18:
            raise ScientificMasterBindingError("source filename mismatch")
        if _check_sha256(self.source_dng_sha256, "source_dng_sha256") != SOURCE_DNG_SHA256_V18:
            raise ScientificMasterBindingError("source DNG hash mismatch")
        if _check_sha256(self.source_cfa_sha256, "source_cfa_sha256") != SOURCE_CFA_SHA256_V18:
            raise ScientificMasterBindingError("source CFA hash mismatch")
        if _check_sha256(self.scientific_master_sha256, "scientific_master_sha256") != SCIENTIFIC_MASTER_SHA256_V18:
            raise ScientificMasterBindingError("Scientific Master hash mismatch")
        if self.scientific_master_sha256 != REAL_RECOMPUTATION_OBSERVATION_V18.scientific_master_sha256:
            raise ScientificMasterBindingError("Scientific Master hash is not backed by the real recomputation observation")
        if self.reference_l0 != REFERENCE_L0_V18:
            raise ScientificMasterBindingError("reference_l0 mismatch")
        if self.gauge_id != GAUGE_ID_V18 or self.reconstruction_backend != RECONSTRUCTION_BACKEND_V18 or self.scene_scale_id != SCENE_SCALE_ID_V18:
            raise ScientificMasterBindingError("Scientific Master reconstruction/gauge lineage mismatch")
        if _check_sha256(self.source_bound_p3_transform_sha256, "source_bound_p3_transform_sha256") != SOURCE_BOUND_P3_TRANSFORM_SHA256_V18:
            raise ScientificMasterBindingError("source-bound P3 transform mismatch")
        if _check_sha256(self.recomputation_observation_sha256, "recomputation_observation_sha256") != REAL_RECOMPUTATION_OBSERVATION_V18.observation_sha256:
            raise ScientificMasterBindingError("recomputation observation binding mismatch")
        if self.dynamic_authority_sha256 is not None:
            _check_sha256(self.dynamic_authority_sha256, "dynamic_authority_sha256")
        return self

    @property
    def schema(self) -> str:
        return "TruthRawRealScientificMasterBinding/1.8"

    @property
    def scientific_master_identity_bound(self) -> bool:
        self.validate()
        return True

    @property
    def scientific_master_payload_persisted_by_v18(self) -> bool:
        return False

    @property
    def dynamic_authority_identity_bound(self) -> bool:
        self.validate()
        return self.dynamic_authority_sha256 is not None

    @property
    def v16_science_binding_complete(self) -> bool:
        return self.dynamic_authority_identity_bound

    @property
    def manifest_sha256(self) -> str:
        self.validate()
        return _canonical_hash(
            {
                "schema": self.schema,
                "source_dng_filename": self.source_dng_filename,
                "source_dng_sha256": self.source_dng_sha256,
                "source_cfa_sha256": self.source_cfa_sha256,
                "scientific_master_sha256": self.scientific_master_sha256,
                "reference_l0": self.reference_l0,
                "gauge_id": self.gauge_id,
                "reconstruction_backend": self.reconstruction_backend,
                "scene_scale_id": self.scene_scale_id,
                "source_bound_p3_transform_sha256": self.source_bound_p3_transform_sha256,
                "recomputation_observation_sha256": self.recomputation_observation_sha256,
                "dynamic_authority_sha256": self.dynamic_authority_sha256,
                "authority": {
                    "single_frame_source_evidence_immutable": True,
                    "scientific_master_identity_deterministically_recomputed": True,
                    "legacy_linearraw_is_scientific_master": False,
                    "scientific_master_payload_persisted_by_v18": False,
                    "creates_new_sensor_evidence": False,
                    "presentation_metadata_has_scientific_authority": False,
                    "scientific_master_writeback_from_hdr": False,
                    "fixed_scene_dynamic_range_ceiling_ev": None,
                },
            }
        )


def real_scientific_master_binding_v18(dynamic_authority_sha256: Optional[str] = None) -> RealScientificMasterBindingV18:
    binding = RealScientificMasterBindingV18(
        source_dng_filename=SOURCE_DNG_FILENAME_V18,
        source_dng_sha256=SOURCE_DNG_SHA256_V18,
        source_cfa_sha256=SOURCE_CFA_SHA256_V18,
        scientific_master_sha256=SCIENTIFIC_MASTER_SHA256_V18,
        reference_l0=REFERENCE_L0_V18,
        gauge_id=GAUGE_ID_V18,
        reconstruction_backend=RECONSTRUCTION_BACKEND_V18,
        scene_scale_id=SCENE_SCALE_ID_V18,
        source_bound_p3_transform_sha256=SOURCE_BOUND_P3_TRANSFORM_SHA256_V18,
        recomputation_observation_sha256=REAL_RECOMPUTATION_OBSERVATION_V18.observation_sha256,
        dynamic_authority_sha256=dynamic_authority_sha256,
    )
    return binding.validate()


def require_complete_v16_science_binding_v18(binding: RealScientificMasterBindingV18) -> dict:
    """Return v1.6 binding kwargs only after the real Dynamic Authority SHA exists."""
    binding.validate()
    if binding.dynamic_authority_sha256 is None:
        raise ScientificMasterBindingError(
            "Dynamic Authority Field SHA-256 is still unbound; v1.6 real projection must remain fail-closed"
        )
    return {
        "source_dng_sha256": binding.source_dng_sha256,
        "source_cfa_sha256": binding.source_cfa_sha256,
        "scientific_master_sha256": binding.scientific_master_sha256,
        "dynamic_authority_sha256": binding.dynamic_authority_sha256,
        "reference_l0": binding.reference_l0,
    }
