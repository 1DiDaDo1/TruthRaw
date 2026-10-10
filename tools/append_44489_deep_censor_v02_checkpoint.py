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

Exact lineage:

- header `0ba2ecc50df5c2c072aa6028d4b7e83e527c964d`;
- applicator `06b8c137c24aa8383d3f076ec6b9093009d3eb5c`;
- initial regression `951082efa855eb1933bc63b8d359b9ac4071842d`;
- main APK workflow binding `5dbed408ee977a0783c83694b74ad1cf2a07bc21`;
- corrected harness `51e4bf5ae2a875e790b02d30568d76b027397116`;
- exact CI-generated runtime wiring `bd6ae77eba0314c9d87130931e0771a91ce749fb`.

The first main workflow stopped in the new static harness because it matched the word `semantic` inside a comment stating that semantic inference is forbidden. This is classified `HARNESS_FAILURE / NOT_RUNTIME_FAILURE`; APK assembly was skipped. The corrected harness then passed.

Exact successful main workflow: `D.RAW Free Raster v0.3 Finish APK`, run `38067210344`, job `114257180765`, **SUCCESS**. All v0.2 and pre-existing Appearance regressions, PURE/headroom checks, sealed-streaming integrity, strict Q100/true-4:4:4 JPEG regression, Android unit tests/build and APK verification are green.

Artifact `11675980316`, archive digest SHA-256 `ec0aa13ef39e53a9ee01f7934de11ed5ded4915af24551243b485eb1c2b8b208`. Exact APK: `8,888,139` bytes, SHA-256 `a77459fa4e8af1faace3165a1c29ae8b61aea987de47c21ab0d081b9e41192ff`.

Dedicated detailed record: `docs/handoff/DRAW_44489_DEEP_CENSOR_CHROMA_GUARD_V0_2_DETAIL_CANDIDATE_2026-10-10.md`.

Classification remains **CI-GREEN APPEARANCE CANDIDATE / REAL-DEVICE DETAIL VALIDATION PENDING**. PASS requires purple suppression at least as good as v0.1 while subtle cloud/foliage/local colour texture and perceived detail improve without halo, seam, ringing, oversharpening or legitimate-colour washout. Scientific Master, sealed evidence and all scientific promotion/writeback firewalls remain unchanged.
'''
p.write_text(s.rstrip() + section + '\n')
print('DRAW_44489_DEEP_CENSOR_V02_CHECKPOINT_APPENDED')
