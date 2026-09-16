#!/usr/bin/env python3
"""TruthRaw legacy LinearRaw HDR-input bridge v1.7.

This module binds the historical v0.4 derived LinearRaw for the exact Adobe
field-trial tele source without relabelling that 16-bit compatibility payload as
the current Scientific Master.

The bridge exists to make old full-frame reconstructed RGB usable as a real-data
transport/integration fixture while preserving the modern authority boundary:
- original source DNG remains the single sealed evidence root;
- the v0.4 RGB payload is DERIVED_RECONSTRUCTED_RAW compatibility data;
- its quantization is explicit and irreversible;
- no Scientific Master SHA or Dynamic Authority Field SHA is manufactured;
- therefore this bridge cannot complete a v1.6 scientific projection binding;
- source-metadata colour may be bound as L1 SOURCE_METADATA_BOUND only, never
  promoted to independent physical/lens calibration.
"""
from __future__ import annotations

import hashlib
import json
import os
import re
import struct
from dataclasses import dataclass
from math import isfinite
from typing import BinaryIO, Dict, Optional, Sequence, Tuple

from tools.open_world_foundations_v01 import ContractError


SOURCE_DNG_FILENAME_V17 = "IMG_BNC_TRUTHRAW20260907_094449_565.dng"
SOURCE_DNG_SHA256_V17 = "7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67"
LEGACY_DNG_FILENAME_V17 = "IMG_BNC_TRUTHRAW20260907_094449_565__TRUTHRAW_DERIVED_LINEAR_RAW_v0_4.dng"
LEGACY_DNG_SHA256_V17 = "347cd68ade21f99607028b23ed7cdc0c6b498885d236e9c6443624228747766f"
LEGACY_PIXEL_PAYLOAD_SHA256_V17 = "71b42d01c2e0a54e0807ad671529d7dddebb3e0982c1d7246515b12b49db26eb"
LEGACY_DEQUANTIZED_F32_SHA256_V17 = "0def5ca38d3339e48e81435df68007745b2cdfeacd8e36d72ead06dafb44acd0"

# Theoretical half-code quantization bound for scene=(code-2048)/32768.
THEORETICAL_QUANTIZATION_HALF_STEP_V17 = 0.5 / 32768.0
# Historical measured maximum error against the pre-quantization v0.1 master.
RECORDED_MAX_QUANTIZATION_ABS_SCENE_ERROR_V17 = 1.531839370727539e-05

# Colorimetric V3 L1 source-bound transform for this exact source.
CAMERA_TO_XYZ_D50_V17 = (
    (0.9360746824592845, 0.27436585365853655, 0.21617130095897005),
    (0.35782747603833864, 0.78125, 0.0),
    (-0.03855306252852579, -0.2906790476190476, 2.0942318885184235),
)
BRADFORD_D50_TO_D65_V17 = (
    (0.9555766, -0.0230393, 0.0631636),
    (-0.0282895, 1.0099416, 0.0210077),
    (0.0122982, -0.0204830, 1.3299098),
)
XYZ_D65_TO_P3_D65_V17 = (
    (2.493496911941425, -0.9313836179191239, -0.40271078445071684),
    (-0.8294889695615747, 1.7626640603183463, 0.023624685841943577),
    (0.03584583024378447, -0.07617238926804182, 0.9568845240076872),
)
CAMERA_TO_P3_D65_V17 = (
    (1.9115759182105407, 0.0018793656065187432, -0.31304168608169364),
    (-0.14532939129426314, 1.1695822925561443, -0.1484403396270572),
    (-0.03882691749797302, -0.432943438113552, 2.6768616785331276),
)
SOURCE_BOUND_P3_TRANSFORM_SHA256_V17 = "2105712a1be9950089976ba358afd4b062347680d5e6d8c500462c2e8d541533"

_TIFF_TYPE_SIZE = {1: 1, 2: 1, 3: 2, 4: 4, 5: 8, 7: 1, 9: 4, 10: 8, 11: 4, 12: 8}
_SHA_HEX = set("0123456789abcdef")


def _check_sha256(value: str, name: str) -> str:
    value = value.lower()
    if len(value) != 64 or any(ch not in _SHA_HEX for ch in value):
        raise ContractError(f"{name} must be a 64-character hexadecimal SHA-256")
    return value


def _canonical_hash(payload: dict) -> str:
    raw = json.dumps(payload, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("utf-8")
    return hashlib.sha256(raw).hexdigest()


def _matmul3(a: Sequence[Sequence[float]], b: Sequence[Sequence[float]]) -> Tuple[Tuple[float, float, float], ...]:
    out = []
    for i in range(3):
        out.append(tuple(sum(float(a[i][k]) * float(b[k][j]) for k in range(3)) for j in range(3)))
    return tuple(out)  # type: ignore[return-value]


def source_bound_p3_transform_payload_v17() -> dict:
    """Return the exact L1 source-metadata-bound CameraRGB->P3-D65 transform record."""
    payload = {
        "schema": "TruthRawSourceBoundP3Transform/1.7",
        "source_dng_sha256": SOURCE_DNG_SHA256_V17,
        "camera_to_xyz_d50": CAMERA_TO_XYZ_D50_V17,
        "bradford_d50_to_d65": BRADFORD_D50_TO_D65_V17,
        "xyz_d65_to_p3_d65": XYZ_D65_TO_P3_D65_V17,
        "matrix_rgb_to_p3_d65": CAMERA_TO_P3_D65_V17,
        "authority": "SOURCE_METADATA_BOUND_L1_NOT_PHYSICAL_CALIBRATION",
    }
    derived = _matmul3(XYZ_D65_TO_P3_D65_V17, _matmul3(BRADFORD_D50_TO_D65_V17, CAMERA_TO_XYZ_D50_V17))
    for got_row, expected_row in zip(derived, CAMERA_TO_P3_D65_V17):
        if max(abs(a - b) for a, b in zip(got_row, expected_row)) > 2e-15:
            raise ContractError("frozen CameraRGB->P3-D65 matrix does not match its declared factors")
    if _canonical_hash(payload) != SOURCE_BOUND_P3_TRANSFORM_SHA256_V17:
        raise ContractError("source-bound P3 transform hash drift")
    return payload


@dataclass(frozen=True)
class LegacyLinearRawExpectationV17:
    file_sha256: str
    pixel_payload_sha256: str
    source_dng_sha256: str
    width: int
    height: int
    samples_per_pixel: int
    bits_per_sample: Tuple[int, int, int]
    photometric_interpretation: int
    compression: int
    orientation: int
    black_level: Tuple[float, float, float]
    white_level: Tuple[int, int, int]
    classification: str = "DERIVED_RECONSTRUCTED_RAW"
    measured_photon_claim: str = "false"

    def validate(self) -> "LegacyLinearRawExpectationV17":
        _check_sha256(self.file_sha256, "file_sha256")
        _check_sha256(self.pixel_payload_sha256, "pixel_payload_sha256")
        _check_sha256(self.source_dng_sha256, "source_dng_sha256")
        if self.width <= 0 or self.height <= 0 or self.samples_per_pixel != 3:
            raise ContractError("legacy bridge requires positive 3-channel LinearRaw geometry")
        if self.bits_per_sample != (16, 16, 16):
            raise ContractError("legacy bridge is frozen to 16-bit RGB LinearRaw")
        if self.compression != 1:
            raise ContractError("legacy bridge requires uncompressed strips")
        if len(self.black_level) != 3 or len(self.white_level) != 3:
            raise ContractError("legacy RGB levels require three channels")
        for b, w in zip(self.black_level, self.white_level):
            if not isfinite(float(b)) or float(w) <= float(b):
                raise ContractError("white level must be finite and greater than black level")
        return self


FROZEN_LEGACY_EXPECTATION_V17 = LegacyLinearRawExpectationV17(
    file_sha256=LEGACY_DNG_SHA256_V17,
    pixel_payload_sha256=LEGACY_PIXEL_PAYLOAD_SHA256_V17,
    source_dng_sha256=SOURCE_DNG_SHA256_V17,
    width=4080,
    height=3072,
    samples_per_pixel=3,
    bits_per_sample=(16, 16, 16),
    photometric_interpretation=34892,
    compression=1,
    orientation=3,
    black_level=(2048.0, 2048.0, 2048.0),
    white_level=(34816, 34816, 34816),
)


@dataclass(frozen=True)
class LegacyLinearRawInspectionV17:
    file_sha256: str
    pixel_payload_sha256: str
    file_bytes: int
    pixel_payload_bytes: int
    width: int
    height: int
    samples_per_pixel: int
    bits_per_sample: Tuple[int, int, int]
    photometric_interpretation: int
    compression: int
    orientation: int
    black_level: Tuple[float, float, float]
    white_level: Tuple[int, int, int]
    rows_per_strip: int
    strip_count: int
    xmp_source_sha256: str
    xmp_payload_sha256: str
    xmp_classification: str
    xmp_measured_photon_claim: str
    xmp_scientific_master_sha256: Optional[str]
    xmp_dynamic_authority_sha256: Optional[str]

    @property
    def exact_legacy_fixture_verified(self) -> bool:
        return True

    @property
    def may_be_relabelled_scientific_master(self) -> bool:
        return False

    @property
    def may_complete_v16_science_binding(self) -> bool:
        return False

    @property
    def creates_new_sensor_evidence(self) -> bool:
        return False


@dataclass(frozen=True)
class LegacyBridgeManifestV17:
    inspection: LegacyLinearRawInspectionV17
    source_bound_color_transform_sha256: str
    theoretical_quantization_half_step: float
    recorded_max_quantization_abs_scene_error: float
    legacy_dequantized_f32_sha256: Optional[str] = None
    scientific_master_sha256: Optional[str] = None
    dynamic_authority_sha256: Optional[str] = None

    def validate(self) -> "LegacyBridgeManifestV17":
        _check_sha256(self.source_bound_color_transform_sha256, "source_bound_color_transform_sha256")
        source_bound_p3_transform_payload_v17()
        if self.source_bound_color_transform_sha256 != SOURCE_BOUND_P3_TRANSFORM_SHA256_V17:
            raise ContractError("legacy bridge color transform is not the frozen exact source-bound transform")
        if self.theoretical_quantization_half_step <= 0.0:
            raise ContractError("quantization half-step must be positive")
        if self.recorded_max_quantization_abs_scene_error < self.theoretical_quantization_half_step:
            raise ContractError("recorded quantization bound may not be tighter than the theoretical half-step")
        if self.legacy_dequantized_f32_sha256 is not None:
            _check_sha256(self.legacy_dequantized_f32_sha256, "legacy_dequantized_f32_sha256")
        if self.scientific_master_sha256 is not None or self.dynamic_authority_sha256 is not None:
            raise ContractError("legacy LinearRaw bridge cannot manufacture Scientific Master or Dynamic Authority hashes")
        return self

    @property
    def schema(self) -> str:
        return "TruthRawHdrLegacyLinearBridge/1.7"

    @property
    def binding_complete_for_v16(self) -> bool:
        return False

    @property
    def manifest_sha256(self) -> str:
        self.validate()
        return _canonical_hash({
            "schema": self.schema,
            "legacy_file_sha256": self.inspection.file_sha256,
            "legacy_pixel_payload_sha256": self.inspection.pixel_payload_sha256,
            "source_dng_sha256": self.inspection.xmp_source_sha256,
            "source_bound_color_transform_sha256": self.source_bound_color_transform_sha256,
            "theoretical_quantization_half_step": self.theoretical_quantization_half_step,
            "recorded_max_quantization_abs_scene_error": self.recorded_max_quantization_abs_scene_error,
            "legacy_dequantized_f32_sha256": self.legacy_dequantized_f32_sha256,
            "scientific_master_sha256": None,
            "dynamic_authority_sha256": None,
            "authority": {
                "classification": "LEGACY_DERIVED_RECONSTRUCTED_LINEAR_RAW_COMPATIBILITY_PAYLOAD",
                "may_be_relabelled_scientific_master": False,
                "may_complete_v16_science_binding": False,
                "creates_new_sensor_evidence": False,
                "scientific_master_writeback_allowed": False,
                "color_authority": "SOURCE_METADATA_BOUND_L1_NOT_PHYSICAL_CALIBRATION",
            },
        })


class _ClassicTiffReader:
    def __init__(self, fh: BinaryIO) -> None:
        self.fh = fh
        head = fh.read(8)
        if len(head) != 8 or head[:2] not in (b"II", b"MM"):
            raise ContractError("legacy bridge requires a classic TIFF byte-order marker")
        self.endian = "<" if head[:2] == b"II" else ">"
        version = struct.unpack(self.endian + "H", head[2:4])[0]
        if version != 42:
            raise ContractError("legacy bridge supports classic TIFF version 42 only")
        self.ifd_offset = struct.unpack(self.endian + "I", head[4:8])[0]

    def _read_at(self, offset: int, n: int) -> bytes:
        if offset < 0 or n < 0:
            raise ContractError("negative TIFF range")
        self.fh.seek(offset)
        data = self.fh.read(n)
        if len(data) != n:
            raise ContractError("TIFF range extends beyond file")
        return data

    def entries(self) -> Dict[int, Tuple[int, int, bytes]]:
        self.fh.seek(self.ifd_offset)
        raw_n = self.fh.read(2)
        if len(raw_n) != 2:
            raise ContractError("truncated TIFF IFD")
        n = struct.unpack(self.endian + "H", raw_n)[0]
        raw_entries = []
        for _ in range(n):
            raw = self.fh.read(12)
            if len(raw) != 12:
                raise ContractError("truncated TIFF IFD entry")
            raw_entries.append(raw)
        out: Dict[int, Tuple[int, int, bytes]] = {}
        for raw in raw_entries:
            tag, typ, count = struct.unpack(self.endian + "HHI", raw[:8])
            if typ not in _TIFF_TYPE_SIZE:
                raise ContractError(f"unsupported TIFF field type {typ} for tag {tag}")
            total = _TIFF_TYPE_SIZE[typ] * count
            if total <= 4:
                value = raw[8:8 + total]
            else:
                offset = struct.unpack(self.endian + "I", raw[8:12])[0]
                value = self._read_at(offset, total)
            out[tag] = (typ, count, value)
        return out

    def decode(self, entry: Tuple[int, int, bytes]):
        typ, count, raw = entry
        e = self.endian
        if typ in (1, 7):
            return tuple(raw)
        if typ == 2:
            return raw.rstrip(b"\x00").decode("utf-8", "replace")
        if typ == 3:
            return struct.unpack(e + "H" * count, raw)
        if typ == 4:
            return struct.unpack(e + "I" * count, raw)
        if typ == 5:
            vals = struct.unpack(e + "I" * (2 * count), raw)
            return tuple(vals[i] / vals[i + 1] if vals[i + 1] else float("nan") for i in range(0, len(vals), 2))
        if typ == 9:
            return struct.unpack(e + "i" * count, raw)
        if typ == 10:
            vals = struct.unpack(e + "i" * (2 * count), raw)
            return tuple(vals[i] / vals[i + 1] if vals[i + 1] else float("nan") for i in range(0, len(vals), 2))
        if typ == 11:
            return struct.unpack(e + "f" * count, raw)
        if typ == 12:
            return struct.unpack(e + "d" * count, raw)
        raise ContractError("unsupported TIFF type")


def _scalar(value, name: str) -> int:
    if not isinstance(value, tuple) or len(value) != 1:
        raise ContractError(f"{name} must contain exactly one value")
    return int(value[0])


def _xmp_attr(xmp: str, name: str) -> Optional[str]:
    m = re.search(r"\btr:" + re.escape(name) + r'="([^"]*)"', xmp)
    return None if m is None else m.group(1)


def _hash_file(path: str) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as fh:
        for block in iter(lambda: fh.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def inspect_legacy_linearraw_v17(
    path: str,
    expected: LegacyLinearRawExpectationV17 = FROZEN_LEGACY_EXPECTATION_V17,
) -> LegacyLinearRawInspectionV17:
    expected.validate()
    file_sha = _hash_file(path)
    if file_sha != expected.file_sha256:
        raise ContractError("legacy LinearRaw file SHA-256 mismatch")
    file_bytes = os.path.getsize(path)

    with open(path, "rb") as fh:
        r = _ClassicTiffReader(fh)
        e = r.entries()
        required = (256, 257, 258, 259, 262, 273, 274, 277, 278, 279, 700, 50714, 50717)
        missing = [tag for tag in required if tag not in e]
        if missing:
            raise ContractError(f"legacy LinearRaw missing required TIFF/DNG tags: {missing}")
        width = _scalar(r.decode(e[256]), "ImageWidth")
        height = _scalar(r.decode(e[257]), "ImageLength")
        bits = tuple(int(v) for v in r.decode(e[258]))
        compression = _scalar(r.decode(e[259]), "Compression")
        photometric = _scalar(r.decode(e[262]), "PhotometricInterpretation")
        strip_offsets = tuple(int(v) for v in r.decode(e[273]))
        orientation = _scalar(r.decode(e[274]), "Orientation")
        spp = _scalar(r.decode(e[277]), "SamplesPerPixel")
        rows_per_strip = _scalar(r.decode(e[278]), "RowsPerStrip")
        strip_counts = tuple(int(v) for v in r.decode(e[279]))
        xmp_raw = e[700][2]
        xmp = xmp_raw.decode("utf-8", "replace")
        black = tuple(float(v) for v in r.decode(e[50714]))
        white = tuple(int(v) for v in r.decode(e[50717]))

        if (width, height, spp, bits, photometric, compression, orientation) != (
            expected.width, expected.height, expected.samples_per_pixel, expected.bits_per_sample,
            expected.photometric_interpretation, expected.compression, expected.orientation,
        ):
            raise ContractError("legacy LinearRaw geometry/storage tags differ from frozen expectation")
        if len(black) != 3 or max(abs(a - b) for a, b in zip(black, expected.black_level)) > 1e-12:
            raise ContractError("legacy LinearRaw BlackLevel mismatch")
        if white != expected.white_level:
            raise ContractError("legacy LinearRaw WhiteLevel mismatch")
        if len(strip_offsets) != len(strip_counts) or not strip_offsets:
            raise ContractError("legacy LinearRaw strip offset/count cardinality mismatch")
        expected_payload_bytes = width * height * spp * 2
        if sum(strip_counts) != expected_payload_bytes:
            raise ContractError("legacy LinearRaw strip payload byte count mismatch")
        payload_hash = hashlib.sha256()
        last_end = -1
        for offset, count in zip(strip_offsets, strip_counts):
            if offset < 0 or count <= 0 or offset + count > file_bytes:
                raise ContractError("legacy LinearRaw strip points outside file")
            if offset < last_end:
                raise ContractError("legacy LinearRaw strips overlap or are out of order")
            last_end = offset + count
            payload_hash.update(r._read_at(offset, count))
        payload_sha = payload_hash.hexdigest()
        if payload_sha != expected.pixel_payload_sha256:
            raise ContractError("legacy LinearRaw pixel payload SHA-256 mismatch")

    xmp_source = _xmp_attr(xmp, "SourceSHA256")
    xmp_payload = _xmp_attr(xmp, "LinearRawPixelPayloadSHA256")
    xmp_class = _xmp_attr(xmp, "Classification")
    xmp_photon = _xmp_attr(xmp, "MeasuredPhotonClaim")
    xmp_master = _xmp_attr(xmp, "ScientificMasterSHA256")
    xmp_daf = _xmp_attr(xmp, "DynamicAuthoritySHA256")
    if xmp_source != expected.source_dng_sha256:
        raise ContractError("legacy LinearRaw XMP source binding mismatch")
    if xmp_payload != expected.pixel_payload_sha256:
        raise ContractError("legacy LinearRaw XMP payload binding mismatch")
    if xmp_class != expected.classification:
        raise ContractError("legacy LinearRaw XMP classification mismatch")
    if xmp_photon != expected.measured_photon_claim:
        raise ContractError("legacy LinearRaw must explicitly deny a new measured-photon claim")
    if xmp_master is not None or xmp_daf is not None:
        raise ContractError("legacy LinearRaw unexpectedly claims modern Scientific Master/Dynamic Authority identity")

    return LegacyLinearRawInspectionV17(
        file_sha256=file_sha,
        pixel_payload_sha256=payload_sha,
        file_bytes=file_bytes,
        pixel_payload_bytes=expected_payload_bytes,
        width=width,
        height=height,
        samples_per_pixel=spp,
        bits_per_sample=bits,  # type: ignore[arg-type]
        photometric_interpretation=photometric,
        compression=compression,
        orientation=orientation,
        black_level=black,  # type: ignore[arg-type]
        white_level=white,  # type: ignore[arg-type]
        rows_per_strip=rows_per_strip,
        strip_count=len(strip_offsets),
        xmp_source_sha256=xmp_source,
        xmp_payload_sha256=xmp_payload,
        xmp_classification=xmp_class,
        xmp_measured_photon_claim=xmp_photon,
        xmp_scientific_master_sha256=xmp_master,
        xmp_dynamic_authority_sha256=xmp_daf,
    )


def dequantize_triplet_v17(codes: Tuple[int, int, int], expected: LegacyLinearRawExpectationV17 = FROZEN_LEGACY_EXPECTATION_V17) -> Tuple[float, float, float]:
    expected.validate()
    if len(codes) != 3:
        raise ContractError("legacy RGB sample requires three codes")
    out = []
    for code, black, white in zip(codes, expected.black_level, expected.white_level):
        if not (0 <= int(code) <= 65535):
            raise ContractError("legacy LinearRaw code must be uint16")
        out.append((float(code) - float(black)) / (float(white) - float(black)))
    return tuple(out)  # type: ignore[return-value]


def build_frozen_bridge_manifest_v17(inspection: LegacyLinearRawInspectionV17) -> LegacyBridgeManifestV17:
    if inspection.file_sha256 != LEGACY_DNG_SHA256_V17 or inspection.pixel_payload_sha256 != LEGACY_PIXEL_PAYLOAD_SHA256_V17:
        raise ContractError("inspection is not the frozen exact v0.4 tele LinearRaw")
    manifest = LegacyBridgeManifestV17(
        inspection=inspection,
        source_bound_color_transform_sha256=SOURCE_BOUND_P3_TRANSFORM_SHA256_V17,
        theoretical_quantization_half_step=THEORETICAL_QUANTIZATION_HALF_STEP_V17,
        recorded_max_quantization_abs_scene_error=RECORDED_MAX_QUANTIZATION_ABS_SCENE_ERROR_V17,
        legacy_dequantized_f32_sha256=LEGACY_DEQUANTIZED_F32_SHA256_V17,
    )
    return manifest.validate()


def frozen_real_observation_v17() -> dict:
    """Repository-safe record of the locally verified exact artifact.

    This is evidence about the historical compatibility payload, not a substitute
    for persisting the modern float Scientific Master or Dynamic Authority Field.
    """
    return {
        "schema": "TruthRawHdrLegacyLinearObservation/1.7",
        "source_dng": SOURCE_DNG_FILENAME_V17,
        "source_dng_sha256": SOURCE_DNG_SHA256_V17,
        "legacy_linearraw_dng": LEGACY_DNG_FILENAME_V17,
        "legacy_linearraw_sha256": LEGACY_DNG_SHA256_V17,
        "pixel_payload_sha256": LEGACY_PIXEL_PAYLOAD_SHA256_V17,
        "dequantized_float32_sha256": LEGACY_DEQUANTIZED_F32_SHA256_V17,
        "geometry": [4080, 3072, 3],
        "storage": {
            "bits_per_sample": [16, 16, 16],
            "compression": 1,
            "photometric_interpretation": 34892,
            "orientation": 3,
            "black_level": [2048.0, 2048.0, 2048.0],
            "white_level": [34816, 34816, 34816],
            "formula": "scene_linear=(stored_code-2048)/32768",
            "theoretical_quantization_half_step": THEORETICAL_QUANTIZATION_HALF_STEP_V17,
            "recorded_max_quantization_abs_scene_error": RECORDED_MAX_QUANTIZATION_ABS_SCENE_ERROR_V17,
        },
        "decoded_real_payload": {
            "scene_min": -0.00634765625,
            "scene_max": 1.297760009765625,
            "negative_component_count": 41873,
            "over_one_component_count": 125419,
        },
        "source_bound_p3": {
            "transform_sha256": SOURCE_BOUND_P3_TRANSFORM_SHA256_V17,
            "authority": "SOURCE_METADATA_BOUND_L1_NOT_PHYSICAL_CALIBRATION",
            "matrix_rgb_to_p3_d65": CAMERA_TO_P3_D65_V17,
            "legacy_payload_p3_component_min": -0.018184110248606337,
            "legacy_payload_p3_component_max": 2.618985313022203,
            "legacy_payload_positive_luminance_max": 1.382045787901244,
            "legacy_payload_positive_luminance_peak_ev_vs_unity": 0.46680541376074286,
        },
        "authority": {
            "classification": "LEGACY_DERIVED_RECONSTRUCTED_LINEAR_RAW_COMPATIBILITY_PAYLOAD",
            "new_sensor_evidence": False,
            "scientific_master_sha256": None,
            "dynamic_authority_sha256": None,
            "v16_science_binding_complete": False,
            "reason": "The historical v0.4 DNG stores a quantized reconstructed RGB compatibility payload and XMP source/payload lineage but no modern Scientific Master or Dynamic Authority identity.",
        },
    }
