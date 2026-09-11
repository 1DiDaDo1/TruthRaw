# TruthRaw CI evidence index — 2026-09-11

This index separates exact validated implementation anchors, later documentation overlays and unresolved branches. Failed runs stay failed history.

## Current integrated Android research head

Head: `2fdf05ca1bbbc59cd8867df0cae117d1eec92d51`

Fresh audit found these workflows successful on that exact head:

- Android Source-Bound Color Preview v0.1 APK Build — run `34614165763` — SUCCESS;
- Adaptive UI Tile Preview v0.2 APK Build — run `34614165859` — SUCCESS;
- Reconstructed Color Preview v0.1 — run `34614165852` — SUCCESS;
- Adaptive UI + Ingress v0.1 APK Build — run `34614165854` — SUCCESS;
- Canonical Integrity — run `34614165885` — SUCCESS;
- Documentation Governance (old workflow identity before this refresh) — run `34614165842` — SUCCESS;
- TruthRange v0.4/v0.5, Camera RGB Covariance and XYZ D50 integrity workflows — SUCCESS on the same head.

Source-bound preview artifact from current-head run `34614165763`:

- name: `truthraw-android-source-bound-color-preview-v0.1-debug-arm64`;
- artifact ID: `10270276895`;
- ZIP digest: `sha256:6f339319a2d9dec8a962b18c39d5bb78f5c565f7ea5be982b770be3362ec56fc`.

This is build/package evidence, not physical-device execution evidence.

## Major validated research anchors

| Module | Validated anchor / overlay | Run | Result |
|---|---|---:|---|
| Professional RAW Ingress v0.1 | `dadc6f95c82007da7060462cb10ced39921fc366` | `34591301790` | sealed/GCC/Clang/ASan PASS |
| Professional RAW Decoder Adapter v0.1 | implementation `39d699780cafa1fc93d03bdb3a9d77a0bf39d69c`; overlay `5bfff6aa5e042efe782fc629cca5605a11713531` | `34594703797`; `34594876182` | PASS |
| LibRaw Compatibility Probe v0.1 | implementation `1d6a165cad53bfb8257cfb764ee63c4c5c5819d9`; report head `4f2232278fcff4cb4e7d9dff99e03fcf8ec7efff` | `34595504942` | metadata-only contract/GCC/Clang/ASan PASS |
| Room ABI / Adaptive All-Room v0.2 | validated implementation `7d2e1a3829117cde559eef38dad07ca7a8ecc0e8`; PR head `497bbea144f2e0b30c203aa7c5b3f40c8f47a16e` | `34550111870`, `34550113743` | PASS |
| Android On-Device Validation v0.1 | PR head `596d671124ce3df949dfff91272c9b828fb4f2fa` | branch history recorded in PR #10 | host/APK research PASS; physical device pending |
| Adaptive UI + Ingress v0.1 | implementation `bed7c29b5d0773f0ccf499448e4f0c9cee9ea7ae`; PR head `66ee5b37206ca8226394a26138e1f34ba85b92ec` | `34582353230` | APK/contract PASS |
| Adaptive UI Tile Preview v0.2 | implementation `1aa38c25418ba5ed06b219e82ba48fa06323f90f`; PR head `ca5837e86dd75689227ff24db5c397272767b80f` | `34589023345` | ARM64 APK PASS |
| Gatehouse Runtime v0.1 | implementation `7943973aeddf2e0c08fec5dd61a12b53b4cd8f8e`; overlay `8d417d2c8bda7eb011b68c5acadac0dc01405cfe` | `34598421730`, `34598507775` | contract/GCC/Clang/ASan PASS |
| Decoded Measurement Handoff v0.1 | `b4c25c267867769313ce7c79be1fb14968a8b13f` | `34599504412` | SUCCESS; Gatehouse regression also green on head |
| Decoded Measurement Tile Source v0.1 | `0edca8edab8afed9aad83b133c8f578e8b7860e4` | `34599879528` | SUCCESS |
| Reconstructed Color Preview v0.1 | `372ffe1565e10070db74586298ba82042500af81` | `34602492858` | GCC/Clang/ASan/APK PASS |
| Scientific Preview Source Binding v0.1 | implementation `badf78ac578495425cf8ece0eef686944b3468b2` | `34605496139` | contract/GCC/Clang/ASan PASS |
| DNG Color Binding Producer v0.1 | implementation `6c0211790a48956f706298b2308118eae2d3f948`; overlay `988d498d37dc71629e25c3b5aed66faf7b5b2939` | `34608401519`, `34608551820` | PASS |
| Scientific Preview Source Binding v0.2 | implementation/current parent lineage through `74f561b10ff65db67dc0fa3db9b84ea6bdc1a079` | `34609562095` on validated v0.2 state | PASS |
| Android Source-Bound Color Preview v0.1 | implementation `65818f529d4588c2ed907fb11787e3dd83d4432c`; current state `2fdf05ca1bbbc59cd8867df0cae117d1eec92d51` | `34613390230`; `34614165763` | APK/contract PASS |

## Known unresolved/current-head CI gaps

- `research/decoded-measurement-main-house-e2e-v0.1` at `b45b32f1302df0355cf733bf0d42fc6eca90d561`: no workflow run associated with that current head was found during this audit. Treat as implemented/unvalidated.
- `research/libraw-fd-datastream-v0.1` at `f428394dcb6a6ee2ff2e683927a9d1dd632377c0`: implementation exists, but no standalone current-head workflow run was found during this audit. Do not infer full decoder validation.
- physical-device execution is not represented by host/APK assembly CI.

## Preserved failure history highlights

- LibRaw probe run `34595384703`: Clang/ASan build failed because distro LibRaw required OpenMP headers and `omp.h` was absent. Fixed by adding `libomp-dev`; probe semantics/sanitizers unchanged.
- DNG Color Binding Producer runs `34607859757` and `34608024699`: strict compile-warning failures; gates were not weakened.
- Reconstructed Color Preview initial run `34602246899`: unused test helper under `-Werror`; fixed by making it an active exposure-anchor check.
- Android Source-Bound Color Preview candidate `3ea45d9e65a3281b862378e9affefec643b4af19`, run `34612805148`: Android Clang `-Wmisleading-indentation` in byte-frozen v4.7i was promoted by target `-Werror`. Canonical bytes were not modified; one source-scoped non-fatal warning exception was used only for frozen `core.cpp`.

Never rewrite any of these failed runs as success.