#!/usr/bin/env python3
from __future__ import annotations

import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "tools" / "high_fidelity_jpeg_codec_regression_v0_1.cpp"
INCLUDE = ROOT / "suite_android" / "app" / "src" / "main" / "cpp"
WIDTH = 256
HEIGHT = 192
CANONICAL_SOS = bytes([3, 1, 0x00, 2, 0x11, 3, 0x11, 0x00, 0x3F, 0x00])


def fail(message: str) -> None:
    raise SystemExit(f"HIGH_FIDELITY_JPEG_REGRESSION_FAIL: {message}")


def run(command: list[str], *, cwd: Path | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        cwd=cwd,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )


def parse_contract(path: Path) -> dict[str, int]:
    data = path.read_bytes()
    if len(data) <= 4 or data[:2] != b"\xff\xd8" or data[-2:] != b"\xff\xd9":
        fail("SOI/terminal EOI contract failed")

    offset = 2
    dqt_all_ones: dict[int, bool] = {}
    sof_seen = False
    sos_seen = False
    entropy_start = -1

    while offset < len(data):
        if data[offset] != 0xFF:
            fail(f"expected marker at offset {offset}")
        marker_start = offset
        while offset < len(data) and data[offset] == 0xFF:
            offset += 1
        if offset >= len(data):
            fail("truncated marker")
        marker = data[offset]
        offset += 1

        if marker == 0xD9:
            fail("EOI reached before SOS")
        if marker in range(0xD0, 0xD8) or marker == 0x01:
            continue
        if offset + 2 > len(data):
            fail("truncated segment length")
        length = (data[offset] << 8) | data[offset + 1]
        if length < 2 or offset + length > len(data):
            fail(f"invalid segment length for marker 0x{marker:02x}")
        payload = data[offset + 2 : offset + length]
        offset += length

        if marker == 0xDB:
            p = 0
            while p < len(payload):
                spec = payload[p]
                p += 1
                precision = spec >> 4
                table_id = spec & 0x0F
                if precision != 0 or p + 64 > len(payload):
                    fail("DQT must be 8-bit and complete")
                values = payload[p : p + 64]
                p += 64
                dqt_all_ones[table_id] = all(v == 1 for v in values)
            if p != len(payload):
                fail("DQT payload mismatch")

        elif marker == 0xC0:
            if len(payload) != 15:
                fail("SOF0 payload length mismatch")
            precision = payload[0]
            height = (payload[1] << 8) | payload[2]
            width = (payload[3] << 8) | payload[4]
            components = payload[5]
            if (precision, width, height, components) != (8, WIDTH, HEIGHT, 3):
                fail("SOF0 geometry/component contract failed")
            expected = [(1, 0x11, 0), (2, 0x11, 1), (3, 0x11, 1)]
            actual = []
            for c in range(3):
                base = 6 + c * 3
                actual.append((payload[base], payload[base + 1], payload[base + 2]))
            if actual != expected:
                fail(f"4:4:4 SOF contract failed: {actual!r}")
            sof_seen = True

        elif marker == 0xDA:
            if not sof_seen:
                fail("SOS before valid SOF0")
            if payload != CANONICAL_SOS:
                fail(f"non-canonical SOS payload: {payload.hex()}")
            if dqt_all_ones.get(0) is not True or dqt_all_ones.get(1) is not True:
                fail("Q100 DQT0/DQT1 all-one contract failed")
            sos_seen = True
            entropy_start = offset
            break

        elif 0xC1 <= marker <= 0xCF and marker not in (0xC4, 0xC8, 0xCC):
            fail(f"non-baseline SOF marker 0x{marker:02x} at {marker_start}")

    if not sos_seen or entropy_start < 0:
        fail("missing SOS")

    stuffed = 0
    i = entropy_start
    eoi_offset = -1
    while i < len(data) - 1:
        if data[i] != 0xFF:
            i += 1
            continue
        nxt = data[i + 1]
        if nxt == 0x00:
            stuffed += 1
            i += 2
            continue
        if nxt == 0xD9:
            eoi_offset = i
            break
        fail(f"unexpected entropy marker 0xff{nxt:02x} at offset {i}")

    if eoi_offset != len(data) - 2:
        fail("EOI is not terminal or entropy scan is truncated")

    return {
        "bytes": len(data),
        "entropy_start": entropy_start,
        "byte_stuffings": stuffed,
    }


def main() -> None:
    compiler = shutil.which("g++") or fail("g++ is required")
    ffmpeg = shutil.which("ffmpeg") or fail("ffmpeg is required for strict decode regression")
    ffprobe = shutil.which("ffprobe") or fail("ffprobe is required for pixel-format regression")

    with tempfile.TemporaryDirectory(prefix="draw_hf_jpeg_") as temp_dir:
        temp = Path(temp_dir)
        binary = temp / "encode_fixture"
        output = temp / "fixture.jpg"

        compiled = run([
            compiler,
            "-std=c++20",
            "-O2",
            "-Wall",
            "-Wextra",
            "-pedantic",
            f"-I{INCLUDE}",
            str(CPP),
            "-o",
            str(binary),
        ])
        if compiled.returncode != 0:
            fail(f"host encoder compile failed\n{compiled.stdout}\n{compiled.stderr}")
        if compiled.stderr.strip():
            print("HIGH_FIDELITY_JPEG_HOST_COMPILE_WARNINGS")
            print(compiled.stderr.strip())

        encoded = run([str(binary), str(output)])
        if encoded.returncode != 0 or not output.is_file() or output.stat().st_size <= 4:
            fail(f"host encoder failed rc={encoded.returncode}\n{encoded.stdout}\n{encoded.stderr}")

        metrics = parse_contract(output)

        decoded = run([
            ffmpeg,
            "-hide_banner",
            "-loglevel",
            "error",
            "-xerror",
            "-i",
            str(output),
            "-f",
            "null",
            "-",
        ])
        if decoded.returncode != 0:
            fail(f"strict FFmpeg decode failed\n{decoded.stdout}\n{decoded.stderr}")

        probed = run([
            ffprobe,
            "-v",
            "error",
            "-select_streams",
            "v:0",
            "-show_entries",
            "stream=width,height,pix_fmt",
            "-of",
            "default=noprint_wrappers=1:nokey=1",
            str(output),
        ])
        if probed.returncode != 0:
            fail(f"ffprobe failed\n{probed.stdout}\n{probed.stderr}")
        values = [line.strip() for line in probed.stdout.splitlines() if line.strip()]
        if len(values) != 3 or values[0] != str(WIDTH) or values[1] != str(HEIGHT) or "444" not in values[2]:
            fail(f"ffprobe geometry/pixel format mismatch: {values!r}")

        print(
            "HIGH_FIDELITY_JPEG_REGRESSION_PASS "
            f"bytes={metrics['bytes']} entropy_start={metrics['entropy_start']} "
            f"byte_stuffings={metrics['byte_stuffings']} pix_fmt={values[2]}"
        )


if __name__ == "__main__":
    main()
