#!/usr/bin/env python3
from pathlib import Path

# One-shot idempotent capsule checkpoint writer; rerun is a no-op after marker exists.
p = Path('docs/handoff/DRAW_44489_KNOWLEDGE_CAPSULE.md')
s = p.read_text()
marker = '## 22. Deep-Censor Chroma Guard v0.2 detail preservation — CI-GREEN CANDIDATE 2026-10-10'
if marker in s:
    print('DRAW_44489_DEEP_CENSOR_V02_CHECKPOINT_ALREADY_PRESENT')
    raise SystemExit(0)
section = r'''

---

## 22. Deep-Censor Chroma Guard v0.2 detail preservation — CI-GREEN CANDIDATE 2026-10-10

This section supersedes v0.1 as the **current deep-CENSOR real-device candidate**, but does not erase v0.1 provenance or physically promote v0.2.

Latest user test evidence is `DRAWPROJPEGtelepurpletest.zip`: `26,779,992` bytes, SHA-256 `f01957a30d60dadcc1774e53daf0b30fd9c10bb6769469f06e2044f85ec19ded`. Contained PRO JPEG `DRAW_CAPTURE_1791647288671_tele_4080x3072_draw_pro_fullres.jpg`: `17,952,362` bytes, SHA-256 `59111a7b474147f42ee344d900744448ac8c12f0b964398ad03eea82ddbdac66`. User judgement after Deep-Censor Guard v0.1: the purple result is **very good**, with remaining request to increase detail preservation.

Interpretation: v0.1 already preserves Rec.709 luminance algebraically; the residual flattening is therefore treated as reduced subtle chroma/colour microcontrast, not as permission to add sharpening. A generic edge exemption was rejected because real purple residual can overlap true luminance edges.

**Deep-Censor Chroma Guard v0.2** preserves the v0.1 CENSOR thresholds (`0.50 -> 0.80`) and maximum extra contraction (`0.78`) but adds bounded low-amplitude relative-chroma relief. Relative chroma <= `0.02` may receive at most `35%` relief from the **additional deep-guard contraction only**; relief smoothly falls to zero by relative chroma `0.12`. Clearly chromatic residuals >= `0.12`, including the locked strong-purple sample, receive the exact v0.1 response. The stage never expands chroma, preserves Rec.709 luminance and chroma-vector direction, and uses no sharpening, blur, spatial edge detector, hue detector, semantics, object identity, camera/vendor identity or brightness threshold. Accepted upstream Censored Chroma Fallback v0.1 remains unchanged.

Exact implementation lineage retained for v0.2 development:

- header `0ba2ecc50df5c2c072aa6028d4b7e83e527c964d`;
- applicator `06b8c137c24aa8383d3f076ec6b9093009d3eb5c`;
- initial regression `951082efa855eb1933bc63b8d359b9ac4071842d`;
- main APK workflow binding `5dbed408ee977a0783c83694b74ad1cf2a07bc21`;
- corrected harness `51e4bf5ae2a875e790b02d30568d76b027397116`;
- earlier exact CI-generated runtime wiring `bd6ae77eba0314c9d87130931e0771a91ce749fb`.

The first main workflow stopped in the new static harness because it matched the word `semantic` inside a comment stating that semantic inference is forbidden. This is classified `HARNESS_FAILURE / NOT_RUNTIME_FAILURE`; APK assembly was skipped. The harness was corrected without changing the v0.2 pixel algorithm.

### Final exact v0.2 APK checkpoint

The current exact tested project head for the downloadable v0.2 APK is:

`0db27c2cfc6eab768d41dcdbb1014b43311bb5a2`

On this exact head:

- workflow `D.RAW Free Raster v0.3 Finish APK`;
- run `38067653940`;
- job `114258469264`;
- conclusion **SUCCESS**;
- `Apply deep-censor chroma detail guard v0.2`: SUCCESS;
- `Deep-censor chroma detail preservation regression v0.2`: SUCCESS;
- all pre-existing Appearance regressions: SUCCESS;
- PURE/headroom checks: SUCCESS;
- sealed Full-Frame Streaming integrity: SUCCESS;
- strict Q100/true-4:4:4 JPEG codec regression: SUCCESS;
- Android unit tests/build: SUCCESS;
- APK verification: SUCCESS;
- artifact upload: SUCCESS.

GitHub Actions artifact:

- artifact id `11676305345`;
- name `draw-free-raster-v03-fullres-candidate-apk`;
- archive digest SHA-256 `0695f32050009ffdcce63583e1bb5331d09c825466c4076c6bb86638a8c47e38`.

Exact extracted APK:

- member `app-debug.apk`;
- bytes `8,888,139`;
- SHA-256 `32ec63317e9cbdfecb353b1b98a30ef0770a742370da44503a7132278b454756`.

This final exact APK checkpoint supersedes the earlier v0.2 APK hash/build identifiers for **current testing**, while those earlier runs remain historical provenance.

Dedicated detailed record: `docs/handoff/DRAW_44489_DEEP_CENSOR_CHROMA_GUARD_V0_2_DETAIL_CANDIDATE_2026-10-10.md`.

Classification remains **CI-GREEN APPEARANCE CANDIDATE / REAL-DEVICE DETAIL VALIDATION PENDING**. PASS requires purple suppression at least as good as v0.1 while subtle cloud/foliage/local colour texture and perceived detail improve without halo, seam, ringing, oversharpening or legitimate-colour washout. Scientific Master, sealed evidence and all scientific promotion/writeback firewalls remain unchanged.
'''
p.write_text(s.rstrip() + section + '\n')
print('DRAW_44489_DEEP_CENSOR_V02_CHECKPOINT_APPENDED')
