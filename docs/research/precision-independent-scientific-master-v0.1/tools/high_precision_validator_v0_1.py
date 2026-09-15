#!/usr/bin/env python3
"""TruthRaw precision-independent reference validator v0.1.

Uses Decimal as an arbitrary-precision oracle for small deterministic scientific
operations. It does not process image evidence and cannot raise scientific
authority. Its job is only to quantify numerical representation error.
"""

from __future__ import annotations

import argparse
import json
import math
import struct
from decimal import Decimal, getcontext
from pathlib import Path
from typing import Iterable, List

getcontext().prec = 80


def d(x) -> Decimal:
    return Decimal(str(x))


def f32(x: float) -> float:
    return struct.unpack("!f", struct.pack("!f", float(x)))[0]


def normalize_decimal(code: int, black, white, gain) -> Decimal:
    b, w, g = d(black), d(white), d(gain)
    denom = max(w - b, Decimal(1))
    return (Decimal(code) - b) / denom * g


def normalize_f64(code: int, black, white, gain) -> float:
    b, w, g = float(black), float(white), float(gain)
    return (float(code) - b) / max(w - b, 1.0) * g


def normalize_f32(code: int, black, white, gain) -> float:
    b, w, g = f32(black), f32(white), f32(gain)
    denom = f32(max(f32(w - b), f32(1.0)))
    return f32(f32(f32(float(code)) - b) / denom * g)


def matmul_decimal(a: List[List[Decimal]], b: List[List[Decimal]]) -> List[List[Decimal]]:
    return [[sum((a[r][k] * b[k][c] for k in range(3)), Decimal(0)) for c in range(3)] for r in range(3)]


def transpose(m):
    return [[m[c][r] for c in range(3)] for r in range(3)]


def cov_decimal(a, c):
    ad = [[d(x) for x in row] for row in a]
    cd = [[d(x) for x in row] for row in c]
    return matmul_decimal(matmul_decimal(ad, cd), transpose(ad))


def matmul_f64(a, b):
    return [[sum(float(a[r][k]) * float(b[k][c]) for k in range(3)) for c in range(3)] for r in range(3)]


def cov_f64(a, c):
    af = [[float(x) for x in row] for row in a]
    cf = [[float(x) for x in row] for row in c]
    return matmul_f64(matmul_f64(af, cf), transpose(af))


def dot3_f32(row, col) -> float:
    s = f32(0.0)
    for x, y in zip(row, col):
        s = f32(s + f32(f32(x) * f32(y)))
    return s


def matmul_f32(a, b):
    af = [[f32(x) for x in row] for row in a]
    bf = [[f32(x) for x in row] for row in b]
    bt = transpose(bf)
    return [[dot3_f32(af[r], bt[c]) for c in range(3)] for r in range(3)]


def cov_f32(a, c):
    af = [[f32(x) for x in row] for row in a]
    cf = [[f32(x) for x in row] for row in c]
    return matmul_f32(matmul_f32(af, cf), transpose(af))


def abs_err(value: float, reference: Decimal) -> float:
    return abs(float(Decimal(str(value)) - reference))


def rel_err(value: float, reference: Decimal) -> float:
    denom = abs(reference)
    if denom == 0:
        return abs_err(value, reference)
    return float(abs(Decimal(str(value)) - reference) / denom)


def flatten(m: Iterable[Iterable]):
    return [x for row in m for x in row]


def evaluate() -> dict:
    normalization_cases = [
        (64, 64, 1023, 1.0),
        (65, 64, 1023, 1.0),
        (512, 64, 1023, 1.0),
        (1022, 64, 1023, 1.0),
        (1023, 64, 1023, 1.0),
        (1000, 63.75, 1023.0, 1.0000001192092896),
        (4095, 63.9375, 4095.0, 7.12503125),
        (65535, 64.00390625, 65535.0, 0.031250000931322574),
    ]

    norm_rows = []
    for code, black, white, gain in normalization_cases:
        ref = normalize_decimal(code, black, white, gain)
        v32 = normalize_f32(code, black, white, gain)
        v64 = normalize_f64(code, black, white, gain)
        norm_rows.append({
            "input": {"code": code, "black": black, "white": white, "gain": gain},
            "oracle": str(ref),
            "float32": v32,
            "float64": v64,
            "float32_abs_error": abs_err(v32, ref),
            "float64_abs_error": abs_err(v64, ref),
            "float32_rel_error": rel_err(v32, ref),
            "float64_rel_error": rel_err(v64, ref),
        })

    a = [
        [0.8123456789012345, 0.12123456789012345, -0.03123456789012345],
        [0.04567890123456789, 1.0234567890123456, 0.07123456789012345],
        [-0.012345678901234567, 0.09876543210987654, 0.9345678901234567],
    ]
    c = [
        [4.000000000123, 0.500000000321, 0.250000000111],
        [0.500000000321, 9.000000000456, 0.750000000222],
        [0.250000000111, 0.750000000222, 16.000000000789],
    ]
    cref = cov_decimal(a, c)
    c32 = cov_f32(a, c)
    c64 = cov_f64(a, c)
    flat_ref = flatten(cref)
    flat32 = flatten(c32)
    flat64 = flatten(c64)

    cov_rows = []
    for i, (ref, v32, v64) in enumerate(zip(flat_ref, flat32, flat64)):
        cov_rows.append({
            "index": i,
            "oracle": str(ref),
            "float32": v32,
            "float64": v64,
            "float32_abs_error": abs_err(v32, ref),
            "float64_abs_error": abs_err(v64, ref),
            "float32_rel_error": rel_err(v32, ref),
            "float64_rel_error": rel_err(v64, ref),
        })

    summary = {
        "normalization_max_abs_error_float32": max(r["float32_abs_error"] for r in norm_rows),
        "normalization_max_abs_error_float64": max(r["float64_abs_error"] for r in norm_rows),
        "covariance_max_abs_error_float32": max(r["float32_abs_error"] for r in cov_rows),
        "covariance_max_abs_error_float64": max(r["float64_abs_error"] for r in cov_rows),
        "covariance_max_rel_error_float32": max(r["float32_rel_error"] for r in cov_rows),
        "covariance_max_rel_error_float64": max(r["float64_rel_error"] for r in cov_rows),
    }
    summary["float64_not_worse_on_normalization_max_abs"] = (
        summary["normalization_max_abs_error_float64"] <= summary["normalization_max_abs_error_float32"]
    )
    summary["float64_not_worse_on_covariance_max_abs"] = (
        summary["covariance_max_abs_error_float64"] <= summary["covariance_max_abs_error_float32"]
    )

    return {
        "schema": "TruthRawPrecisionOracle/0.1",
        "authority": "NUMERICAL_REFERENCE_ONLY_NO_EVIDENCE_UPGRADE",
        "decimal_precision_digits": getcontext().prec,
        "normalization": norm_rows,
        "covariance": cov_rows,
        "summary": summary,
        "boundary": "Higher numerical precision measures/limits arithmetic error only. It does not add photographic evidence or change measured/reconstructed authority.",
    }


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out")
    ns = ap.parse_args()
    result = evaluate()
    text = json.dumps(result, indent=2, sort_keys=True)
    if ns.out:
        Path(ns.out).write_text(text + "\n", encoding="utf-8")
    print(text)


if __name__ == "__main__":
    main()
