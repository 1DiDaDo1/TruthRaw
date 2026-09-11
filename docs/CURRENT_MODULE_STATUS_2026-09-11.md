# TruthRaw current module status — 2026-09-11

This is the global module-status dashboard. A module-local README remains authoritative for that exact version/branch contract, but does not override this cross-branch status index.

| Module / layer | Current reference | Audited status | Important boundary |
|---|---|---|---|
| Promoted `main` | `514f2f4bde6aba5a6709e176c03b22c3b9aea912` | PROMOTED | Research PR stack is not automatically in main. |
| Canonical reconstruction v4.7i | canonical bytes on audited lineage | SEALED / integrity-protected | Scientific single-frame baseline; do not edit for integration convenience. |
| Building Runtime v0.1 | research/promoted lineage | VALIDATED architecture/runtime | Resource authority cannot alter truth authority. |
| Technical Backplane v0.1 | main head lineage | PROMOTED / validated | 180-byte lineage binding; not pixels/calibration. |
| Tile-Native DNG Source v0.1 | `c5a51610fbb7750c1e8bf95e97c4b1349fbaec7e` clean branch lineage; reused in later CI | VALIDATED strict subset | Not universal DNG/RAW reader. |
| Full-Frame Streaming v0.1 | clean/integration lineage; exercised by later Room/Preview CI | VALIDATED research | Two-pass bounded adapter model; production master container still separate. |
| Room ABI / Adaptive All-Room v0.2 | PR #9, head `497bbea144f2e0b30c203aa7c5b3f40c8f47a16e` | OPEN DRAFT, CI-validated research | Host/resource integration proof, not physical Android proof. |
| Android On-Device Validation v0.1 | PR #10, head `596d671124ce3df949dfff91272c9b828fb4f2fa` | OPEN DRAFT, host/APK research | Physical-device evidence still pending. |
| Adaptive UI + Ingress v0.1 | PR #11, head `66ee5b37206ca8226394a26138e1f34ba85b92ec` | OPEN DRAFT, APK CI PASS | UI has no camera/scientific authority. |
| Adaptive UI Tile Preview v0.2 | PR #12, head `ca5837e86dd75689227ff24db5c397272767b80f` | OPEN DRAFT, APK CI PASS | Historical gray CFA `SOURCE_PROXY`, not scientific color. |
| Professional RAW Ingress v0.1 | `dadc6f95c82007da7060462cb10ced39921fc366` | CI-validated research | Classifier/admission, not decoder implementation. |
| Professional RAW Decoder Adapter v0.1 | `5bfff6aa5e042efe782fc629cca5605a11713531` overlay | CI-validated research contract | No real vendor codec implemented by this module. |
| LibRaw Compatibility Probe v0.1 | branch head `4f2232278fcff4cb4e7d9dff99e03fcf8ec7efff`; validated implementation `1d6a165cad53bfb8257cfb764ee63c4c5c5819d9` | METADATA PROBE CI PASS | No `unpack()`/pixel scientific admission. |
| LibRaw borrowed-fd datastream v0.1 | `f428394dcb6a6ee2ff2e683927a9d1dd632377c0` | IMPLEMENTED; no standalone current-head workflow indexed | Do not inflate into pixel-decode support. |
| Professional RAW Gatehouse Runtime v0.1 | PR #13, head `8d417d2c8bda7eb011b68c5acadac0dc01405cfe` | OPEN DRAFT, matrix PASS | Separate memory/failure domain; no second truth/master. |
| Decoded Measurement Handoff v0.1 | PR #14, head `b4c25c267867769313ce7c79be1fb14968a8b13f` | OPEN DRAFT; workflow `34599504412` SUCCESS | Bounded persisted handoff; same evidence lineage. |
| Decoded Measurement Tile Source v0.1 | PR #15, head `0edca8edab8afed9aad83b133c8f578e8b7860e4` | OPEN DRAFT; workflow `34599879528` SUCCESS | Fail-closed on unmodeled metadata such as GainMap/residual black. |
| Decoded Measurement Main-House E2E v0.1 | branch `b45b32f1302df0355cf733bf0d42fc6eca90d561` | IMPLEMENTED BRANCH, **NO CURRENT-HEAD CI RUN FOUND** | Must not be called E2E-proven. |
| Preview Representation v0.1 | PR #16, head `cbf8867fae04034bfe32a3a5ca0c6903be92ed3c` | OPEN DRAFT research policy | JPEG/sRGB is compatibility preview, never evidence. |
| Reconstructed Color Preview v0.1 | PR #17, `372ffe1565e10070db74586298ba82042500af81` | OPEN DRAFT, GCC/Clang/sanitizer/APK PASS | Appearance preview surface only. |
| Scientific Preview Source Binding v0.1 | PR #18 branch head `0b4bb8536f2f2329d40f330a8b34c54cb9cee5f0`; validated implementation `badf78ac578495425cf8ece0eef686944b3468b2` | OPEN DRAFT, matrix PASS | Requires exact source hash and authorized color binding. |
| DNG Color Binding Producer v0.1 | PR #19, head `988d498d37dc71629e25c3b5aed66faf7b5b2939`; implementation `6c0211790a48956f706298b2308118eae2d3f948` | OPEN DRAFT, matrix PASS | `SOURCE_METADATA_BOUND`, not independent spectral calibration. |
| Scientific Preview Source Binding v0.2 | head `74f561b10ff65db67dc0fa3db9b84ea6bdc1a079` | TWO-PHASE research gate validated | Phase 1 can release labeled appearance; scientific release remains blocked. |
| Android Source-Bound Color Preview v0.1 | PR #20, current head `2fdf05ca1bbbc59cd8867df0cae117d1eec92d51`; implementation `65818f529d4588c2ed907fb11787e3dd83d4432c` | OPEN DRAFT, current-head Android workflow PASS | Build/package proof only; physical Honor execution pending. |

## Professional RAW support statement

Current code has meaningful compatibility architecture but no basis for saying every listed professional RAW format is scientifically supported. The truthful progression is:

`container/probe recognized -> decoder available -> byte/sample semantics proven -> topology proven -> source bound -> certification matrix entry -> scientific admission`.

Any missing step lowers admission or fails closed.

## Physical status

`FULL_PHYSICAL` remains blocked where independent electron/color/illuminant/optics calibration or physical-device evidence is missing. Source metadata can be faithfully rendered without being relabeled independent physical calibration.