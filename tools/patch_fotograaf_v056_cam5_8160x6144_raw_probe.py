#!/usr/bin/env python3
from pathlib import Path

P = Path("suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt")
s = P.read_text(encoding="utf-8")

repls = {
    'private val cameraThread = HandlerThread("truthraw-200mp-v053").apply { start() }':
    'private val cameraThread = HandlerThread("truthraw-cam5-8160-v056").apply { start() }',
    'private const val TARGET_W = 16320':
    'private const val TARGET_W = 8160',
    'private const val TARGET_H = 12288':
    'private const val TARGET_H = 6144',
    'private const val TARGET_SAMPLES = 200_540_160L':
    'private const val TARGET_SAMPLES = 50_135_040L',
    'button("Stap 3 · PHYSICAL-SCOPED CAPTURE · 16320×12288")':
    'button("Stap 3 · PHYSICAL-SCOPED TEST · 8160×6144")',
    '"Donkere preview is geen blokkade. Stage 3 PASS vereist 16320×12288 RAW_SENSOR + physical Camera-5 result + timestampidentiteit. Returned SENSOR_PIXEL_MODE wordt pas ná sealing geïnterpreteerd."':
    '"Donkere preview is geen blokkade. v0.56 TEST vereist 8160×6144 RAW_SENSOR + physical Camera-5 result + timestampidentiteit. RAW wordt vóór interpretatie verzegeld; geen native-ADC-promotie."',
    '"schema", "truthraw.fotograaf-camera5-200mp-v014-route-replay.v0.53"':
    '"schema", "truthraw.fotograaf-camera5-8160x6144-raw-probe.v0.56"',
}

for old,new in repls.items():
    if old not in s:
        raise SystemExit(f"required anchor missing: {old}")
    s=s.replace(old,new,1)

# Do not alter source-first sealing, physical Camera-5 binding, timestamp identity,
# topology audit/admission, or scientific authority gates.
P.write_text(s,encoding="utf-8")
print("patched", P)
