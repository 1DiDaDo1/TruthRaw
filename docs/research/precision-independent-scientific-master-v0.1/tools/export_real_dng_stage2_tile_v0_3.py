#!/usr/bin/env python3
"""Export one exact real-DNG Stage-2 tile as F32 and F64 research inputs.

Host research tool only. It does not create or upgrade photographic evidence.
It currently accepts the uncompressed single-IFD CFA DNG family used by the
TruthRaw HONOR/MotionCam precision audit and fails closed for unsupported forms.
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


def _sdk_f32_gain(m, height, width, row, col):
    scale_v = 1.0 / float(height)
    scale_h = 1.0 / float(width)
    offset_v = 0.5
    offset_h = 0.5

    row_f = (scale_v * (float(row) + offset_v) - m["originV"]) / m["spacingV"]
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

    knots = (
        m["samples"][r0, :, 0] * (np.float32(1.0) - fy)
        + m["samples"][r1, :, 0] * fy
    ).astype(np.float32)

    col_f = (scale_h * (float(col) + offset_h) - m["originH"]) / m["spacingH"]
    if col_f <= 0.0:
        return np.float32(knots[0])
    last_c = m["pointsH"] - 1
    if col_f >= float(last_c):
        return np.float32(knots[last_c])
    c0 = int(col_f)
    base = float(knots[c0])
    delta = float(knots[c0 + 1]) - base
    # For an isolated pixel this is equivalent to the SDK reset value. Full-row
    # audits use gainmap_precision_v0_2.cpp to test incremental stepping itself.
    return np.float32(base + delta * (col_f - float(c0)))


def _f64_gain(m, height, width, row, col):
    scale_v = 1.0 / float(height)
    scale_h = 1.0 / float(width)
    row_f = (scale_v * (float(row) + 0.5) - m["originV"]) / m["spacingV"]
    col_f = (scale_h * (float(col) + 0.5) - m["originH"]) / m["spacingH"]
    row_f = min(max(row_f, 0.0), float(m["pointsV"] - 1))
    col_f = min(max(col_f, 0.0), float(m["pointsH"] - 1))
    r0, c0 = int(math.floor(row_f)), int(math.floor(col_f))
    r1 = min(r0 + 1, m["pointsV"] - 1)
    c1 = min(c0 + 1, m["pointsH"] - 1)
    fy, fx = row_f - r0, col_f - c0
    s = m["samples"][:, :, 0].astype(np.float64)
    v0 = s[r0, c0] * (1.0 - fy) + s[r1, c0] * fy
    v1 = s[r0, c1] * (1.0 - fy) + s[r1, c1] * fy
    return v0 * (1.0 - fx) + v1 * fx


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

    f32 = np.empty((th, tw), dtype=np.float32)
    f64 = np.empty((th, tw), dtype=np.float64)

    for yy in range(th):
        gy = y0 + yy
        for xx in range(tw):
            gx = x0 + xx
            phase = (gy & 1) * 2 + (gx & 1)
            g32 = np.float32(1.0)
            g64 = 1.0
            for m in maps:
                in_area = (
                    gy >= m["top"] and gy < m["bottom"]
                    and gx >= m["left"] and gx < m["right"]
                    and ((gy - m["top"]) % m["rowPitch"] == 0)
                    and ((gx - m["left"]) % m["colPitch"] == 0)
                )
                if in_area:
                    g32 = _sdk_f32_gain(m, H, W, gy, gx)
                    g64 = _f64_gain(m, H, W, gy, gx)
                    break
            b32 = np.float32(black[phase])
            w32 = np.float32(white)
            denom32 = max(np.float32(w32 - b32), np.float32(1.0))
            s32 = np.float32(np.float32(np.float32(raw[gy, gx]) - b32) / denom32)
            f32[yy, xx] = np.float32(s32 * g32)

            b64 = float(black[phase])
            denom64 = max(white - b64, 1.0)
            f64[yy, xx] = ((float(raw[gy, gx]) - b64) / denom64) * g64

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
        "f32_sha256": hashlib.sha256(p32.read_bytes()).hexdigest(),
        "f64_sha256": hashlib.sha256(p64.read_bytes()).hexdigest(),
        "max_abs_stage2_delta": float(np.max(np.abs(f32.astype(np.float64) - f64))),
    }, indent=2))

    print(manifest)


if __name__ == "__main__":
    main()
