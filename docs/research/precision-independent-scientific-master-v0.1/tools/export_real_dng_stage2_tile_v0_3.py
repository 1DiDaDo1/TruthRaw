#!/usr/bin/env python3
"""Export one real-DNG Stage-2 tile as paired F32/F64 research inputs.

Host research tool only. It does not create or upgrade photographic evidence.
It currently accepts the uncompressed single-IFD CFA DNG family used by the
TruthRaw HONOR/MotionCam precision audit and fails closed for unsupported forms.

The F32 GainMap lane intentionally reproduces the validated SDK-structured row
interpolation/stepping used by `gainmap_precision_v0_2.cpp`. This matters for
branch-sensitive downstream reconstruction: per-pixel recomputation is not a
valid substitute for the historical incremental F32 row semantics.
"""

import argparse
import hashlib
import json
import math
import struct
from pathlib import Path

import numpy as np
import tifffile


def _black_phase(tag_value):
    v = tuple(tag_value)
    if len(v) == 8:
        return [v[i] / v[i + 1] for i in range(0, 8, 2)]
    if len(v) == 4:
        return [float(x) for x in v]
    raise ValueError("unsupported BlackLevel representation")


def _parse_gainmaps(blob):
    off = 0

    def u32():
        nonlocal off
        v = struct.unpack_from(">I", blob, off)[0]
        off += 4
        return v

    def f64():
        nonlocal off
        v = struct.unpack_from(">d", blob, off)[0]
        off += 8
        return v

    count = u32()
    maps = []
    for _ in range(count):
        opcode_id = u32()
        version = u32()
        flags = u32()
        byte_count = u32()
        start = off
        if opcode_id != 9:  # GainMap
            off += byte_count
            continue

        top, left, bottom, right = u32(), u32(), u32(), u32()
        plane, planes, row_pitch, col_pitch = u32(), u32(), u32(), u32()
        points_v, points_h = u32(), u32()
        spacing_v, spacing_h = f64(), f64()
        origin_v, origin_h = f64(), f64()
        map_planes = u32()
        n = points_v * points_h * map_planes
        samples = np.frombuffer(blob, dtype=">f4", count=n, offset=off).astype(np.float32)
        samples = samples.reshape(points_v, points_h, map_planes)
        off += 4 * n
        if off - start != byte_count:
            raise ValueError("GainMap opcode byte-count mismatch")
        if map_planes != 1:
            raise ValueError("unsupported GainMap mapPlanes != 1")
        maps.append({
            "version": version,
            "flags": flags,
            "top": top,
            "left": left,
            "bottom": bottom,
            "right": right,
            "plane": plane,
            "planes": planes,
            "rowPitch": row_pitch,
            "colPitch": col_pitch,
            "pointsV": points_v,
            "pointsH": points_h,
            "spacingV": spacing_v,
            "spacingH": spacing_h,
            "originV": origin_v,
            "originH": origin_h,
            "mapPlanes": map_planes,
            "samples": samples,
        })
    return maps


def _sdk_f32_row(m, height, width, row):
    """Return full AreaSpec phase-row columns/gains with incremental F32 stepping."""
    if row < m["top"] or row >= m["bottom"] or ((row - m["top"]) % m["rowPitch"]):
        return None, None

    scale_v = 1.0 / float(height)
    scale_h = 1.0 / float(width)
    row_f = (scale_v * (float(row) + 0.5) - m["originV"]) / m["spacingV"]
    last_r = m["pointsV"] - 1
    if row_f <= 0.0:
        r0 = r1 = 0
        fy = np.float32(0.0)
    elif row_f >= float(last_r):
        r0 = r1 = last_r
        fy = np.float32(0.0)
    else:
        r0 = int(row_f)
        r1 = r0 + 1
        fy = np.float32(row_f - float(r0))

    one = np.float32(1.0)
    a = m["samples"][r0, :, 0]
    b = m["samples"][r1, :, 0]
    knots = (a * (one - fy) + b * fy).astype(np.float32)

    cols = np.arange(m["left"], m["right"], m["colPitch"], dtype=np.int32)
    gains = np.empty(cols.size, dtype=np.float32)
    col_f = ((scale_h * (cols.astype(np.float64) + 0.5) - m["originH"]) / m["spacingH"])
    seg = np.floor(col_f).astype(np.int32)
    seg = np.clip(seg, 0, m["pointsH"] - 1)

    for s in np.unique(seg):
        mask = seg == s
        c = cols[mask]
        cf = col_f[mask]
        if s <= 0 and np.all(cf <= 0.0):
            gains[mask] = knots[0]
            continue
        if s >= m["pointsH"] - 1:
            gains[mask] = knots[-1]
            continue

        # Match the SDK-style reset at the first phase sample in the segment,
        # then advance using a Float32 per-pixel step. valueIndex is expressed
        # in sensor columns, so colPitch is naturally represented in c-c0.
        c0 = int(c[0])
        cf0 = (scale_h * (float(c0) + 0.5) - m["originH"]) / m["spacingH"]
        base = float(knots[s])
        delta = float(knots[s + 1]) - base
        value_base = np.float32(base + delta * (cf0 - float(s)))
        value_step = np.float32((delta * scale_h) / m["spacingH"])
        value_index = (c - c0).astype(np.float32)
        gains[mask] = (value_base + value_step * value_index).astype(np.float32)

    return cols, gains


def _f64_row(m, height, width, row):
    if row < m["top"] or row >= m["bottom"] or ((row - m["top"]) % m["rowPitch"]):
        return None, None

    scale_v = 1.0 / float(height)
    scale_h = 1.0 / float(width)
    cols = np.arange(m["left"], m["right"], m["colPitch"], dtype=np.int32)
    row_f = (scale_v * (float(row) + 0.5) - m["originV"]) / m["spacingV"]
    col_f = (scale_h * (cols.astype(np.float64) + 0.5) - m["originH"]) / m["spacingH"]
    row_f = np.clip(row_f, 0.0, float(m["pointsV"] - 1))
    col_f = np.clip(col_f, 0.0, float(m["pointsH"] - 1))

    r0 = int(math.floor(float(row_f)))
    r1 = min(r0 + 1, m["pointsV"] - 1)
    fy = float(row_f) - r0
    c0 = np.floor(col_f).astype(np.int32)
    c1 = np.minimum(c0 + 1, m["pointsH"] - 1)
    fx = col_f - c0

    s = m["samples"][:, :, 0].astype(np.float64)
    v0 = s[r0, c0] * (1.0 - fy) + s[r1, c0] * fy
    v1 = s[r0, c1] * (1.0 - fy) + s[r1, c1] * fy
    return cols, v0 * (1.0 - fx) + v1 * fx


def _gain_tiles(maps, height, width, x0, y0, tw, th):
    g32 = np.ones((th, tw), dtype=np.float32)
    g64 = np.ones((th, tw), dtype=np.float64)

    for m in maps:
        row_start = max(y0, m["top"])
        row_end = min(y0 + th, m["bottom"])
        # Align first row to AreaSpec rowPitch.
        rem = (row_start - m["top"]) % m["rowPitch"]
        if rem:
            row_start += m["rowPitch"] - rem
        for gy in range(row_start, row_end, m["rowPitch"]):
            cols32, gains32 = _sdk_f32_row(m, height, width, gy)
            cols64, gains64 = _f64_row(m, height, width, gy)
            if cols32 is None:
                continue
            if not np.array_equal(cols32, cols64):
                raise ValueError("F32/F64 GainMap row coordinate mismatch")
            mask = (cols32 >= x0) & (cols32 < x0 + tw)
            if np.any(mask):
                lx = cols32[mask] - x0
                ly = gy - y0
                g32[ly, lx] = gains32[mask]
                g64[ly, lx] = gains64[mask]

    return g32, g64


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dng")
    ap.add_argument("--x0", type=int, required=True)
    ap.add_argument("--y0", type=int, required=True)
    ap.add_argument("--width", type=int, required=True)
    ap.add_argument("--height", type=int, required=True)
    ap.add_argument("--out-prefix", required=True)
    args = ap.parse_args()

    src = Path(args.dng)
    source_sha = hashlib.sha256(src.read_bytes()).hexdigest()

    with tifffile.TiffFile(src) as tf:
        if len(tf.pages) != 1:
            raise SystemExit("unsupported: expected one CFA IFD")
        page = tf.pages[0]
        if int(page.compression) != 1:
            raise SystemExit("unsupported: this research exporter currently requires uncompressed CFA data")
        raw = page.asarray()
        if raw.dtype != np.uint16 or raw.ndim != 2:
            raise SystemExit("unsupported: expected uint16 2D CFA")
        H, W = raw.shape
        x0, y0, tw, th = args.x0, args.y0, args.width, args.height
        if x0 < 0 or y0 < 0 or tw <= 0 or th <= 0 or x0 + tw > W or y0 + th > H:
            raise SystemExit("tile outside source bounds")
        black = _black_phase(page.tags["BlackLevel"].value)
        white = float(page.tags["WhiteLevel"].value)
        cfa_bytes = list(page.tags["CFAPattern"].value)
        if cfa_bytes == [2, 1, 1, 0]:
            cfa = "BGGR"
        elif cfa_bytes == [0, 1, 1, 2]:
            cfa = "RGGB"
        elif cfa_bytes == [1, 0, 2, 1]:
            cfa = "GRBG"
        elif cfa_bytes == [1, 2, 0, 1]:
            cfa = "GBRG"
        else:
            raise SystemExit("unsupported CFA pattern")
        maps = _parse_gainmaps(page.tags["OpcodeList2"].value) if "OpcodeList2" in page.tags else []

    g32, g64 = _gain_tiles(maps, H, W, x0, y0, tw, th)
    tile_raw = raw[y0:y0 + th, x0:x0 + tw]
    yy = np.arange(y0, y0 + th, dtype=np.int64)[:, None]
    xx = np.arange(x0, x0 + tw, dtype=np.int64)[None, :]
    phase = ((yy & 1) * 2 + (xx & 1)).astype(np.intp)

    b32 = np.take(np.asarray(black, dtype=np.float32), phase)
    b64 = np.take(np.asarray(black, dtype=np.float64), phase)
    w32 = np.float32(white)
    denom32 = np.maximum(w32 - b32, np.float32(1.0))
    denom64 = np.maximum(white - b64, 1.0)

    f32 = ((tile_raw.astype(np.float32) - b32) / denom32 * g32).astype(np.float32)
    f64 = (tile_raw.astype(np.float64) - b64) / denom64 * g64

    prefix = Path(args.out_prefix)
    p32 = Path(str(prefix) + ".f32.bin")
    p64 = Path(str(prefix) + ".f64.bin")
    manifest = Path(str(prefix) + ".json")
    p32.write_bytes(f32.astype("<f4", copy=False).tobytes(order="C"))
    p64.write_bytes(f64.astype("<f8", copy=False).tobytes(order="C"))
    manifest.write_text(json.dumps({
        "schema": "TruthRawRealDngStage2Tile/0.3",
        "authority": "NUMERICAL_DERIVATION_FROM_ONE_SOURCE_NO_EVIDENCE_UPGRADE",
        "source": src.name,
        "source_sha256": source_sha,
        "source_width": W,
        "source_height": H,
        "tile": {"x0": x0, "y0": y0, "width": tw, "height": th},
        "cfa": cfa,
        "black_phase": black,
        "white": white,
        "gainmap_count": len(maps),
        "f32_gain_semantics": "SDK_STRUCTURED_ROW_INCREMENTAL",
        "f64_gain_semantics": "DIRECT_DOUBLE_BILINEAR_REFERENCE",
        "f32_sha256": hashlib.sha256(p32.read_bytes()).hexdigest(),
        "f64_sha256": hashlib.sha256(p64.read_bytes()).hexdigest(),
        "max_abs_stage2_delta": float(np.max(np.abs(f32.astype(np.float64) - f64))),
    }, indent=2))

    print(manifest)


if __name__ == "__main__":
    main()
