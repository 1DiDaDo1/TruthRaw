#!/usr/bin/env python3
"""TruthRaw v1.0: classify Adobe Lightroom mobile HDR TIFF observations.

The contract deliberately separates Adobe HDR edit-state metadata from a
self-describing, independently validated HDR pixel transport. It never permits
Adobe-rendered TIFF data to write scientific authority back into TruthRaw.
"""
from __future__ import annotations

from dataclasses import dataclass
from enum import Enum
from math import isfinite
from typing import Tuple

from open_world_foundations_v01 import ContractError


class TiffHdrTransportClass(str, Enum):
    ADOBE_ECOSYSTEM_HDR_INTERCHANGE = "ADOBE_ECOSYSTEM_HDR_INTERCHANGE"
    SELF_DESCRIBING_HDR_TRANSPORT = "SELF_DESCRIBING_HDR_TRANSPORT"
    SDR_ONLY = "SDR_ONLY"


@dataclass(frozen=True)
class AdobeHdrTiffObservationV10:
    source_raw_file: str
    tiff_sha256: str
    width: int
    height: int
    bits_per_sample: Tuple[int, int, int]
    sample_storage: str
    compression: str
    photometric: str
    icc_profile_description: str
    xmp_hdr_edit_mode: bool
    xmp_hdr_max_value_ev: float
    exposure_2012_ev: float
    whites_2012: float
    tone_curve_name: str
    extended_tone_curve_points: Tuple[Tuple[int, int], ...]
    lightroom_software: str
    any_channel_code_ceiling_fraction: float
    explicit_standard_hdr_transfer_in_tiff_tags: bool
    independent_decoder_hdr_roundtrip_proven: bool = False

    def validate(self) -> "AdobeHdrTiffObservationV10":
        if not self.source_raw_file.lower().endswith(".dng"):
            raise ContractError("source_raw_file must identify the bound DNG")
        s = self.tiff_sha256.lower()
        if len(s) != 64 or any(c not in "0123456789abcdef" for c in s):
            raise ContractError("tiff_sha256 must be a 64-character hexadecimal SHA-256")
        if self.width <= 0 or self.height <= 0:
            raise ContractError("invalid TIFF geometry")
        if self.bits_per_sample != (16, 16, 16):
            raise ContractError("v1.0 observation is frozen to 16-bit RGB TIFF")
        if self.sample_storage != "UINT16":
            raise ContractError("v1.0 observation is frozen to unsigned 16-bit storage")
        if self.photometric != "RGB":
            raise ContractError("v1.0 requires RGB photometric interpretation")
        if not self.icc_profile_description.strip():
            raise ContractError("ICC profile description is required")
        if not self.lightroom_software.strip():
            raise ContractError("Lightroom software string is required")
        for name, value in (
            ("xmp_hdr_max_value_ev", self.xmp_hdr_max_value_ev),
            ("exposure_2012_ev", self.exposure_2012_ev),
            ("whites_2012", self.whites_2012),
            ("any_channel_code_ceiling_fraction", self.any_channel_code_ceiling_fraction),
        ):
            if not isfinite(float(value)):
                raise ContractError(f"{name} must be finite")
        if self.xmp_hdr_max_value_ev < 0.0:
            raise ContractError("HDR max value cannot be negative")
        if not 0.0 <= self.any_channel_code_ceiling_fraction <= 1.0:
            raise ContractError("ceiling fraction must be in [0,1]")
        if not self.extended_tone_curve_points:
            raise ContractError("extended HDR tone-curve points are required")
        return self


@dataclass(frozen=True)
class AdobeHdrTiffAssessmentV10:
    classification: TiffHdrTransportClass
    adobe_hdr_edit_state_present: bool
    generic_decoder_self_describing_hdr_transport_proven: bool
    standard_pixel_payload_can_be_treated_as_pq_without_extra_contract: bool
    scientific_master_writeback_allowed: bool
    creates_new_measured_dynamic_range: bool
    code_ceiling_requires_caution: bool
    interpretation: str


def assess_adobe_hdr_tiff_v10(obs: AdobeHdrTiffObservationV10) -> AdobeHdrTiffAssessmentV10:
    o = obs.validate()
    adobe_state = bool(o.xmp_hdr_edit_mode and o.xmp_hdr_max_value_ev > 0.0)

    if adobe_state and (o.explicit_standard_hdr_transfer_in_tiff_tags or o.independent_decoder_hdr_roundtrip_proven):
        cls = TiffHdrTransportClass.SELF_DESCRIBING_HDR_TRANSPORT
        proven = True
    elif adobe_state:
        cls = TiffHdrTransportClass.ADOBE_ECOSYSTEM_HDR_INTERCHANGE
        proven = False
    else:
        cls = TiffHdrTransportClass.SDR_ONLY
        proven = False

    return AdobeHdrTiffAssessmentV10(
        classification=cls,
        adobe_hdr_edit_state_present=adobe_state,
        generic_decoder_self_describing_hdr_transport_proven=proven,
        standard_pixel_payload_can_be_treated_as_pq_without_extra_contract=(
            proven and o.explicit_standard_hdr_transfer_in_tiff_tags
        ),
        scientific_master_writeback_allowed=False,
        creates_new_measured_dynamic_range=False,
        code_ceiling_requires_caution=o.any_channel_code_ceiling_fraction > 0.0,
        interpretation=(
            "Adobe HDR edit state can coexist with a TIFF payload whose independently readable HDR transfer semantics "
            "are not yet proven. Treat the file as presentation/editing interchange only until a standards-explicit "
            "or independently validated round-trip establishes the pixel transport."
        ),
    )


REAL_MOBILE_TIFF_OBSERVATION_V10 = AdobeHdrTiffObservationV10(
    source_raw_file="IMG_BNC_TRUTHRAW20260907_094449_565.dng",
    tiff_sha256="03da35eb8c708eac09de55850e868a728378501ded5e6a91db0dbae7f74f8af9",
    width=4064,
    height=3056,
    bits_per_sample=(16, 16, 16),
    sample_storage="UINT16",
    compression="NONE",
    photometric="RGB",
    icc_profile_description="Adobe RGB (1998)",
    xmp_hdr_edit_mode=True,
    xmp_hdr_max_value_ev=8.0,
    exposure_2012_ev=0.0,
    whites_2012=2.0,
    tone_curve_name="Custom",
    extended_tone_curve_points=((0, 12), (152, 228), (386, 500)),
    lightroom_software="Adobe Lightroom 11.5.22 (Android)",
    any_channel_code_ceiling_fraction=0.004926896102156079,
    explicit_standard_hdr_transfer_in_tiff_tags=False,
    independent_decoder_hdr_roundtrip_proven=False,
)
