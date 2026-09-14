# TruthRaw next-chat handoff — 2026-09-14

Status: **ACTIVE SESSION HANDOFF / IMPLEMENTATION OVERLAY**

This file is the shortest path for the next chat to continue the current implementation without reconstructing the session from conversation history.

It does **not** replace the project-wide canonical state or the sealed scientific architecture. Read it together with `START_HERE_NEW_CHAT.md`, the 2026-09-13 project map/fact-check/house documents, and the module-local tests/manifests named below.

## 1. Repository / PR snapshot

Repository: `1DiDaDo1/TruthRaw`

Active implementation branch:

`research/ui-output-modes-certificate-v0.1-2026-09-14`

Draft PR:

`#23 — Integrate four-mode output policy + TRUTHRAW PURE float32 DNG`

PR base:

`research/project-factcheck-optimization-v0.1-2026-09-13`

Validated implementation head before this handoff-document commit:

`ef5175707d2b69973601408d6053fd234b2c6906`

At that implementation head, the current PR-triggered workflow set observed for the commit was fully green. The successful workflows included:

- `TruthRaw Certificate v0.1`
- `TruthRaw DNG Certificate Embed v0.1`
- `TruthRaw Product UI Contract v0.1`
- `Output Mode Policy v0.1`
- `Scientific Master Linear DNG Projection v0.1`
- `Adaptive UI + Ingress v0.1 APK Build`
- `Adaptive UI Tile Preview v0.2 APK Build`
- `Android Source-Bound Color Preview v0.1 APK Build`
- `Android Finalized Scientific Preview v0.1 APK Build`
- `Android Finalized Dual-Illuminant Color v0.2 APK Build`
- `Android Honor Empirical Harness v0.1`
- `Reconstructed Color Preview v0.1`
- `Canonical Integrity`
- `Documentation Governance Current`
- `TruthRange v0.4 Integrity`
- `TruthRange v0.5 Integrity`
- `Camera RGB Covariance v0.6 Integrity`
- `XYZ D50 Uncertainty v0.7 Integrity`

Important: the documentation commits that follow this snapshot move the branch head without changing the implementation code. The next chat must re-fetch the live PR head and workflow state before making a fresh “all green” claim.

Earlier red runs on intermediate commits are preserved as useful negative evidence; they are not the current implementation status. In particular, an Android build failure around `TileRect` namespace compatibility was fixed without changing the sealed scientific meaning.

## 2. Non-negotiable scientific laws

Keep these exact project laws:

**Measured where measured. Reconstructed where necessary. Never invented.**

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

The original Direct-CFA/source RAW remains immutable sealed evidence. Normal scientific processing remains single-frame with:

- `physicalFrameCount = 1`
- `independentEvidenceCount = 1`

Measured, reconstructed, censored/unknown, counterfactual, appearance and projection state remain different authority classes.

The current project-level Scientific Master is reconstructed **camera-native RGB before the normal `camera_to_xyz()` route and before appearance**. It is not original sensor evidence and it is not a display image.

TruthRange zero-line remains a gauge/reference, not sensor black, absolute darkness, DNG BlackLevel, clipping or display middle grey. Virtual EV/ISO/gain does not create new photons or evidence.

## 3. House / history context that must not be lost

The current architecture is the convergence of four recovered design concepts:

- **gezegelde woning / sealed house** — preserve the original RAW as immutable evidence;
- **alle vrijheid / new house** — allow a richer reconstructed scene representation without claiming stronger evidence;
- **achterkant van de foto / Technical Backplane** — bind source/master/zero-line/scene-scale/provenance behind the visible projection;
- **tussenwoning / Gatehouse** — isolate external decode complexity and detach it after a sealed handoff before Main-House processing.

Read `docs/CORE_VISION_HOUSE_GENEALOGY_BACKPLANE_GATEHOUSE_2026-09-13.md` before changing house/runtime boundaries.

The Technical Backplane is a compact technical backside, not hidden image evidence and not a second Scientific Master. The Gatehouse is not a second truth source.

## 4. Four-mode product contract

The public launcher now has four primary modes:

1. `JPG` — presentation output.
2. `JPG XL` — high-quality presentation slot; currently fail-closed until an actual JPEG XL encoder is independently validated.
3. `TRUTHRAW PURE` — scientific output; no creative appearance toggles.
4. `TRUTHRAW ADVANCED` — same scientific foundation, with downstream appearance/export controls.

The option chips are:

- `Colourful`
- `Detailed`
- `Soft`
- `HDR`

They are selectable only for `JPG`, `JPG XL`, and `TRUTHRAW ADVANCED`.

`TRUTHRAW PURE` must remain appearance-neutral. Missing/unknown mode handoff fails safe to PURE. Unsupported appearance options sanitize to neutral.

Current profile selections are **appearance intent only**. They must not yet be described as completed rendering algorithms. They do not alter the source evidence, reconstruction authority, Scientific Master identity, TruthRange zero-line, evidence counts or Technical Backplane.

Primary implementation files:

- `app/android/truthraw-adaptive-ui-v01/app/src/main/java/com/truthraw/adaptiveui/OutputModeActivity.kt`
- `app/android/truthraw-adaptive-ui-v01/app/src/main/java/com/truthraw/adaptiveui/OutputModePolicy.kt`
- `app/android/truthraw-adaptive-ui-v01/app/src/main/java/com/truthraw/adaptiveui/MainActivity.kt`
- `tools/verify_output_mode_policy.py`
- `app/android/truthraw-adaptive-ui-v01/tools/verify_product_ui_contract_v0_1.py`
- `.github/workflows/output-mode-policy-v0-1.yml`
- `.github/workflows/truthraw-product-ui-contract-v0-1.yml`

## 5. Language / product identity contract

English is the canonical fallback language.

Current Android resource localizations:

- English
- Dutch / Nederlands
- German / Deutsch
- French / Français

The product names `TRUTHRAW PURE`, `TRUTHRAW ADVANCED` and the option identifiers `Colourful`, `Detailed`, `Soft`, `HDR` remain stable across languages.

Per-app locale configuration is present for `en`, `nl`, `de`, `fr`.

## 6. APK icon state

The launcher now uses an adaptive-vector TruthRaw icon following the approved dark `TR` / blue-glow direction.

Relevant files include:

- `app/android/truthraw-adaptive-ui-v01/app/src/main/res/drawable/truthraw_launcher_art.xml`
- `app/android/truthraw-adaptive-ui-v01/app/src/main/res/drawable/truthraw_launcher_foreground.xml`
- `app/android/truthraw-adaptive-ui-v01/app/src/main/res/mipmap-anydpi-v26/ic_launcher.xml`
- `app/android/truthraw-adaptive-ui-v01/app/src/main/res/mipmap-anydpi-v26/ic_launcher_round.xml`
- `app/android/truthraw-adaptive-ui-v01/docs/TRUTHRAW_ICON_SOURCE.md`
- `docs/implementation/ICON_BINARY_PENDING_2026-09-14.md`

The exact previously generated raster artwork is not silently treated as a repository binary unless its exact approved source is intentionally supplied and committed. The current vector is a repository-safe implementation of the approved direction.

## 7. TRUTHRAW PURE scientific DNG path

The high-fidelity PURE RAW/DNG path is now the restored float32 Scientific Master Linear DNG writer, not the older 16-bit compatibility window.

Current scientific export sequence:

`sealed source -> source-bound color -> Scientific Master v0.2 -> Technical Backplane phase 2 -> canonical 64x64 Scientific Master replay -> exact master digest gate -> cameraToXyzD50 -> float32 XYZ-D50 LinearRaw DNG`

The float32 writer:

- stores 32-bit IEEE float samples;
- uses `PhotometricInterpretation = LinearRaw`;
- writes three samples per pixel;
- retains negative and `>1` values;
- does not apply tone mapping, appearance or gamut mapping;
- validates Scientific Master identity before artifact commit;
- preserves one physical frame / one independent evidence root.

The old 16-bit projections remain compatibility outputs. They may clip to the finite compatibility interval and therefore must never redefine TRUTHRAW PURE.

Relevant module:

`docs/research/scientific-master-linear-dng-projection-v0.1/`

Android bridge:

`app/android/truthraw-adaptive-ui-v01/app/src/main/cpp/raw_projection_export_bridge.cpp`

## 8. TruthRaw Certificate v0.1

The certificate belongs **inside the file / technical backside**, never as a visible watermark.

Current Certificate v0.1 binds the important provenance/authority identities, including source evidence, Scientific Master, zero-line/scene-scale/backplane identity, projection class, color binding and the single-frame evidence counts.

Relevant modules:

- `docs/research/truthraw-certificate-v0.1/`
- `docs/research/truthraw-dng-certificate-embed-v0.1/`

The DNG embed route is implemented and validated at host level.

Current security rule:

**No private TruthRaw brand-signing key may be embedded in the APK or repository.**

Until a trusted issuer key and verifier trust distribution are provisioned, the file/UI must report:

`UNSIGNED DEVELOPMENT`

and must **not** display a verified certificate badge.

The certificate may authenticate a reconstructed/projection artifact, but it may not relabel reconstructed data as measured Direct-CFA sensor evidence.

Brand/trademark protection and cryptographic provenance are related but distinct. Metadata/signature does not itself create trademark rights.

## 9. Real physical DNG test set

A five-file Honor Magic 8 Pro / MotionCam physical test-set manifest is now in:

`docs/research/physical-dng-test-set-v0.1/TRUTHRAW_PHYSICAL_TESTSET_2026-09-14.json`

Files:

- `IMG_260816_134122_304_005.dng`
- `IMG_260816_134204_911_008.dng`
- `IMG_260816_143716_085_041.dng`
- `IMG_260830_143012_297_014.dng`
- `IMG_260908_193845_662_027.dng`

Shared observations recorded in the manifest include HONOR `BKQ-N49`, MotionCam Pro, 4080×3072, BGGR, source `WhiteLevel=1023`, DNG 1.4.0.0, and one physical frame / one evidence root.

The GainMap OpcodeList2 payload is recorded as byte-identical across all five files, and the manifest records per-file SHA-256, ISO, exposure, black levels, AsShotNeutral, NoiseProfile, clipping/below-black counts and selected normalized statistics.

These files are **test evidence**, not automatically calibration evidence. Do not upgrade them to independent physical calibration without an acquisition protocol that supports that claim.

The `IMG_260830_143012_297_014.dng` reference remains the previously exercised source-bound physical reference with SHA-256:

`fac842110cfd5b527aae2fa46237824d444aa5701017c2b68021962ebce47da3`

## 10. Current app identity

Android prototype version is currently advanced to:

`0.7-four-mode-certificate`

The branch is still research/implementation state and the PR remains draft. Do not describe it as a production release.

## 11. What is already proven vs still open

Already implemented/host-validated on this branch:

- four-mode launcher contract;
- profile-option visibility policy;
- fail-safe PURE policy sanitization;
- English fallback + NL/DE/FR resources;
- adaptive launcher-icon resource integration;
- float32 Scientific Master Linear DNG writer + Android export route;
- preservation of negative and `>1` float components in the scientific DNG writer;
- source re-verification and exact Scientific Master digest gate around export;
- TruthRaw Certificate v0.1 canonical record;
- DNG certificate embedding;
- explicit unsigned-development certificate state;
- product/policy/certificate/float32 module CI;
- all current PR workflows observed at validated implementation head `ef517570...` green.

Still intentionally open / blocked from production claim:

- actual JPEG XL encoder validation;
- final renderer semantics for `Colourful`, `Detailed`, `Soft`, `HDR`;
- trusted cryptographic issuer signing key and verifier trust distribution;
- real-device Honor/MotionCam end-to-end float32 DNG export validation on the five-file set;
- verification that the embedded certificate survives the complete Android save/export route exactly as intended;
- Adobe Camera Raw / Lightroom interoperability proof for the float32 LinearRaw DNG;
- optional exact binary launcher artwork if the approved source asset is explicitly provided for repository inclusion.

## 12. Recommended next execution order

The next chat should continue in this order unless a new user priority overrides it:

1. Re-fetch PR `#23`, branch head and current workflows; do not assume this handoff's head is still live.
2. Keep current green scientific/export modules frozen while starting real-device validation.
3. Exercise the five physical DNGs through the same finalized source -> Scientific Master -> PURE float32 DNG route.
4. For every produced DNG verify dimensions, float32 tags, negative/>1 counts, source/master digest binding, Backplane/certificate identity, evidence counts `1/1`, and that appearance/counterfactual flags remain false in PURE.
5. Verify `DNGPrivateData`/certificate extraction from the actual Android-produced file and verify that modified covered metadata invalidates verification once a trusted signature path exists.
6. Test actual ingestion in Adobe Camera Raw / Lightroom separately from scientific correctness. A structurally valid DNG does not prove interoperability.
7. Implement `Colourful`, `Detailed`, `Soft`, `HDR` only as downstream appearance operators with regression tests proving the Scientific Master/backplane hashes do not change.
8. Add JPEG XL only after a real encoder and round-trip/metadata tests exist; until then keep the button fail-closed.
9. Design trusted certificate signing with an external/protected issuer key. Never add a private signing key to source control or the APK.
10. Only after these gates are green consider promoting PR `#23` out of draft or merging it into a broader current branch.

## 13. Files the next chat should read first

Read, in order:

1. `START_HERE_NEW_CHAT.md`
2. `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-14.md` (this file)
3. `docs/implementation/OUTPUT_MODES_CERTIFICATE_IMPLEMENTATION_2026-09-14.md`
4. `docs/research/physical-dng-test-set-v0.1/TRUTHRAW_PHYSICAL_TESTSET_2026-09-14.json`
5. `docs/CORE_VISION_HOUSE_GENEALOGY_BACKPLANE_GATEHOUSE_2026-09-13.md`
6. `state/CURRENT_CANONICAL_STATE_2026-09-13.json`
7. module-local README/manifests/tests for the specific feature being changed

## 14. Do not regress these boundaries

Do not:

- edit sealed/canonical files just to make a new workflow green;
- change expected hashes to hide an integrity failure;
- call a reconstructed CFA output original measured sensor RAW;
- let JPG/JPG XL/ADVANCED appearance options modify PURE's Scientific Master;
- let a certificate strengthen evidence authority;
- let the Technical Backplane become a second image/master;
- let the Gatehouse become a second scientific source;
- call JPEG XL production-ready before an encoder is validated;
- call `UNSIGNED DEVELOPMENT` cryptographically verified;
- use GCam/APK/computational-RAW content as TruthRaw evidence/calibration authority;
- infer physical sensor dynamic range from TruthRange's mathematical address space.

## 15. Continuity sentence

**The current work has moved TruthRaw from a scientific architecture with experimental export pieces to a four-mode Android product contract in which TRUTHRAW PURE is backed by the restored float32 Scientific Master DNG path, TRUTHRAW ADVANCED/JPG/JPG XL are explicitly downstream appearance/export surfaces, the TruthRaw Certificate lives in the file's technical backside, and the remaining work is real-device/interoperability/profile-rendering/trusted-signing validation rather than redefining the evidence model.**
