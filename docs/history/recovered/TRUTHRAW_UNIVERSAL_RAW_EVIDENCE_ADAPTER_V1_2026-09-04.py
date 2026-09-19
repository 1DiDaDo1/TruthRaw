#!/usr/bin/env python3
"""TruthRaw Universal RAW Evidence Adapter v1.

Goal: normalize arbitrary RAW-like inputs into an auditable Evidence Contract.
It classifies *storage representation* separately from *capture/processing lineage*.
It never assumes '.dng == direct sensor'.

Supported v1 adapters:
  - DNG/TIFF metadata probe (CFA, LinearRaw, monochrome/unknown)
  - TruthRaw legacy Truth Certificate upgrade
  - TruthRaw Capture Evidence v1/v1.1 manifest upgrade
  - opaque vendor RAW registry stub (CR3/NEF/ARW/RAF/RW2/ORF/etc.)

Pixel decoding is deliberately separate from classification. This metadata path should
not allocate the full sensor image.
"""
from __future__ import annotations
import argparse, hashlib, json, math, os, pathlib, re, time
from dataclasses import dataclass
from typing import Any, Dict, List, Optional, Tuple

CONTRACT_VERSION = "truthraw.universal_evidence.v1"

DNG_PHOTOMETRIC_CFA = 32803
DNG_PHOTOMETRIC_LINEAR_RAW = 34892

CFA_COLOR_NAMES = {
    0: "R", 1: "G", 2: "B", 3: "C", 4: "M", 5: "Y", 6: "W"
}

VENDOR_RAW_EXTENSIONS = {
    ".cr2": "Canon", ".cr3": "Canon", ".nef": "Nikon", ".nrw": "Nikon",
    ".arw": "Sony", ".srf": "Sony", ".sr2": "Sony", ".raf": "Fujifilm",
    ".rw2": "Panasonic", ".orf": "OM System/Olympus", ".pef": "Pentax",
    ".iiq": "Phase One", ".3fr": "Hasselblad", ".fff": "Hasselblad",
    ".rwl": "Leica", ".dcr": "Kodak", ".kdc": "Kodak", ".mos": "Leaf",
}


def _jsonable(v: Any) -> Any:
    if isinstance(v, bytes):
        return list(v)
    if hasattr(v, "tolist"):
        return v.tolist()
    if isinstance(v, tuple):
        return [_jsonable(x) for x in v]
    if isinstance(v, list):
        return [_jsonable(x) for x in v]
    if isinstance(v, (str, int, float, bool)) or v is None:
        return v
    try:
        return int(v)
    except Exception:
        return str(v)


def _rat_pairs(v: Any, n: Optional[int] = None) -> Optional[List[float]]:
    """Decode tifffile's flat RATIONAL tuple where appropriate."""
    if v is None:
        return None
    if isinstance(v, (int, float)):
        return [float(v)]
    if not isinstance(v, (tuple, list)):
        return None
    a = list(v)
    if n is not None and len(a) == 2 * n:
        out = []
        for i in range(n):
            den = float(a[2*i+1])
            out.append(float(a[2*i]) / den if den else math.nan)
        return out
    if n is not None and len(a) == n:
        return [float(x) for x in a]
    return None


def _safe_tag(page, name: str, default=None):
    t = page.tags.get(name)
    return default if t is None else t.value


def _cfa_matrix(dim: Optional[Tuple[int, int]], pattern: Any) -> Optional[List[List[str]]]:
    if not dim or pattern is None:
        return None
    h, w = map(int, dim)
    vals = list(pattern if not isinstance(pattern, bytes) else pattern)
    if len(vals) < h*w:
        return None
    return [[CFA_COLOR_NAMES.get(int(vals[y*w+x]), f"CFA_{int(vals[y*w+x])}") for x in range(w)] for y in range(h)]


def _pattern_name(matrix: Optional[List[List[str]]]) -> str:
    if matrix is None:
        return "UNKNOWN"
    h, w = len(matrix), len(matrix[0])
    if (h, w) == (2, 2):
        s = "".join(matrix[0] + matrix[1])
        if s in {"RGGB", "GRBG", "GBRG", "BGGR"}:
            return s
    return f"PERIODIC_{w}x{h}"


def _shift_periodic_pattern(matrix: Optional[List[List[str]]], x: int, y: int) -> Optional[List[List[str]]]:
    if matrix is None:
        return None
    h, w = len(matrix), len(matrix[0])
    return [[matrix[(yy+y) % h][(xx+x) % w] for xx in range(w)] for yy in range(h)]


def _lineage_from_dng(make: str, model: str, software: str, representation: str) -> Tuple[str, str, List[str]]:
    text = " ".join([make or "", model or "", software or ""]).lower()
    reasons: List[str] = []
    if "truthraw virtual ideal camera" in text:
        return "VIRTUAL_RECONSTRUCTED_RAW", "high", ["TruthRaw Virtual Ideal Camera identity"]
    computational_tokens = ["hdr+", "proraw", "expert raw", "computational raw"]
    found = [t for t in computational_tokens if t in text]
    if found:
        reasons.append("computational-writer hint(s): " + ", ".join(found))
        if representation == "CFA_MOSAIC":
            return "COMPUTATIONAL_CFA_OR_MERGED_RAW", "high", reasons
        return "COMPUTATIONAL_LINEAR_RAW", "high", reasons
    # Storage alone cannot certify single exposure.
    if representation == "CFA_MOSAIC":
        return "DIRECT_CFA_STORAGE_UNCERTIFIED", "medium", ["CFA storage detected; no computational writer hint; lineage still not independently certified"]
    if representation == "LINEAR_RAW":
        return "LINEAR_SCENE_RAW_UNCERTIFIED", "medium", ["LinearRaw storage detected; upstream processing lineage not independently certified"]
    return "UNKNOWN", "low", ["No trusted lineage signal"]


def _evidence_tier(lineage: str, representation: str) -> str:
    if lineage in {"DIRECT_SENSOR_SINGLE_EXPOSURE_CERTIFIED", "DIRECT_SENSOR_MULTI_FRAME_CERTIFIED"}: return "A_PHYSICAL_DIRECT"
    if lineage in {"DIRECT_CFA_STORAGE_UNCERTIFIED", "DIRECT_SENSOR_UNCERTIFIED"}: return "B_PHYSICAL_OR_DIRECT_UNCERTIFIED"
    if lineage == "REMOSAICED_SENSOR_CFA": return "C_REMOSAICED"
    if lineage == "COMPUTATIONAL_CFA_OR_MERGED_RAW": return "D_COMPUTATIONAL_CFA"
    if lineage == "VIRTUAL_RECONSTRUCTED_RAW": return "V_VIRTUAL_RECONSTRUCTED"
    if lineage in {"LINEAR_SCENE_RAW", "LINEAR_SCENE_RAW_UNCERTIFIED"}: return "E_LINEAR_SCENE"
    if lineage == "COMPUTATIONAL_LINEAR_RAW": return "F_COMPUTATIONAL_LINEAR"
    return "G_UNKNOWN_OR_OPAQUE"


def _route(lineage: str, representation: str) -> str:
    if lineage == "DIRECT_SENSOR_SINGLE_EXPOSURE_CERTIFIED": return "direct_physical_cfa_solver"
    if lineage == "DIRECT_SENSOR_MULTI_FRAME_CERTIFIED": return "direct_physical_burst_solver"
    if representation in {"CFA_MOSAIC", "PACKED_CFA"} and lineage in {"DIRECT_CFA_STORAGE_UNCERTIFIED", "DIRECT_SENSOR_UNCERTIFIED"}: return "direct_cfa_solver_with_provenance_guard"
    if lineage == "REMOSAICED_SENSOR_CFA": return "remosaic_aware_forward_model"
    if lineage == "COMPUTATIONAL_CFA_OR_MERGED_RAW": return "computational_cfa_guarded_solver"
    if lineage == "VIRTUAL_RECONSTRUCTED_RAW":
        return "virtual_linear_scene_ingest" if representation in {"LINEAR_RAW","LINEAR_RGB","MULTICHANNEL_LINEAR"} else "virtual_cfa_compatibility_projection_ingest"
    if representation in {"LINEAR_RAW", "LINEAR_RGB", "MULTICHANNEL_LINEAR"}:
        return "linear_scene_ingest_no_cfa_reconstruction"
    return "decode_or_classify_before_reconstruction"


def _base_contract(source_path: str) -> Dict[str, Any]:
    return {
        "contract_version": CONTRACT_VERSION,
        "source": {"path": os.path.abspath(source_path), "basename": os.path.basename(source_path)},
        "storage": {}, "sampling": {}, "lineage": {}, "radiometry": {}, "geometry": {},
        "color": {}, "processing_accounting": {}, "decoder": {}, "evidence_policy": {},
        "performance_hints": {}, "limitations": [], "blockers": [], "audit": {}
    }


def probe_dng(path: str) -> Dict[str, Any]:
    import tifffile
    t0 = time.perf_counter()
    c = _base_contract(path)
    with tifffile.TiffFile(path) as tf:
        page = tf.pages[0]
        photometric = int(page.photometric.value if hasattr(page.photometric, "value") else page.photometric)
        compression = int(page.compression.value if hasattr(page.compression, "value") else page.compression)
        spp = int(page.samplesperpixel or 1)
        bits = page.bitspersample
        bits = int(bits[0] if isinstance(bits, tuple) else bits)
        make = str(_safe_tag(page, "Make", ""))
        model = str(_safe_tag(page, "Model", ""))
        software = str(_safe_tag(page, "Software", ""))
        unique_model = str(_safe_tag(page, "UniqueCameraModel", ""))
        if photometric == DNG_PHOTOMETRIC_CFA:
            representation = "CFA_MOSAIC"
        elif photometric == DNG_PHOTOMETRIC_LINEAR_RAW:
            representation = "LINEAR_RAW"
        elif spp == 1:
            representation = "MONO_OR_UNKNOWN_SINGLE_PLANE"
        else:
            representation = "MULTICHANNEL_LINEAR_OR_UNKNOWN"
        dim = _safe_tag(page, "CFARepeatPatternDim")
        pat = _safe_tag(page, "CFAPattern")
        matrix = _cfa_matrix(dim, pat)
        pat_name = _pattern_name(matrix)
        crop_origin = _safe_tag(page, "DefaultCropOrigin")
        crop_size = _safe_tag(page, "DefaultCropSize")
        active_area = _safe_tag(page, "ActiveArea")
        crop_xy = [0, 0]
        if crop_origin is not None:
            ro = _rat_pairs(crop_origin, 2)
            if ro is not None and all(math.isfinite(x) for x in ro): crop_xy = [int(round(ro[0])), int(round(ro[1]))]
            elif isinstance(crop_origin, (tuple, list)) and len(crop_origin) >= 2: crop_xy = [int(crop_origin[0]), int(crop_origin[1])]
        effective_matrix = _shift_periodic_pattern(matrix, crop_xy[0], crop_xy[1])
        lineage, confidence, reasons = _lineage_from_dng(make, model, software, representation)
        black_dim = _safe_tag(page, "BlackLevelRepeatDim")
        black_raw = _safe_tag(page, "BlackLevel")
        black_n = None
        if isinstance(black_dim, (tuple, list)) and len(black_dim) == 2:
            black_n = int(black_dim[0]) * int(black_dim[1])
        black = _rat_pairs(black_raw, black_n) if black_n else _rat_pairs(black_raw, 1)
        white_raw = _safe_tag(page, "WhiteLevel")
        if isinstance(white_raw, (tuple, list)):
            white = [float(x) for x in white_raw]
        elif white_raw is not None: white = [float(white_raw)]
        else: white = None
        asn = _rat_pairs(_safe_tag(page, "AsShotNeutral"), 3)
        c["source"].update({"container": "DNG/TIFF", "make": make, "model": model, "software": software, "unique_camera_model": unique_model})
        c["storage"] = {
            "representation": representation, "shape": list(page.shape), "width": int(page.imagewidth), "height": int(page.imagelength),
            "samples_per_pixel": spp, "bits_per_sample": bits, "compression": compression,
            "photometric_interpretation": photometric,
            "dng_version": _jsonable(_safe_tag(page, "DNGVersion")), "dng_backward_version": _jsonable(_safe_tag(page, "DNGBackwardVersion"))
        }
        cfa_layout = _safe_tag(page, "CFALayout")
        cfa_plane_color = _safe_tag(page, "CFAPlaneColor")
        c["sampling"] = {
            "topology_family": "BAYER_2X2" if pat_name in {"RGGB","GRBG","GBRG","BGGR"} else ("PERIODIC_CFA" if matrix else ("LINEAR_RGB" if representation == "LINEAR_RAW" else "UNKNOWN")),
            "cfa_pattern_name": pat_name, "cfa_period": list(map(int, dim)) if dim is not None else None,
            "cfa_matrix_stored": matrix, "cfa_matrix_at_default_crop": effective_matrix,
            "default_crop_phase_shift_xy": crop_xy if matrix is not None else None,
            "cfa_layout": _jsonable(cfa_layout), "cfa_plane_color": _jsonable(cfa_plane_color),
            "single_sensel_semantics_certified": False,
        }
        c["lineage"] = {"classification": lineage, "confidence": confidence, "single_exposure_certified": False, "reasons": reasons}
        c["radiometry"] = {
            "black_level": black, "black_level_repeat_dim": _jsonable(black_dim), "white_level": white,
            "noise_profile_present": page.tags.get("NoiseProfile") is not None,
            "noise_reduction_applied": _rat_pairs(_safe_tag(page, "NoiseReductionApplied"), 1),
            "linearization_table_present": page.tags.get("LinearizationTable") is not None,
            "black_level_delta_h_present": page.tags.get("BlackLevelDeltaH") is not None,
            "black_level_delta_v_present": page.tags.get("BlackLevelDeltaV") is not None,
        }
        c["geometry"] = {
            "active_area": _jsonable(active_area), "default_crop_origin": _jsonable(crop_origin), "default_crop_size": _jsonable(crop_size),
            "orientation": _jsonable(_safe_tag(page, "Orientation")),
            "row_interleave_factor": _jsonable(_safe_tag(page, "RowInterleaveFactor")),
            "column_interleave_factor": _jsonable(_safe_tag(page, "ColumnInterleaveFactor")),
            "sub_tile_block_size": _jsonable(_safe_tag(page, "SubTileBlockSize")),
        }
        third_cal = any(page.tags.get(n) is not None for n in ("CalibrationIlluminant3","ColorMatrix3","CameraCalibration3","ReductionMatrix3","ForwardMatrix3"))
        c["color"] = {
            "as_shot_neutral": asn,
            "calibration_illuminant_1": _jsonable(_safe_tag(page,"CalibrationIlluminant1")),
            "calibration_illuminant_2": _jsonable(_safe_tag(page,"CalibrationIlluminant2")),
            "calibration_illuminant_3": _jsonable(_safe_tag(page,"CalibrationIlluminant3")),
            "color_matrix_1_present": page.tags.get("ColorMatrix1") is not None, "color_matrix_2_present": page.tags.get("ColorMatrix2") is not None, "color_matrix_3_present": page.tags.get("ColorMatrix3") is not None,
            "forward_matrix_1_present": page.tags.get("ForwardMatrix1") is not None, "forward_matrix_2_present": page.tags.get("ForwardMatrix2") is not None, "forward_matrix_3_present": page.tags.get("ForwardMatrix3") is not None,
            "camera_calibration_1_present": page.tags.get("CameraCalibration1") is not None, "camera_calibration_2_present": page.tags.get("CameraCalibration2") is not None, "camera_calibration_3_present": page.tags.get("CameraCalibration3") is not None,
            "third_calibration_set_present": third_cal,
        }
        opcode_presence = {f"opcode_list_{i}": page.tags.get(f"OpcodeList{i}") is not None for i in (1,2,3)}
        c["processing_accounting"] = {
            **opcode_presence,
            "gain_map_or_opcode_processing_may_be_required": bool(opcode_presence["opcode_list_2"]),
            "profile_gain_table_map_present": page.tags.get("ProfileGainTableMap") is not None,
            "upstream_processing_known": lineage in {"COMPUTATIONAL_CFA_OR_MERGED_RAW", "COMPUTATIONAL_LINEAR_RAW", "VIRTUAL_RECONSTRUCTED_RAW"},
        }
        c["decoder"] = {
            "metadata_backend": "tifffile-tags-only", "pixel_decoder_required": True,
            "recommended_pixel_backend": "LibRaw stage-0/direct raw_image for compressed/vendor DNG; native TIFF/DNG strip/tile decoder only when conformance-validated",
            "full_image_allocated_during_probe": False,
        }
        c["performance_hints"] = {
            "metadata_probe_only": True, "preferred_tile_pixels": 1024, "keep_compressed_source_immutable": True,
            "decode_once_or_tile_decode": True,
        }
        # fail-closed markers for constructs not yet interpreted by the reconstruction core
        if c["geometry"]["row_interleave_factor"] not in (None, 1): c["blockers"].append("non-default RowInterleaveFactor requires explicit mapping")
        if c["geometry"]["column_interleave_factor"] not in (None, 1): c["blockers"].append("non-default ColumnInterleaveFactor requires explicit mapping")
        if c["geometry"]["sub_tile_block_size"] not in (None, [1,1], (1,1)): c["blockers"].append("SubTileBlockSize requires explicit mapping")
        if representation == "CFA_MOSAIC" and matrix is None: c["blockers"].append("CFA photometric data without interpretable CFAPattern")
        if cfa_layout not in (None, 1): c["blockers"].append("non-default CFALayout requires explicit sample-coordinate mapping")
        if third_cal: c["blockers"].append("third DNG calibration set requires 1.6+/1.7 color interpolation support before color reconstruction")
        c["evidence_policy"] = {
            "evidence_tier": _evidence_tier(lineage, representation), "reconstruction_route": _route(lineage, representation),
            "allow_direct_sensor_noise_model": lineage in {"DIRECT_SENSOR_SINGLE_EXPOSURE_CERTIFIED", "DIRECT_SENSOR_MULTI_FRAME_CERTIFIED"},
            "allow_sqrt_n_multiframe_claim": False,
            "treat_noise_profile_as": "upstream/residual model only; not per-frame physical sensor noise" if lineage.startswith("COMPUTATIONAL") else "sensor prior with provenance caveat",
            "must_preserve_original_codes": True,
            "signal_changing_inverse_enabled_by_default": lineage not in {"COMPUTATIONAL_CFA_OR_MERGED_RAW", "COMPUTATIONAL_LINEAR_RAW"},
            "one_stored_sample_equals_one_physical_sensel_measurement": False if lineage in {"COMPUTATIONAL_CFA_OR_MERGED_RAW","COMPUTATIONAL_LINEAR_RAW","VIRTUAL_RECONSTRUCTED_RAW"} else None,
            "independence_budget": {"physical_frame_count_known": False, "independent_physical_measurements": None, "virtual_ev_evidence_multiplier": 1.0, "hypothesis_count_evidence_multiplier": 1.0},
        }
    c["audit"]["probe_ms"] = (time.perf_counter()-t0)*1000.0
    c["audit"]["contract_hash_sha256"] = "PENDING"
    return finalize_contract(c)


def upgrade_legacy_certificate(path: str) -> Dict[str, Any]:
    t0=time.perf_counter(); old=json.load(open(path,"r",encoding="utf-8")); c=_base_contract(path)
    mi=old.get("measurement_integrity",{}); mdi=old.get("metadata_integrity",{}); pa=old.get("processing_accounting",{})
    raw_kind=str(mi.get("raw_kind","")).upper(); representation="CFA_MOSAIC" if raw_kind=="CFA" else ("LINEAR_RAW" if "LINEAR" in raw_kind else "UNKNOWN")
    lineage=old.get("upstream_lineage") or "UNKNOWN"
    dims=mi.get("dimensions") or []
    c["source"].update({"container":"legacy_truth_certificate", "input_label":old.get("input_label"), "source_truthraw_version":old.get("truthraw_version")})
    c["storage"]={"representation":representation,"width":dims[0] if len(dims)>0 else None,"height":dims[1] if len(dims)>1 else None,"bits_per_sample":mi.get("bits_per_sample"),"compression":mi.get("compression")}
    p=mi.get("cfa"); c["sampling"]={"topology_family":"BAYER_2X2" if p in {"RGGB","GRBG","GBRG","BGGR"} else ("PERIODIC_CFA" if p else "UNKNOWN"),"cfa_pattern_name":p,"single_sensel_semantics_certified":bool(old.get("single_exposure_certified",False))}
    c["lineage"]={"classification":lineage,"confidence":old.get("provenance_confidence","unknown"),"single_exposure_certified":bool(old.get("single_exposure_certified",False)),"reasons":old.get("evidence",[])}
    crop=mdi.get("crop_geometry",{}); c["geometry"]={"active_area":crop.get("active_area"),"default_crop_origin":crop.get("default_crop_origin"),"default_crop_size":crop.get("default_crop_size")}
    c["radiometry"]={"white_level":mi.get("white_level"),"black_level_valid":mdi.get("black_level_valid"),"noise_profile_present":mdi.get("noise_profile_valid")}
    c["color"]={"as_shot_neutral_present":mdi.get("as_shot_neutral_valid"),"colour_route":mdi.get("colour_route"),"dual_calibration":mdi.get("dual_calibration")}
    c["processing_accounting"]={"known_upstream_processing":pa.get("known_upstream_processing",[]),"gainmap_nontrivial":pa.get("gainmap_nontrivial"),"linearization_table":pa.get("linearization_table")}
    c["decoder"]={"legacy_decoder_backend":pa.get("decoder_backend"),"pixel_decoder_required":False,"binary_test_note":old.get("binary_test_note")}
    c["limitations"]=list(old.get("limitations",[])); c["blockers"]=list(old.get("blockers",[]))
    c["evidence_policy"]={"evidence_tier":_evidence_tier(lineage,representation),"reconstruction_route":_route(lineage,representation),"allow_direct_sensor_noise_model":False if lineage.startswith("COMPUTATIONAL") else bool(old.get("single_exposure_certified",False)),"allow_sqrt_n_multiframe_claim":False,"treat_noise_profile_as":"upstream/residual model only; not per-frame physical sensor noise" if lineage.startswith("COMPUTATIONAL") else "sensor prior with provenance caveat","must_preserve_original_codes":True,"signal_changing_inverse_enabled_by_default":not lineage.startswith("COMPUTATIONAL"),"one_stored_sample_equals_one_physical_sensel_measurement":False if lineage.startswith("COMPUTATIONAL") else None,"independence_budget":{"physical_frame_count_known":False,"independent_physical_measurements":None,"virtual_ev_evidence_multiplier":1.0,"hypothesis_count_evidence_multiplier":1.0}}
    c["performance_hints"]={"prefer_tiled_decode":True,"preserve_decoder_stage_separation":True}
    c["audit"]["probe_ms"]=(time.perf_counter()-t0)*1000.0
    return finalize_contract(c)


def upgrade_capture_manifest(path: str) -> Dict[str, Any]:
    t0=time.perf_counter(); m=json.load(open(path,"r",encoding="utf-8")); c=_base_contract(path)
    static=m.get("camera_static",{}); frames=m.get("frames",[]); sess=m.get("session",{})
    formats=static.get("supported_raw_formats",[]); frame0=frames[0] if frames else {}
    fmt=frame0.get("format") or frame0.get("image_format") or (formats[0] if len(formats)==1 else None)
    representation="PACKED_CFA" if fmt in {"RAW10","RAW12","RAW14"} else ("CFA_MOSAIC" if fmt=="RAW_SENSOR" else "CAPABILITY_MANIFEST_ONLY")
    single_cert=bool(frames) and all((f.get("sensor_timestamp") is not None or f.get("sensor_timestamp_ns") is not None) for f in frames)
    # Distinguish physical-exposure count from storage streams and sample topology.
    binning_used = frame0.get("sensor_raw_binning_factor_used")
    timestamps = [(f.get("sensor_timestamp") if f.get("sensor_timestamp") is not None else f.get("sensor_timestamp_ns")) for f in frames]
    unique_ts = set(t for t in timestamps if t is not None)
    all_timestamped = bool(frames) and len(unique_ts) >= 1 and all(t is not None for t in timestamps)
    physical_n = len(unique_ts) if all_timestamped else 0
    if all_timestamped and physical_n == 1:
        lineage = "DIRECT_SENSOR_SINGLE_EXPOSURE_CERTIFIED"
    elif all_timestamped and physical_n > 1:
        lineage = "DIRECT_SENSOR_MULTI_FRAME_CERTIFIED"
    elif frames:
        lineage = "DIRECT_SENSOR_UNCERTIFIED"
    else:
        lineage = "CAPABILITY_ONLY"
    single_cert = lineage == "DIRECT_SENSOR_SINGLE_EXPOSURE_CERTIFIED"
    c["source"].update({"container":"TruthRaw Capture Evidence","schema_version":m.get("schema_version"),"device_model":sess.get("device_model"),"camera_id":sess.get("camera_id"),"capture_mode":sess.get("capture_mode")})
    c["storage"]={"representation":representation,"android_image_format":fmt,"supported_raw_formats":formats,"frame_count":len(frames)}
    p=static.get("cfa_arrangement"); c["sampling"]={"topology_family":"BINNING_FACTOR_CFA" if binning_used is True else ("BAYER_2X2" if p in {"RGGB","GRBG","GBRG","BGGR"} else "UNKNOWN_OR_VENDOR_CFA"),"cfa_pattern_name":p,"sensor_info_binning_factor":static.get("sensor_info_binning_factor"),"sensor_raw_binning_factor_used":binning_used,"single_sensel_semantics_certified":single_cert}
    c["lineage"]={"classification":lineage,"confidence":"high" if lineage in {"DIRECT_SENSOR_SINGLE_EXPOSURE_CERTIFIED","DIRECT_SENSOR_MULTI_FRAME_CERTIFIED"} else "medium","single_exposure_certified":single_cert,"reasons":[f"per-frame CaptureResult/SENSOR_TIMESTAMP establishes {physical_n} physical exposure(s)"] if all_timestamped else ["capability or capture metadata insufficient for strict physical-exposure certification"]}
    c["radiometry"]={"static_black_level":static.get("black_level_pattern_row_major_2x2"),"static_white_level":static.get("sensor_info_white_level"),"optical_black_regions":static.get("optical_black_regions",[]),"noise_profile":static.get("noise_profile")}
    c["geometry"]={"pixel_array_size":static.get("pixel_array_size"),"active_array":static.get("active_array"),"maximum_resolution_active_array":static.get("active_array_maximum_resolution"),"sensor_orientation_degrees":static.get("sensor_orientation_degrees")}
    c["processing_accounting"]={"lens_shading_applied_to_raw":static.get("lens_shading_applied_to_raw"),"requested_vs_returned_controls_required":True}
    c["decoder"]={"pixel_decoder_required":bool(frames),"recommended_pixel_backend":"native packed RAW10/12/14 decoder or zero-copy RAW_SENSOR uint16 view","full_image_allocated_during_probe":False}
    c["evidence_policy"]={"evidence_tier":_evidence_tier(lineage,"CFA_MOSAIC"),"reconstruction_route":_route(lineage,"CFA_MOSAIC"),"allow_direct_sensor_noise_model":lineage in {"DIRECT_SENSOR_SINGLE_EXPOSURE_CERTIFIED","DIRECT_SENSOR_MULTI_FRAME_CERTIFIED"},"allow_sqrt_n_multiframe_claim":False,"sqrt_n_upper_bound_eligible":physical_n>1,"must_preserve_original_codes":True,"signal_changing_inverse_enabled_by_default":lineage in {"DIRECT_SENSOR_SINGLE_EXPOSURE_CERTIFIED","DIRECT_SENSOR_MULTI_FRAME_CERTIFIED"},"one_stored_sample_equals_one_physical_sensel_measurement":True if lineage in {"DIRECT_SENSOR_SINGLE_EXPOSURE_CERTIFIED","DIRECT_SENSOR_MULTI_FRAME_CERTIFIED"} else None,"independence_budget":{"physical_frame_count_known":all_timestamped,"independent_physical_measurements":physical_n if all_timestamped else None,"storage_stream_count":len(frames),"virtual_ev_evidence_multiplier":1.0,"hypothesis_count_evidence_multiplier":1.0}}
    c["performance_hints"]={"keep_packed_frames_until_tile_decode":fmt in {"RAW10","RAW12","RAW14"},"prefer_uint16_zero_copy":fmt=="RAW_SENSOR","bounded_ring_buffer":True}
    if not frames: c["limitations"].append("Capability manifest only: no physical frame evidence is present")
    c["audit"]["probe_ms"]=(time.perf_counter()-t0)*1000.0
    return finalize_contract(c)


def opaque_vendor_contract(path: str) -> Dict[str, Any]:
    c=_base_contract(path); ext=pathlib.Path(path).suffix.lower(); vendor=VENDOR_RAW_EXTENSIONS.get(ext)
    c["source"].update({"container":"vendor_raw_opaque","extension":ext,"vendor_hint":vendor})
    c["storage"]={"representation":"OPAQUE_VENDOR_RAW"}; c["sampling"]={"topology_family":"UNKNOWN_UNTIL_DECODER_PROBE"}
    c["lineage"]={"classification":"UNKNOWN","confidence":"low","single_exposure_certified":False,"reasons":["container extension is not evidence of sampling/processing lineage"]}
    c["decoder"]={"pixel_decoder_required":True,"recommended_pixel_backend":"LibRaw 0.22+ / vendor SDK adapter with raw-stage metadata extraction","full_image_allocated_during_probe":False}
    c["evidence_policy"]={"evidence_tier":"G_UNKNOWN_OR_OPAQUE","reconstruction_route":"decode_or_classify_before_reconstruction","allow_direct_sensor_noise_model":False,"allow_sqrt_n_multiframe_claim":False,"must_preserve_original_codes":True,"signal_changing_inverse_enabled_by_default":False}
    c["blockers"].append("Raw pixel semantics must be established by a decoder adapter before Truth Scene reconstruction")
    return finalize_contract(c)


def finalize_contract(c: Dict[str, Any]) -> Dict[str, Any]:
    # Canonical hash excludes the hash itself.
    c.setdefault("audit",{})["contract_hash_sha256"]=""
    b=json.dumps(c,sort_keys=True,separators=(",",":"),ensure_ascii=False).encode("utf-8")
    c["audit"]["contract_hash_sha256"]=hashlib.sha256(b).hexdigest()
    return c


def classify(path: str, kind: Optional[str]=None) -> Dict[str, Any]:
    ext=pathlib.Path(path).suffix.lower()
    if kind == "legacy_certificate": return upgrade_legacy_certificate(path)
    if kind == "capture_manifest": return upgrade_capture_manifest(path)
    if kind == "dng" or ext==".dng": return probe_dng(path)
    if ext==".json":
        obj=json.load(open(path,"r",encoding="utf-8"))
        if str(obj.get("schema_version","")).startswith("truthraw.capture_evidence."): return upgrade_capture_manifest(path)
        if "measurement_integrity" in obj and "upstream_lineage" in obj: return upgrade_legacy_certificate(path)
    if ext in VENDOR_RAW_EXTENSIONS: return opaque_vendor_contract(path)
    c=opaque_vendor_contract(path); c["source"]["container"]="unknown"; return c


def main():
    ap=argparse.ArgumentParser(); ap.add_argument("input"); ap.add_argument("-o","--output"); ap.add_argument("--kind",choices=["dng","legacy_certificate","capture_manifest"])
    args=ap.parse_args(); c=classify(args.input,args.kind); text=json.dumps(c,indent=2,ensure_ascii=False)
    if args.output: open(args.output,"w",encoding="utf-8").write(text+"\n")
    else: print(text)

if __name__=="__main__": main()