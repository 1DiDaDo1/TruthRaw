#!/usr/bin/env python3
from pathlib import Path

p = Path("docs/handoff/DRAW_44489_KNOWLEDGE_CAPSULE.md")
s = p.read_text(encoding="utf-8")
marker = "## 23. Censored Illuminant Hue Floor v0.1 — CI-GREEN CANDIDATE 2026-10-10"
if marker in s:
    print("DRAW_44489_CENSORED_ILLUMINANT_HUE_FLOOR_V01_ALREADY_PRESENT")
    raise SystemExit(0)

section = r'''

---

## 23. Censored Illuminant Hue Floor v0.1 — CI-GREEN CANDIDATE 2026-10-10

This section supersedes section 22 only for the **current downstream warm-highlight test candidate**. Deep-Censor Chroma Guard v0.2 remains intact and is not weakened.

New real-device motivation: `DRAWJPEGtelelamptest.zip`, bytes `26,237,955`, SHA-256 `fc41536dfc1d384ceae852b314afbe91b766ddfb3d1625ea7f6745ef7b181f06`. Contained PRO tele JPEG `DRAW_CAPTURE_1791652795293_tele_4080x3072_draw_pro_fullres.jpg`, bytes `23,324,624`, SHA-256 `f984ac88ff72ed3dc5276ac20ad06f49673bc6aff1f80d1a1059321f700fa2e5`.

User judgement: purple suppression is now substantially improved, but the brightest lamp/highlight area is still too neutral white/gray and lacks the darker yellow/amber hue perceived in the real lamp. This is a new downstream Appearance residual, not permission to weaken the purple guard, darken highlights globally or recolour by detected hue.

New candidate: **Censored Illuminant Hue Floor v0.1**. Runtime order is:

`Near-Censor Shoulder -> accepted Censored Chroma Fallback -> Deep-Censor v0.2 -> Warm Illuminant v0.1 -> Censored Illuminant Hue Floor v0.1 -> historical highlight observer -> gamut fit`.

It is ADVANCED/PRO Appearance only; PURE bypasses it. It requires the existing valid source-bound white point and existing Warm Illuminant CCT gate, high CENSOR authority (`0.50 -> 0.80`), high luminance (`0.72 -> 0.95`) and nearly neutral output (full through relative chroma `0.015`, zero by `0.090`). Maximum introduced relative chroma floor is `0.060`. Chroma direction comes from the admitted source-white xy projected into the current D50 linear-sRGB basis; there is no hard-coded yellow/orange target. Clearly chromatic output and the strong-purple regression sample are exact no-op. Rec.709 luminance is preserved. No sharpening, blur, resampling, hue/object/semantic detector, camera/vendor identity, source mutation, Scientific-Master writeback, evidence creation or recovered-scene-colour claim is introduced.

Initial code/workflow commit: `a02e12133a8aa8b7777d6ef65ce20e7abc0c21a6`. Exact CI-generated runtime wiring: `2f4375e4371ace6770b6ae25690f025ee7191208`.

Exact successful workflow: `D.RAW Censored Illuminant Hue Floor v0.1`, run `38073401245`, job `114275277204`, **SUCCESS**. New hue-floor regression, Deep-Censor v0.2, Warm Illuminant, accepted fallback, Near-Censor Shoulder, Natural Light, gamut fit, PURE headroom, sealed Full-Frame Streaming, strict Q100/true-4:4:4 JPEG, Android tests/build and APK verification are green.

Artifact `11677282649`, archive digest SHA-256 `efdbf8c3fe4534690877f713690bd9396dcc91092021158dcde451bfb86b0cc8`. Exact extracted APK: `8,891,987` bytes, SHA-256 `abec7589b9da32a1fbc2ff0674c8f83db93440274a00d2ffc3777cc92e68e85b`.

Detailed handoff: `docs/handoff/DRAW_44489_CENSORED_ILLUMINANT_HUE_FLOOR_V0_1_CANDIDATE_2026-10-10.md`.

Classification: **CI-GREEN APPEARANCE CANDIDATE / REAL-DEVICE WARM-HIGHLIGHT VALIDATION PENDING**. PASS requires darker yellow/amber source-white appearance to return in the bright lamp highlight without reducing luminance/detail, without reopening purple/magenta, without global warm cast or valid-colour washout, and with Q100/true-4:4:4 intact. Scientific Master, sealed evidence and all scientific promotion/writeback firewalls remain unchanged.
'''
p.write_text(s.rstrip() + section.rstrip() + "\n", encoding="utf-8")
print("DRAW_44489_CENSORED_ILLUMINANT_HUE_FLOOR_V01_APPENDED")
