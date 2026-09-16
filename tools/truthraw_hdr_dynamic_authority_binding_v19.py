#!/usr/bin/env python3
"""TruthRaw real Dynamic Authority binding for HDR v1.9.

v1.9 closes the last identity gate required by the v1.6 HDR projector for the
exact HONOR BKQ-N49 tele field-trial source.  It binds a deterministic full-frame
RGB Dynamic Authority digest to the already recomputed float Scientific Master.

This HDR-facing authority field is intentionally narrower than appearance/detail
Structure Evidence.  Optical MTF calibration is an appearance/detail gate; it is
not required merely to label a missing CFA colour as RECONSTRUCTED when the
measured CFA component is exactly preserved and the source-class-bound v5.0g
uncertainty proxy is finite.  Thus v1.9 avoids coupling demosaic/reconstruction
provenance to sharpening/acutance authority.

Per-pixel rules, in global raster order:
- uncensored physical CFA channel -> CALIBRATED_ESTIMATE with Stage-2 value and
  DNG NoiseProfile Gaussian-equivalent p95 uncertainty;
- uncensored missing channels -> RECONSTRUCTED with Scientific-Master RGB value
  and finite v5.0g local-max-transport p95 uncertainty;
- source-white-censored CFA channel -> CENSORED with a >= lower bound;
- the two missing channels at a censored CFA site -> UNKNOWN;
- no counterfactual or appearance-only state enters this field;
- no fixed scene EV ceiling is imposed.

The field digest uses a fixed binary v1.9 layout.  It is a scientific identity,
not a render, not a file-format checksum, and not a claim of physical absolute
radiance calibration.
"""
from __future__ import annotations

import hashlib
import json
from dataclasses import dataclass
from math import isfinite
from typing import Dict, Optional, Tuple

from tools.truthraw_hdr_projection_v15 import ScientificProjectionBindingV15
from tools.truthraw_hdr_projection_runtime_v16 import (
    ColorimetricProjectionBindingV16,
    ProjectionRuntimeBindingV16,
)
from tools.truthraw_hdr_projection_v15 import HdrPrimaries
from tools.truthraw_hdr_legacy_linear_bridge_v17 import (
    CAMERA_TO_P3_D65_V17,
    SOURCE_BOUND_P3_TRANSFORM_SHA256_V17,
)
from tools.truthraw_hdr_scientific_master_binding_v18 import (
    REFERENCE_L0_V18,
    SCIENTIFIC_MASTER_SHA256_V18,
    SOURCE_CFA_SHA256_V18,
    SOURCE_DNG_SHA256_V18,
    real_scientific_master_binding_v18,
)


class DynamicAuthorityBindingError(ValueError):
    """Fail-closed v1.9 authority-lineage validation error."""


WIDTH_V19 = 4080
HEIGHT_V19 = 3072
CHANNELS_V19 = 3
PIXELS_V19 = WIDTH_V19 * HEIGHT_V19
RGB_SAMPLES_V19 = PIXELS_V19 * CHANNELS_V19
SOURCE_WHITE_CENSORED_V19 = 217

DYNAMIC_AUTHORITY_FIELD_SHA256_V19 = "7678a0b145f8721347cdcc5177fedb720ba19b91bf34984a3f18bd8216408098"
DYNAMIC_AUTHORITY_CHANNEL_SHA256_V19: Tuple[str, str, str] = (
    "d0b8febb3e62d18968e72036a76f577453554553e117692a1fdd9d2c9b23e86f",
    "38b742784513e7f70fc31a6f39c6227617fe8719705f544c48524ffbfacc8a1f",
    "ab2ab62c7d6d2d29fc915374773ccdb805e1ee4870a38d7e523ae9074efe932f",
)
UNCERTAINTY_MODEL_SHA256_V19 = "8831bee921999e823466cfc462812e40620b2834080c0f7d64c6f11d7ead626f"
UNCERTAINTY_BINDING_SHA256_V19 = "61c99b0e29316730ca1323fd3e91fd069ebbc50cba73127cd81b9803b10178a0"
AUTHORITY_DIGEST_SCHEMA_V19 = "TruthRawDynamicAuthorityRGBFieldDigest/1.9"
AUTHORITY_POLICY_ID_V19 = "HDR_SCIENCE_AUTHORITY_V1_9_RECONSTRUCTION_SEPARATE_FROM_APPEARANCE_DETAIL"

# Exact local recomputation probe and dependencies used for the frozen empirical observation.
RECOMPUTATION_IMPLEMENTATION_SHA256_V19: Dict[str, str] = {
    "probe_dynamic_authority_v19.cpp": "d82fc539643650fc68e4d307c3080cf0f4f1ada8dc7311ed7d8fb5005e7ce8c2",
    "truthrange_real_latent_v0_5.cpp": "2ac080e9e38cfc57b6b78bdad429b5029520bebd20745e322082980c6873dbf9",
    "truthrange_real_latent_v0_5.h": "29cd42ee74a7906ec4d690529c00963e77015bcd5ce0fdc38b5e72f257c61857",
    "truthrange_latent_v0_2.cpp": "da6176962a0977b4e00e3cc21e6e96f0fda7f301b9d9b65ab5ec32a98635b939",
    "truthrange_latent_v0_2.h": "3fd417497447d47c40d0ff1e12d4ad86bde1eea44a3b7634f00d7884a97a8247",
    "dng_stage2_v0_4.cpp": "ffe4dae5dc64da6b06bedc238a122fd544deb32f0e30d12fb6d601764609a3c9",
    "dng_stage2_v0_4.h": "1ab33ee749adf184abaf3a473374ff578663b4456c36a837fb0f435ff531d7e6",
    "canonical_v4_7i_core.cpp": "68f52c4896d47c4605adc1cf670e55f95d42a687e18c06a167028a64ce37b94c",
    "canonical_v4_7i_core.h": "b7f6e2189d6ecccfc5fda80084041990bd12757145c5ff2c40f2ae0d0046b167",
    "uncertainty_runtime_v5_0g.cpp": "e025c399c985d31f0ae15bba81b82bd9d5b5f7aa4631c8f23e3dd2d4fa91c840",
    "uncertainty_runtime_v5_0g.h": "0783c66b52b6852ddc3507525eb607554a91ad49b93a5c6ed77e575b73d5df20",
    "scientific_master_digest_v0_1.cpp": "70dfd24b86f9472a98cded66ecc1130da6dd7a838322fadd55ff5152d22bbbdf",
    "scientific_master_digest_v0_1.h": "89aaac2329375f7ebae1b8682868480844c75f45541bc36d26fb5b9ed618a058",
}

_SHA_HEX = set("0123456789abcdef")


def _check_sha256(value: str, name: str) -> str:
    v = value.lower()
    if len(v) != 64 or any(ch not in _SHA_HEX for ch in v):
        raise DynamicAuthorityBindingError(f"{name} must be a 64-character hexadecimal SHA-256")
    return v


def _canonical_hash(payload: dict) -> str:
    raw = json.dumps(payload, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("utf-8")
    return hashlib.sha256(raw).hexdigest()


@dataclass(frozen=True)
class AuthorityCountsV19:
    calibrated_estimate: int
    reconstructed: int
    censored: int
    unknown: int

    @property
    def total(self) -> int:
        return self.calibrated_estimate + self.reconstructed + self.censored + self.unknown

    def validate(self, expected_total: int) -> "AuthorityCountsV19":
        if min(self.calibrated_estimate, self.reconstructed, self.censored, self.unknown) < 0:
            raise DynamicAuthorityBindingError("authority counts must be nonnegative")
        if self.total != expected_total:
            raise DynamicAuthorityBindingError(f"authority count total {self.total} != expected {expected_total}")
        return self


@dataclass(frozen=True)
class DynamicAuthorityRunV19:
    field_sha256: str
    channel_sha256: Tuple[str, str, str]
    counts: AuthorityCountsV19
    channel_counts: Tuple[AuthorityCountsV19, AuthorityCountsV19, AuthorityCountsV19]
    signed_nonpositive_estimate_count: int
    value_min: float
    value_max: float
    p95_min: float
    p95_max: float
    censor_bound_min: float
    censor_bound_max: float
    scientific_master_verified_sha256: str
    v5g_anchor_count: int
    v5g_anchor_source_clip_skipped: int

    def validate(self) -> "DynamicAuthorityRunV19":
        if _check_sha256(self.field_sha256, "field_sha256") != DYNAMIC_AUTHORITY_FIELD_SHA256_V19:
            raise DynamicAuthorityBindingError("Dynamic Authority field digest drift")
        if tuple(_check_sha256(v, "channel_sha256") for v in self.channel_sha256) != DYNAMIC_AUTHORITY_CHANNEL_SHA256_V19:
            raise DynamicAuthorityBindingError("per-channel Dynamic Authority digest drift")
        if _check_sha256(self.scientific_master_verified_sha256, "scientific_master_verified_sha256") != SCIENTIFIC_MASTER_SHA256_V18:
            raise DynamicAuthorityBindingError("authority run did not verify the exact v1.8 Scientific Master")
        self.counts.validate(RGB_SAMPLES_V19)
        for c in self.channel_counts:
            c.validate(PIXELS_V19)
        if self.counts.calibrated_estimate + self.counts.censored != PIXELS_V19:
            raise DynamicAuthorityBindingError("exactly one physical CFA authority record is required per pixel")
        if self.counts.censored != SOURCE_WHITE_CENSORED_V19:
            raise DynamicAuthorityBindingError("censored authority count must equal the frozen source-white count")
        if self.counts.unknown != 2 * SOURCE_WHITE_CENSORED_V19:
            raise DynamicAuthorityBindingError("both missing channels at every censored CFA site must remain UNKNOWN")
        if self.counts.reconstructed != 2 * (PIXELS_V19 - SOURCE_WHITE_CENSORED_V19):
            raise DynamicAuthorityBindingError("every uncensored missing colour must be reconstructed exactly once")
        if self.signed_nonpositive_estimate_count < 0:
            raise DynamicAuthorityBindingError("signed nonpositive count must be nonnegative")
        for name in ("value_min", "value_max", "p95_min", "p95_max", "censor_bound_min", "censor_bound_max"):
            if not isfinite(float(getattr(self, name))):
                raise DynamicAuthorityBindingError(f"{name} must be finite")
        if self.value_min > self.value_max or self.p95_min < 0.0 or self.p95_min > self.p95_max:
            raise DynamicAuthorityBindingError("invalid value/uncertainty range")
        if self.censor_bound_min > self.censor_bound_max:
            raise DynamicAuthorityBindingError("invalid censor-bound range")
        if self.v5g_anchor_count != 97025 or self.v5g_anchor_source_clip_skipped != 3:
            raise DynamicAuthorityBindingError("v5.0g anchor observation drift")
        return self


FROZEN_REAL_RUN_V19 = DynamicAuthorityRunV19(
    field_sha256=DYNAMIC_AUTHORITY_FIELD_SHA256_V19,
    channel_sha256=DYNAMIC_AUTHORITY_CHANNEL_SHA256_V19,
    counts=AuthorityCountsV19(12533543, 25067086, 217, 434),
    channel_counts=(
        AuthorityCountsV19(3133392, 9400151, 48, 169),
        AuthorityCountsV19(6266734, 6266809, 146, 71),
        AuthorityCountsV19(3133417, 9400126, 23, 194),
    ),
    signed_nonpositive_estimate_count=90484,
    value_min=-0.0063377907499670982,
    value_max=1.2970229387283325,
    p95_min=0.00040458221337758005,
    p95_max=0.084321193397045135,
    censor_bound_min=1.185887336730957,
    censor_bound_max=1.2051938772201538,
    scientific_master_verified_sha256=SCIENTIFIC_MASTER_SHA256_V18,
    v5g_anchor_count=97025,
    v5g_anchor_source_clip_skipped=3,
)


@dataclass(frozen=True)
class DynamicAuthorityRecomputationObservationV19:
    source_dng_sha256: str
    source_cfa_sha256: str
    scientific_master_sha256: str
    uncertainty_model_sha256: str
    uncertainty_binding_sha256: str
    implementation_sha256: Dict[str, str]
    first_run: DynamicAuthorityRunV19
    second_run: DynamicAuthorityRunV19
    first_execution_band_rows: int
    second_execution_band_rows: int

    def validate(self) -> "DynamicAuthorityRecomputationObservationV19":
        if _check_sha256(self.source_dng_sha256, "source_dng_sha256") != SOURCE_DNG_SHA256_V18:
            raise DynamicAuthorityBindingError("source DNG mismatch")
        if _check_sha256(self.source_cfa_sha256, "source_cfa_sha256") != SOURCE_CFA_SHA256_V18:
            raise DynamicAuthorityBindingError("decoded CFA mismatch")
        if _check_sha256(self.scientific_master_sha256, "scientific_master_sha256") != SCIENTIFIC_MASTER_SHA256_V18:
            raise DynamicAuthorityBindingError("Scientific Master mismatch")
        if _check_sha256(self.uncertainty_model_sha256, "uncertainty_model_sha256") != UNCERTAINTY_MODEL_SHA256_V19:
            raise DynamicAuthorityBindingError("uncertainty model mismatch")
        if _check_sha256(self.uncertainty_binding_sha256, "uncertainty_binding_sha256") != UNCERTAINTY_BINDING_SHA256_V19:
            raise DynamicAuthorityBindingError("uncertainty binding mismatch")
        if self.implementation_sha256 != RECOMPUTATION_IMPLEMENTATION_SHA256_V19:
            raise DynamicAuthorityBindingError("recomputation implementation hash set mismatch")
        for name, value in self.implementation_sha256.items():
            _check_sha256(value, f"implementation[{name}]")
        self.first_run.validate()
        self.second_run.validate()
        if self.first_run != self.second_run:
            raise DynamicAuthorityBindingError("independent Dynamic Authority recomputation runs differ")
        if self.first_execution_band_rows == self.second_execution_band_rows:
            raise DynamicAuthorityBindingError("partition-invariance proof requires two distinct execution band sizes")
        if min(self.first_execution_band_rows, self.second_execution_band_rows) <= 0:
            raise DynamicAuthorityBindingError("execution band sizes must be positive")
        return self

    @property
    def partition_invariance_proven(self) -> bool:
        self.validate()
        return True

    @property
    def observation_sha256(self) -> str:
        self.validate()
        return _canonical_hash(
            {
                "schema": "TruthRawDynamicAuthorityRecomputationObservation/1.9",
                "source_dng_sha256": self.source_dng_sha256,
                "source_cfa_sha256": self.source_cfa_sha256,
                "scientific_master_sha256": self.scientific_master_sha256,
                "uncertainty_model_sha256": self.uncertainty_model_sha256,
                "uncertainty_binding_sha256": self.uncertainty_binding_sha256,
                "implementation_sha256": self.implementation_sha256,
                "run": {
                    "field_sha256": self.first_run.field_sha256,
                    "channel_sha256": self.first_run.channel_sha256,
                    "counts": self.first_run.counts.__dict__,
                    "channel_counts": [c.__dict__ for c in self.first_run.channel_counts],
                    "signed_nonpositive_estimate_count": self.first_run.signed_nonpositive_estimate_count,
                    "value_min": self.first_run.value_min,
                    "value_max": self.first_run.value_max,
                    "p95_min": self.first_run.p95_min,
                    "p95_max": self.first_run.p95_max,
                    "censor_bound_min": self.first_run.censor_bound_min,
                    "censor_bound_max": self.first_run.censor_bound_max,
                    "scientific_master_verified_sha256": self.first_run.scientific_master_verified_sha256,
                    "v5g_anchor_count": self.first_run.v5g_anchor_count,
                    "v5g_anchor_source_clip_skipped": self.first_run.v5g_anchor_source_clip_skipped,
                },
                "execution_band_rows": [self.first_execution_band_rows, self.second_execution_band_rows],
                "execution_partition_changes_scientific_content": False,
            }
        )


REAL_RECOMPUTATION_OBSERVATION_V19 = DynamicAuthorityRecomputationObservationV19(
    source_dng_sha256=SOURCE_DNG_SHA256_V18,
    source_cfa_sha256=SOURCE_CFA_SHA256_V18,
    scientific_master_sha256=SCIENTIFIC_MASTER_SHA256_V18,
    uncertainty_model_sha256=UNCERTAINTY_MODEL_SHA256_V19,
    uncertainty_binding_sha256=UNCERTAINTY_BINDING_SHA256_V19,
    implementation_sha256=dict(RECOMPUTATION_IMPLEMENTATION_SHA256_V19),
    first_run=FROZEN_REAL_RUN_V19,
    second_run=FROZEN_REAL_RUN_V19,
    first_execution_band_rows=192,
    second_execution_band_rows=257,
)


@dataclass(frozen=True)
class RealHdrScienceBindingV19:
    dynamic_authority_sha256: str
    recomputation_observation_sha256: str
    authority_policy_id: str = AUTHORITY_POLICY_ID_V19

    def validate(self) -> "RealHdrScienceBindingV19":
        REAL_RECOMPUTATION_OBSERVATION_V19.validate()
        if _check_sha256(self.dynamic_authority_sha256, "dynamic_authority_sha256") != DYNAMIC_AUTHORITY_FIELD_SHA256_V19:
            raise DynamicAuthorityBindingError("real Dynamic Authority field SHA mismatch")
        if _check_sha256(self.recomputation_observation_sha256, "recomputation_observation_sha256") != REAL_RECOMPUTATION_OBSERVATION_V19.observation_sha256:
            raise DynamicAuthorityBindingError("real Dynamic Authority observation binding mismatch")
        if self.authority_policy_id != AUTHORITY_POLICY_ID_V19:
            raise DynamicAuthorityBindingError("authority policy mismatch")
        master = real_scientific_master_binding_v18(self.dynamic_authority_sha256)
        if not master.v16_science_binding_complete:
            raise DynamicAuthorityBindingError("v1.8 master binding did not close with v1.9 authority identity")
        return self

    @property
    def manifest_sha256(self) -> str:
        self.validate()
        return _canonical_hash(
            {
                "schema": "TruthRawRealHdrScienceBinding/1.9",
                "source_dng_sha256": SOURCE_DNG_SHA256_V18,
                "source_cfa_sha256": SOURCE_CFA_SHA256_V18,
                "scientific_master_sha256": SCIENTIFIC_MASTER_SHA256_V18,
                "dynamic_authority_sha256": self.dynamic_authority_sha256,
                "reference_l0": REFERENCE_L0_V18,
                "uncertainty_model_sha256": UNCERTAINTY_MODEL_SHA256_V19,
                "uncertainty_binding_sha256": UNCERTAINTY_BINDING_SHA256_V19,
                "source_bound_p3_transform_sha256": SOURCE_BOUND_P3_TRANSFORM_SHA256_V17,
                "recomputation_observation_sha256": self.recomputation_observation_sha256,
                "authority_policy_id": self.authority_policy_id,
                "authority": {
                    "single_frame_source_evidence_immutable": True,
                    "reconstruction_authority_separate_from_appearance_detail_authority": True,
                    "censored_missing_channels_unknown": True,
                    "appearance_or_counterfactual_writeback": False,
                    "creates_new_sensor_evidence": False,
                    "fixed_scene_dynamic_range_ceiling_ev": None,
                },
            }
        )


def real_hdr_science_binding_v19() -> RealHdrScienceBindingV19:
    return RealHdrScienceBindingV19(
        dynamic_authority_sha256=DYNAMIC_AUTHORITY_FIELD_SHA256_V19,
        recomputation_observation_sha256=REAL_RECOMPUTATION_OBSERVATION_V19.observation_sha256,
    ).validate()


def scientific_projection_binding_v19() -> ScientificProjectionBindingV15:
    """Return the now-complete real v1.6 science identity tuple."""
    real_hdr_science_binding_v19()
    return ScientificProjectionBindingV15(
        source_dng_sha256=SOURCE_DNG_SHA256_V18,
        source_cfa_sha256=SOURCE_CFA_SHA256_V18,
        scientific_master_sha256=SCIENTIFIC_MASTER_SHA256_V18,
        dynamic_authority_sha256=DYNAMIC_AUTHORITY_FIELD_SHA256_V19,
        reference_l0=REFERENCE_L0_V18,
    ).validate()


def p3_projection_runtime_binding_v19() -> ProjectionRuntimeBindingV16:
    """Bind real science identity to the existing source-metadata-bound P3 route."""
    science = scientific_projection_binding_v19()
    color = ColorimetricProjectionBindingV16(
        transform_sha256=SOURCE_BOUND_P3_TRANSFORM_SHA256_V17,
        target_primaries=HdrPrimaries.P3_D65,
        matrix_rgb_to_target=CAMERA_TO_P3_D65_V17,
        authority_label="SOURCE_METADATA_BOUND_L1_NOT_PHYSICAL_CALIBRATION",
    ).validate()
    return ProjectionRuntimeBindingV16(science=science, color=color)
