#!/usr/bin/env python3
"""Compare TruthRaw Camera-5 pre/post-HAL evidence captures without inventing semantics.

This is an observation/differencing tool. It never modifies source evidence and never promotes
unknown HONOR/QTI vendor fields to calibrated sensor meaning.

Typical use:
    python3 tools/compare_camera5_airlock_evidence.py capture_a.json capture_b.json -o diff.json

Best use is controlled same-scene captures from different readout/raster domains
(4080x3072, 8160x6144, 16320x12288) so stable vs changing route fields can be isolated.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any

SCHEMA = "truthraw.camera5-airlock-comparison.v0.1"

FOCUS_VENDOR_KEYS = [
    "com.hihonor.capture.metadata.binningFactor",
    "com.hihonor.capture.metadata.slaveBinningFactor",
    "com.hihonor.capture.metadata.isInSensorZoom",
    "com.hihonor.capture.metadata.sensorCustomMetaData",
    "com.hihonor.capture.metadata.sensorStages",
    "com.hihonor.capture.metadata.sensorZoomRatio",
    "com.hihonor.capture.metadata.allISPCropWindow",
    "com.hihonor.capture.metadata.AECRealCropWindow",
    "com.hihonor.capture.metadata.fdActiveArrayBoundary",
    "com.hihonor.capture.metadata.previewCameraPhysicalId",
    "com.hihonor.capture.metadata.previewPhysicalCam",
    "com.hihonor.capture.metadata.masterSensorSlotId",
    "com.hihonor.capture.metadata.SensorColorFilterArr",
    "com.qti.chi.multicamerainfo.ActiveCameraInfo",
    "com.qti.chi.multicamerainfo.MasterCamera",
    "com.qti.chi.multicamerainfo.MultiCameraIds",
    "org.quic.camera2.streamconfigs.StreamFormatInfo",
]


def load(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as f:
        data = json.load(f)
    if not isinstance(data, dict):
        raise ValueError(f"{path}: top-level JSON must be an object")
    return data


def stable_json(value: Any) -> str:
    return json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"))


def digest(value: Any) -> str:
    return hashlib.sha256(stable_json(value).encode("utf-8")).hexdigest()


def get(obj: Any, *path: str, default: Any = None) -> Any:
    cur = obj
    for key in path:
        if not isinstance(cur, dict) or key not in cur:
            return default
        cur = cur[key]
    return cur


def decode_encoded_value(value: Any) -> Any:
    """Preserve the recorded representation; expose preview arrays as the comparable payload."""
    if isinstance(value, dict) and {"type", "length", "preview"}.issubset(value):
        return {
            "type": value.get("type"),
            "length": value.get("length"),
            "preview": value.get("preview"),
            "truncated": value.get("truncated"),
        }
    return value


def vendor_map(evidence: dict[str, Any]) -> dict[str, Any]:
    entries = get(evidence, "halBufferEnvelope", "physicalCaptureResultMetadata", "vendorEntries", default=[])
    out: dict[str, Any] = {}
    if isinstance(entries, list):
        for entry in entries:
            if isinstance(entry, dict) and isinstance(entry.get("name"), str):
                out[entry["name"]] = decode_encoded_value(entry.get("value"))
    return out


def route_candidate_map(evidence: dict[str, Any]) -> dict[str, Any]:
    entries = get(evidence, "preHalRequestGate", "routeCandidateRequestState", default=[])
    out: dict[str, Any] = {}
    if isinstance(entries, list):
        for entry in entries:
            if isinstance(entry, dict) and isinstance(entry.get("name"), str):
                out[entry["name"]] = {
                    k: entry.get(k)
                    for k in (
                        "availableOnLogicalRequest",
                        "availableOnPhysicalRequest",
                        "availableAsLogicalSessionKey",
                        "availableAsPhysicalSessionKey",
                        "availableAsPhysicalOverride",
                        "builderDefaultOrCurrentValue",
                    )
                }
    return out


def byte_preview(value: Any) -> list[int] | None:
    if not isinstance(value, dict) or value.get("type") != "byte[]":
        return None
    p = value.get("preview")
    if not isinstance(p, list) or not all(isinstance(x, int) for x in p):
        return None
    return [x & 0xFF for x in p]


def u32le_words(data: list[int]) -> list[int]:
    n = len(data) // 4
    return [
        data[4*i] | (data[4*i+1] << 8) | (data[4*i+2] << 16) | (data[4*i+3] << 24)
        for i in range(n)
    ]


def summarize(path: Path, e: dict[str, Any]) -> dict[str, Any]:
    vm = vendor_map(e)
    sensor_custom = byte_preview(vm.get("com.hihonor.capture.metadata.sensorCustomMetaData"))
    native_desc = get(e, "halBufferEnvelope", "hardwareBuffer", "nativeDescriptor", default={})
    return {
        "file": path.name,
        "evidenceSchema": e.get("schema"),
        "createdAtUtc": e.get("createdAtUtc"),
        "authority": e.get("authority"),
        "boundary": e.get("boundary"),
        "captureRoute": {
            "physicalResultCameraId": get(e, "captureRoute", "physicalResultCameraId"),
            "width": get(e, "captureRoute", "width"),
            "height": get(e, "captureRoute", "height"),
            "sampleCount": get(e, "captureRoute", "sampleCount"),
            "captureResultSensorPixelMode": get(e, "captureRoute", "captureResultSensorPixelMode"),
            "requestedMaximumResolution": get(e, "captureRoute", "requestedMaximumResolution"),
            "outputMaximumResolutionModeDeclared": get(e, "captureRoute", "outputMaximumResolutionModeDeclared"),
        },
        "captureResult": {
            "sensorTimestampNs": get(e, "captureResult", "sensorTimestampNs"),
            "iso": get(e, "captureResult", "iso"),
            "exposureTimeNs": get(e, "captureResult", "exposureTimeNs"),
            "focalLengthMm": get(e, "captureResult", "focalLengthMm"),
            "rawBinningFactorUsed": get(e, "captureResult", "rawBinningFactorUsed"),
        },
        "rawPayload": {
            "bytes": get(e, "rawPayload", "bytes"),
            "sha256": get(e, "rawPayload", "sha256"),
            "rowStride": get(e, "rawPayload", "rowStride"),
            "pixelStride": get(e, "rawPayload", "pixelStride"),
            "canonicalContiguousRawSensor": get(e, "rawPayload", "canonicalContiguousRawSensor"),
        },
        "preHal": {
            "sessionGatePresent": isinstance(e.get("preHalSessionGate"), dict),
            "requestGatePresent": isinstance(e.get("preHalRequestGate"), dict),
            "routeCandidates": route_candidate_map(e),
        },
        "postHalHardwareBuffer": native_desc if isinstance(native_desc, dict) else native_desc,
        "focusVendorFields": {k: vm.get(k) for k in FOCUS_VENDOR_KEYS if k in vm},
        "sensorCustomMetaDataU32LE": u32le_words(sensor_custom) if sensor_custom else None,
        "fullVendorResultFingerprintSha256": digest(vm),
        "routeCandidateFingerprintSha256": digest(route_candidate_map(e)),
    }


def changes(values: list[Any], labels: list[str]) -> dict[str, Any]:
    pairs = [{"capture": labels[i], "value": values[i]} for i in range(len(values))]
    fingerprints = [digest(v) for v in values]
    return {
        "changed": len(set(fingerprints)) > 1,
        "values": pairs,
        "valueFingerprints": fingerprints,
    }


def compare_focus_vendor(summaries: list[dict[str, Any]]) -> dict[str, Any]:
    labels = [s["file"] for s in summaries]
    keys = sorted({k for s in summaries for k in s.get("focusVendorFields", {})})
    return {
        key: changes([s.get("focusVendorFields", {}).get(key) for s in summaries], labels)
        for key in keys
    }


def compare_route_candidates(summaries: list[dict[str, Any]]) -> dict[str, Any]:
    labels = [s["file"] for s in summaries]
    keys = sorted({k for s in summaries for k in s.get("preHal", {}).get("routeCandidates", {})})
    return {
        key: changes([s.get("preHal", {}).get("routeCandidates", {}).get(key) for s in summaries], labels)
        for key in keys
    }


def sensor_custom_diff(summaries: list[dict[str, Any]]) -> dict[str, Any]:
    labels = [s["file"] for s in summaries]
    arrays = []
    for s in summaries:
        v = s.get("focusVendorFields", {}).get("com.hihonor.capture.metadata.sensorCustomMetaData")
        arrays.append(byte_preview(v) or [])
    max_len = max((len(x) for x in arrays), default=0)
    changed_offsets = []
    for i in range(max_len):
        vals = [a[i] if i < len(a) else None for a in arrays]
        if len(set(vals)) > 1:
            changed_offsets.append({"offset": i, "values": dict(zip(labels, vals))})
    words = [u32le_words(a) for a in arrays]
    max_words = max((len(x) for x in words), default=0)
    changed_words = []
    for i in range(max_words):
        vals = [a[i] if i < len(a) else None for a in words]
        if len(set(vals)) > 1:
            changed_words.append({"u32leWordIndex": i, "byteOffset": i*4, "values": dict(zip(labels, vals))})
    return {
        "semantics": "RAW_BYTE_DIFFERENCE_ONLY__NO_FIELD_MEANING_ASSIGNED",
        "changedByteOffsets": changed_offsets,
        "changedAlignedU32LEWords": changed_words,
    }


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("evidence", nargs="+", type=Path, help="Two or more v0.16/v0.17 evidence JSON files")
    ap.add_argument("-o", "--output", type=Path)
    args = ap.parse_args()
    if len(args.evidence) < 2:
        ap.error("provide at least two evidence JSON files")

    loaded = [(p, load(p)) for p in args.evidence]
    summaries = [summarize(p, e) for p, e in loaded]
    labels = [s["file"] for s in summaries]

    report = {
        "schema": SCHEMA,
        "authority": "OBSERVATION_DIFFERENCE_ONLY_NO_VENDOR_SEMANTIC_PROMOTION",
        "sourceFiles": labels,
        "sourceEvidenceSha256": [digest(e) for _, e in loaded],
        "captures": summaries,
        "comparisons": {
            "captureDimensions": changes([
                [s["captureRoute"].get("width"), s["captureRoute"].get("height")]
                for s in summaries
            ], labels),
            "returnedSensorPixelMode": changes([
                s["captureRoute"].get("captureResultSensorPixelMode") for s in summaries
            ], labels),
            "rawBinningFactorUsed": changes([
                s["captureResult"].get("rawBinningFactorUsed") for s in summaries
            ], labels),
            "hardwareBufferDescriptor": changes([
                s.get("postHalHardwareBuffer") for s in summaries
            ], labels),
            "routeCandidates": compare_route_candidates(summaries),
            "focusVendorFields": compare_focus_vendor(summaries),
            "sensorCustomMetaData": sensor_custom_diff(summaries),
        },
        "interpretationBoundary": [
            "A changed field is correlation evidence, not proof that the field caused the route change.",
            "An unchanged field is useful negative evidence but does not prove irrelevance.",
            "Unknown HONOR/QTI vendor fields retain UNKNOWN_VENDOR_SEMANTICS until independently validated.",
            "No comparison upgrades app-visible RAW_SENSOR to untouched photodiode/ADC evidence.",
        ],
    }

    text = json.dumps(report, indent=2, ensure_ascii=False)
    if args.output:
        args.output.write_text(text + "\n", encoding="utf-8")
        print(args.output)
    else:
        print(text)


if __name__ == "__main__":
    main()
