# TruthRaw next-chat handoff — 2026-09-19

**READ THIS FIRST IN THE NEXT CHAT.**

This file is the current operational handoff for the active TruthRaw integration line.

Active branch:

`integration/truthraw-suite-v0-68-restoration-transactional-fgs`

Current app version:

`0.33-v0.68-restoration-transactional-fgs`

This branch is an integration/research branch. It is **not** a canonical/main promotion.

## 1. Permanent project law

> **Measured where measured. Reconstructed where necessary. Never invented.**

> **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

Source evidence remains immutable. Measured, calibrated-estimate, reconstructed, censored, unknown, counterfactual and appearance-only states may not be silently relabelled into one another.

## 2. Scientific house — unchanged

The current scientific flow remains:

`SOURCE_EVIDENCE`
→ `MEASUREMENT_DE_ISP`
→ `SCIENTIFIC_MASTER`
→ `DYNAMIC_AUTHORITY_UNCERTAINTY_SUPPORT`
→ `OPEN_SCENE_STATE`
→ optional `COUNTERFACTUAL_OR_RESTORATION_STATE`
→ `APPEARANCE_HDR_TRANSPORT`
→ `FINITE_PROJECTION_EXPORT`.

The Scientific Master remains camera-native, scene-linear reconstructed RGB before normal camera→XYZ/appearance processing.

Current precision policy remains:

- source evidence: exact packed/integer;
- demonstrated-safe simple stages: F32 allowed by gate;
- branch-sensitive reconstruction: F64 reference;
- calibration/optimization/covariance: F64 reference;
- Scientific-Master storage: F32 only after the established storage gate.

## 2A. v0.61 — current active work: PURE file becomes self-binding

v0.60 restored the correct 32-bit IEEE Float XYZ-D50 LinearRaw pixel route. v0.61 leaves that pixel transform unchanged and closes the next provenance gap identified at the previous-chat endpoint.

The saved PURE DNG now carries a versioned private contract:

`TRUTHRAW_PURE_SELF_BINDING_V0_61`

It binds the file to:

- sealed source SHA-256;
- Scientific Master SHA-256;
- Zero-Line SHA-256;
- exact `L0` IEEE-754 binary64 bits plus gauge mode/id/flags;
- scene-scale SHA-256 and scene-scale state;
- exact canonical 180-byte Technical Backplane bytes plus CRC32;
- precision-policy identity;
- actual runtime reconstruction-backend identity;
- explicit no-master-mutation / no-appearance / no-counterfactual flags;
- one physical frame and one independent evidence item.

The writer now deserializes the supplied Backplane and fails closed unless source/master/zero-line/scene-scale identity, evidence counts and forbidden-mutation flags all match.

Important recovered history: the older 2026-09-14 PURE branch had a `TRCERT01` in-file certificate that already carried part of this backside provenance. Recovery governance classifies that schema as `LEGACY_VERIFY_ONLY`. v0.61 therefore does **not** reactivate it as the current certificate. Any current PTC/Dynamic-Authority certification must use a new versioned binding and may not change PURE pixels, the Scientific Master, Zero-Line or frozen Backplane layout.

Read:

`docs/TRUTHRAW_V061_PURE_SELF_BINDING_DNG_2026-09-19.md`

v0.61 CI run `35464897909` is green on host GCC, host Clang and Android. Artifact ID `10590768017`; extracted APK SHA-256 `38f118dbb068e846d0004da6d6c2091073a7fd73edaa31753e518a6723021137`.

## 2B. v0.62 — artifact-level post-write verification

v0.62 keeps the v0.61 writer/private contract and pixel mathematics unchanged. Android now re-opens the exact destination URI after commit and refuses to report success unless the saved DNG itself proves the current PURE role, v0.61 self-binding, exact L0 binary64 payload, scene-scale, 180-byte Technical Backplane, CRC32, precision/runtime provenance and 1/1 evidence state. The old compatibility-role marker causes fail-closed rejection. New test outputs use `*_truthraw_pure_float32_v0_62.dng`.

Read `docs/TRUTHRAW_V062_PURE_POSTWRITE_VERIFY_2026-09-19.md`.

## 2C. v0.63 — CRC correction + product UI

The Backplane CRC metadata bug found from the real v0.62 artifact is corrected first: `technical_backplane_crc32` is now CRC32 over bytes 0..175, matching the canonical internal CRC stored at bytes 176..179. The saved DNG is reopened and the verifier decodes the 180-byte Backplane, checks source/master/Zero-Line/scene-scale lineage and compares recomputed, embedded and declared CRC values. The private contract is versioned to `TRUTHRAW_PURE_SELF_BINDING_V0_63`.

After that correction, the launcher is rebuilt around two primary inputs — Open RAW / DNG and Use camera — while all historical probes move behind the settings gear. The generated TR lens/open-world icon is applied to the app and header. JPG and PURE are real selectable output preferences; JPG XL and Advanced remain visibly disabled until implemented.

Read `docs/TRUTHRAW_V063_CRC_UI_BRANDING_2026-09-19.md`.

v0.63 CI run `35469414682` is green on host GCC, host Clang and Android. Artifact ID `10592980623`; extracted APK SHA-256 `82dc3103b4e945e129fa5ea38fe717a342d459d1f6b71e4895bab9e00282d173`.

## 2D. v0.64 — TRUTHRAW ADVANCED derivative

v0.64 leaves the validated v0.63 PURE writer and pixel mathematics untouched. TRUTHRAW ADVANCED is now a real selectable downstream route with four bounded controls: Natural Light Balance, Natural HDR, Detail/Structure and Evidence-bound Restoration.

Light Balance is appearance-only and does not claim physical relighting. Natural HDR consumes the existing single-frame scene-aware gain state and cannot create measured dynamic range. Detail uses the existing support-limited appearance backend. Restoration targets only bounded preview locations whose underlying measured CFA sample is actually at/above source WhiteLevel; it requires at least three non-censored neighbours and leaves the underlying scientific authority CENSORED.

The current Advanced export is deliberately bounded to the existing preview/JPEG surface (384 px maximum edge). Full-resolution Advanced transport/export remains open. Full-resolution PURE Float32 DNG stays unchanged.

Read `docs/TRUTHRAW_V064_ADVANCED_DERIVATIVE_2026-09-19.md`.

v0.64 CI run `35471926706` is green on GCC, Clang and Android. Artifact ID `10593425008`; extracted APK SHA-256 `8d5b0e54517fe29c7e26eade989652aba5292fd498308d41d11dd6691d662441`.

## 2E. v0.65 — compact-phone UI + clean TR lens icon

v0.65 is presentation-only on top of v0.64. System-bar insets now belong to the scrolling viewport so content cannot scroll behind Android status/navigation bars. PURE and ADVANCED titles are intentionally constrained to clean two-line labels. The noisy/glitch-like v0.63 icon is replaced by a clean TR mark with a subtle camera-lens/open-world reflection in the upper R. No PURE or Advanced math/authority changes.

Read `docs/TRUTHRAW_V065_UI_ICON_POLISH_2026-09-19.md`.

v0.65 CI run `35473170260` is green on GCC, Clang and Android. Artifact ID `10593941511`; extracted APK SHA-256 `9557aaba0222ebed23be961b3865fe5004eaa0af5589de654439eacb766bf21d`.

## 2F. v0.66 — TruthNegative + Open-World + Dynamic Authority

v0.66 reconnects three recovered scientific lines into the current Android app rather than leaving them as separate research islands.

- TruthNegative TN-2 writes a source-resolution camera-native Float32 Scientific Negative bound to the exact Scientific Master and Technical Backplane.
- Open-World/Scene Physics becomes a runtime authority corridor for Advanced. Single-frame inferred illumination may constrain a derivative but is not promoted into a physical relight claim.
- Dynamic Authority remains fail-closed. Generic missing channels stay UNKNOWN until a source-bound uncertainty model is admitted. Censored support is not promoted to an exact recovered value.
- The dormant JPG XL output card is replaced by TRUTHNEGATIVE.

PURE stays on the real-device validated `TRUTHRAW_PURE_SELF_BINDING_V0_63` route.

v0.66 CI run `35497663010` is green on GCC, Clang, recovered scientific contract tests and Android. Artifact ID `10600757246`; extracted APK SHA-256 `746e43c5e675f556cb1d9eb40aef37773c79092b746987bd92c506dd467a60f5`.

## 2G. v0.67 — full-resolution retreatable Restoration

v0.67 makes Restoration a true source-resolution derivative instead of only a 384-pixel-edge Advanced preview effect.

New output:

`*_truthraw_fullres_restoration_v0_67.trr`

The file stores the full camera-native Float32 raster plus one restoration-role byte per source pixel. The roles are `PRESERVE_SCIENTIFIC_MASTER`, `AESTHETIC_REINTEGRATION_ONLY` and `UNRESOLVED_LOSS`.

Only a source site whose measured CFA sample is at/above admitted WhiteLevel is eligible for reintegration. Support excludes censored neighbours, requires at least three finite neighbours within radius 2, and never writes back into the Scientific Master. Before release the exporter recomputes the complete Scientific-Master digest; mismatch truncates/rejects the artifact. Android reopens the exact saved file and verifies the lineage/role contract and final size.

Read `docs/TRUTHRAW_V067_FULLRES_RESTORATION_2026-09-20.md`.

v0.67 CI run `35498407249` is green on GCC, Clang, OpenWorld/Dynamic-Authority/Restoration contracts and Android. Artifact ID `10601092611`; extracted APK SHA-256 `1ce0e75d409f3c2c8e5f6886e43cb39796dae58fc406d9aa3b78bc4406783c94`.

Real-device export validation of the new `.trr` path exposed an Android lifecycle/atomicity defect: one device artifact stopped after 487 complete tiles with an all-zero 8192-byte header and never reached finalization. That finding is preserved; it was not a scientific-restoration failure.

## 2H. v0.68 — transactional foreground Restoration

v0.68 keeps the v0.67 native restoration science unchanged and replaces the Android write lifecycle.

Long-running reconstruction now writes only to an app-private staging file owned by a `dataSync` foreground service with a bounded partial wake lock. After native completion, staging is contract-verified and whole-file SHA-256 hashed.

Only then is the user-selected SAF destination committed:

`zero 8192-byte header -> full body -> fsync -> VALID header written LAST -> fsync -> exact destination reopen -> whole-file SHA equality`.

A destination is therefore not a valid TruthRaw Restoration artifact until the complete body already exists.

Persistent transaction phases are:

`STAGING -> STAGING_VERIFIED -> COMMITTING -> VERIFYING -> SUCCESS/FAILED`.

If the process is later restarted with a non-terminal job but no live foreground service, v0.68 deletes private staging and deletes or truncates the incomplete destination. It deliberately fails closed rather than resuming an unknown native reconstruction state.

Read `docs/TRUTHRAW_V068_RESTORATION_TRANSACTIONAL_FGS_2026-09-20.md`.

v0.68 CI run `35501207534` is green on GCC, Clang, OpenWorld/Dynamic-Authority/Restoration contract tests and Android. Artifact ID `10602552000`; extracted APK SHA-256 `7d1812500a090f7ef1b8f6a13ba6f6ee1a9e0f31ff983cd45563494970ee7824`.

Real-device gate: repeat the full-resolution export and deliberately switch to another app while staging is running. Require a complete valid header, all tiles and staging↔destination SHA-256 equality.

v0.62 CI run `35466767939` is green on host GCC, host Clang and Android. Artifact ID `10591985003`; extracted APK SHA-256 `36e468a8a7e5449d6c649006a109f3a9163dbd5f3f746ed0c7cf521a4f88c447`.

## 3. Recovery work completed before app unification

The full genealogy recovery is preserved on:

`research/full-genealogy-pure-openworld-recovery-v0-1`

Key recovery files:

- `docs/TRUTHRAW_FULL_GENEALOGY_RECOVERY_AUDIT_2026-09-19.md`
- `state/FULL_GENEALOGY_PURE_OPENWORLD_RECOVERY_V0_1_2026-09-19.json`
- `docs/PURE_RECOVERY_TRANSPLANT_MATRIX_2026-09-19.md`

The recovery explicitly includes:

- historical TRUTHRAW PURE lineage;
- Open-World illumination;
- scientific HDR;
- water-droplet/material/detail stress tests;
- photo/painting-restorer conservation line;
- Camera-5 v0.14 source-first RAW_SENSOR + auxiliary DNG milestone;
- v0.19/v0.20 payload correction;
- complete HONOR v0.40→v0.54 investigation ladder;
- v0.53 Android-17 replay as current Camera-5 replay/control;
- v0.54 OEM Pro TELE DNG as independent 4080×3072 payload-domain corroboration.

The Restorer rule is explicit: valid measured CFA support is analogous to surviving original material and may not be overpainted merely because a model predicts a prettier value.

## 4. Camera-5 status that must not be simplified

Preserve both truths:

**v0.14** = successful source-first physical Camera-5 RAW_SENSOR acquisition, sealed original `.rawsensor`, exact timestamp binding and successful auxiliary DNG creation.

**v0.19/v0.20** = the tested 16320×12288 envelope was not a fully populated 200MP scene raster. Only the first 25,067,520 bytes were meaningful, exactly equal to 4080×3072×2.

**v0.53** = Android-17 replay of the same route, again successful, with the same 4080×3072 populated-prefix topology. This is the preferred current replay/control.

**v0.54** = HONOR Pro TELE DNG is also 4080×3072 uncompressed U16 CFA with 25,067,520 payload bytes.

Do not claim either “200MP capture completely failed” or “native full 200MP ADC RAW proven”.

## 5. Product direction now: one app, two front doors

The user explicitly chose:

1. **RAW file import is the primary choice**;
2. **Camera access is the secondary choice**, because after a valid RAW/DNG artifact exists it should enter the same pipeline.

Correct convergence:

`RAW file import`
or
`camera capture -> RAW/DNG artifact`
→ `SEALED_SOURCE_ADMISSION`
→ the same TruthRaw scientific path.

Important boundary:

A developed JPEG/HEIF may be used only as a developed-image-derived reconstruction source. It may not be converted into a fake “original RAW” and admitted as measured sensor evidence.

The camera path must hand an admissible RAW/DNG artifact into the same RAW ingress. TruthNegative is downstream of Scientific Master, not a mechanism for turning JPEG back into measured RAW.

See:

`docs/UNIFIED_FILE_CAMERA_INGRESS_CONVERGENCE_V056_2026-09-19.md`

## 6. v0.56 — unified multi-vendor RAW ingress

Branch:

`integration/truthraw-suite-v0-56-multivendor-raw-ingress`

v0.56 made the Android app file-first and vendor-neutral.

Primary UI:

- `1 · RAW-bestand openen · smartphone / professionele camera`
- `2 · Camera gebruiken · capture → dezelfde RAW-ingang`

DNG is the first native processing route.

Recognized proprietary RAW families include Canon CR3/CR2, Nikon NEF/NRW, Sony ARW, Fujifilm RAF, Panasonic RW2, OM/Olympus ORF, Pentax PEF, Leica RWL, Hasselblad 3FR/FFF, Phase One IIQ, Sigma X3F and others.

At v0.56 proprietary RAW was accepted only as an immutable handle and remained fail-closed as `DECODER_PENDING`.

The camera DNG now automatically hands into the common RAW ingress after successful source-first capture.

## 7. v0.57 — generic RAW source-adapter ABI

Branch:

`integration/truthraw-suite-v0-57-raw-source-adapter-abi`

v0.57 introduced:

`sealed source bytes`
→ versioned format adapter
→ common `IRawTileSource`
→ existing TruthRaw Measurement/de-ISP.

Core module:

`docs/research/multivendor-raw-source-adapter-v0.1/`

DNG now runs through the generic adapter registry instead of having a permanently special architectural path.

A synthetic non-DNG fixture proves that another container can populate the same `IRawTileSource` ABI without creating a second scientific house.

Permanent adapter rule:

A successful container decode does **not** prove untouched ADC provenance.

## 8. v0.58 — first real proprietary RAW decoder: Nikon NEF subset

Branch:

`integration/truthraw-suite-v0-58-nef-uncompressed-sample-adapter`

App version:

`0.23-v0.58-nef-uncompressed-sample-adapter`

Nikon NEF became the first real manufacturer-family decoder behind the generic ABI.

Current accepted NEF subset is intentionally strict:

- Nikon TIFF Make;
- Nikon Model required;
- little-endian classic TIFF;
- one unambiguous CFA IFD;
- Photometric CFA;
- SamplesPerPixel=1;
- Compression=1 only;
- BitsPerSample=16 only;
- validated strip bounds;
- explicit 2×2 Bayer CFA.

Compressed/packed NEF still fails closed.

A successful v0.58 decode reports:

- `exactCfaSamplesAvailable=true`;
- `measurementAdmissionReady=true`;
- `scientificAdmissionReady=false`;
- `directSensorAdcClaimAllowed=false`;
- `fullRawFrameMaterialized=false`.

The Android UI now has:

`Inspecteer NEF CFA-samples`

This produces only a **MEASUREMENT_ONLY grayscale visibility proxy** from exact decoded source codes. It is not black-subtracted, not demosaiced, not color-corrected, not a photographic preview, and not a Scientific Master.

The route seals the source SHA-256 before decode and re-verifies it after sample reading.

It can export:

`TruthRawNefMeasurementEvidence/0.58`

JSON containing sealed source identity and the bounded sample-domain claims.

Historical `universal_adapter.py` knowledge was recovered/aligned so the older universal evidence-adapter idea is not lost. Its key rule remains: storage representation and capture/processing lineage are separate; `.dng` or `.nef` alone does not prove direct-sensor truth.

### v0.58 usable debug APK

Successful Android run:

`35456055064`

Artifact:

`truthraw-suite-v0-58-nef-uncompressed-sample-adapter-debug-arm64`

Artifact ID:

`10587943240`

Artifact archive digest:

`sha256:779bb184bcf6cb71c759c0999e3f34bac2ea862c5cef56b8f65c6550d934d1e3`

This is a debug/research APK, not a final release-signed product.

## 9. v0.59 — current active work: Nikon radiometric admission

Active branch:

`integration/truthraw-suite-v0-60-pure-float32-dng`

App version:

`0.25-v0.60-pure-float32-dng`

v0.59 adds the next authority gate after exact NEF sample decode:

`exact sample decode`
→ exact camera/source scope
→ black-level authority
→ saturation/white-level authority
→ noise/uncertainty gate
→ source-bound color gate
→ held-out validation
→ Scientific Master eligibility.

New ABI object:

`RawRadiometricBinding`

The binding has an explicit authority class and must match exactly:

- format;
- camera Make;
- camera Model;
- RAW width;
- RAW height;
- CFA code;
- storage bits per sample;
- binding ID.

It carries four CFA-phase black values plus admitted white/saturation code.

A Nikon Z8 pack may not silently apply to a Nikon Z9 file. Geometry/CFA/bit-depth mismatches also fail closed.

Important scientific correction:

**16-bit storage ceiling 65535 is not automatically the physical sensor saturation level.**

Without a validated radiometric binding, the NEF remains measurement-only.

With a valid exact-scope radiometric binding:

- `radiometricBindingProvided=true`;
- `blackLevelAuthoritative=true`;
- `saturationLevelAuthoritative=true`;
- `measurementAdmissionReady=true`;
- **`scientificAdmissionReady=false` still remains**.

Scientific Master stays blocked because noise/uncertainty and source-bound color remain independent gates.

Current research docs/state:

- `docs/research/nikon-nef-radiometric-admission-v0.59/README.md`
- `state/TRUTHRAW_V059_NIKON_NEF_RADIOMETRIC_ADMISSION_2026-09-19.json`

### v0.59 CI

Host radiometric admission run:

`35456672651`

Result:

`SUCCESS`

Both GCC and Clang passed:

- generic adapter conformance;
- Nikon NEF strict decode;
- exact-scope black/saturation admission;
- wrong-camera scope fail-closed;
- radiometric success still scientific-blocked.

Android v0.59 run:

`35456737363`

Result:

`SUCCESS`

Artifact:

`truthraw-suite-v0-59-nef-radiometric-admission-debug-arm64`

Artifact ID:

`10588558376`

Artifact archive digest:

`sha256:4de4346d18fc9c3477ed5740d08f400c3e96500b89d9626a9a553e1cad0793b8`

## 10. v0.60 — current active work: TRUTHRAW PURE Float32 restored

Active branch:

`integration/truthraw-suite-v0-60-pure-float32-dng`

App version:

`0.25-v0.60-pure-float32-dng`

The current multi-vendor app had a 16-bit `linear-dng-projection-v0.1` compatibility export. That route clips values below 0 and above 1 at the export boundary and is **not** TRUTHRAW PURE.

The recovered historical PURE writer has now been reconnected to the current Android app.

Current PURE route:

`sealed source DNG`
→ source-bound color
→ generic DNG adapter
→ Scientific Master streaming
→ Technical Backplane Phase 2
→ canonical Scientific Master replay
→ exact master-digest gate
→ cameraToXyzD50
→ **32-bit IEEE Float XYZ-D50 LinearRaw DNG**.

Serialized contract:

- 3 samples/pixel;
- `BitsPerSample = 32,32,32`;
- IEEE float SampleFormat;
- `PhotometricInterpretation = LinearRaw`;
- `DNGVersion = 1.4.0.0`;
- `DNGBackwardVersion = 1.4.0.0`;
- `Compression = 1` (uncompressed, non-lossy storage);
- `Software = TruthRaw scientific-master-linear-dng-projection-v0.1`;
- 64×64 canonical tiling;
- negative and >1 components preserved;
- no appearance;
- no counterfactual;
- frame/evidence remains 1/1.

The app now labels the two exports separately:

- **TRUTHRAW PURE · 32-bit Float DNG** — primary scientific DNG;
- **16-bit Linear DNG (compatibility)** — bounded compatibility projection only.

The uint16 writer may not redefine PURE.

v0.60 is currently enabled only for the fully admitted DNG route. Nikon NEF remains measurement/radiometric-gated and must not enter PURE until its remaining scientific admission gates are closed.

CI run `35458367764`:

- host GCC: SUCCESS;
- host Clang: SUCCESS;
- Android arm64: SUCCESS;
- APK artifact ID `10589820246`;
- artifact `truthraw-suite-v0-60-pure-float32-dng-debug-arm64`;
- artifact archive SHA-256 `fbf4973bc49714dadf821e4421141bd11a8e60022cd495e5ec3821b39547e443`.

Current real-device gate: install the v0.60 APK, export a known-good DNG through TRUTHRAW PURE, then inspect the saved artifact and confirm 32-bit IEEE Float / DNG 1.4 / LinearRaw / historical Software string / preserved signed-overrange components.

See:

`docs/TRUTHRAW_V060_PURE_FLOAT32_DNG_2026-09-19.md`

## 10. Important current limitation

There is **not yet a real Nikon camera calibration pack** admitted.

The radiometric pack used in host tests is a synthetic scope-validation fixture. It proves that the gate works; it does not prove Nikon Z8/Z9/etc. black/saturation science.

Do not claim real Nikon Scientific Master support yet.

A real Nikon promotion needs actual NEF files plus independently validated camera/mode evidence.

## 11. Immediate next safe work

Continue in this order:

1. run v0.63 host GCC/Clang and Android CI and verify the produced APK;
2. install v0.63 and confirm the new icon, RAW/camera launcher and research settings screen;
3. export a new v0.63 PURE DNG and require `self-binding verified=true`;
4. independently recompute CRC32 over Backplane bytes 0..175 and confirm it matches both the internal bytes 176..179 and the DNG metadata field;
3. keep historical `TRCERT01` verification support separate from any new current PTC/Dynamic-Authority certificate schema;
4. define a versioned Nikon `NoiseUncertaintyBinding` or equivalent, scoped to exact camera/mode/readout identity;
5. keep noise evidence separate from black/saturation evidence;
6. define source-bound Nikon color admission without treating ordinary Make/Model/white balance metadata as physical calibration;
7. require held-out validation before `scientificAdmissionReady=true`;
8. only then let an admitted NEF enter the existing Measurement/de-ISP → Scientific Master path.

If a real Nikon NEF is uploaded, first determine whether it belongs to the strict v0.58 uncompressed 16-bit subset. Do not silently broaden the parser when it does not.

## 12. Files the next chat should read immediately

Read in this order:

1. `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-19.md` — this file;
2. `state/CURRENT_PROJECT_STATE_2026-09-19.json`;
3. `docs/TRUTHRAW_FULL_GENEALOGY_RECOVERY_AUDIT_2026-09-19.md`;
4. `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md`;
5. `docs/UNIFIED_FILE_CAMERA_INGRESS_CONVERGENCE_V056_2026-09-19.md`;
6. `docs/research/multivendor-raw-source-adapter-v0.1/README.md`;
7. `docs/research/nikon-nef-radiometric-admission-v0.59/README.md`;
8. `state/TRUTHRAW_V058_NIKON_NEF_SAMPLE_ADAPTER_2026-09-19.json`;
9. `state/TRUTHRAW_V059_NIKON_NEF_RADIOMETRIC_ADMISSION_2026-09-19.json`;
10. only then older dated architecture/camera handoffs as needed.

## 13. Do not regress these decisions

- File upload remains the primary product entrance; camera capture is secondary.
- Both converge only at sealed source admission.
- A developed image is never relabelled as original measured RAW.
- DNG is not inherently stronger evidence merely because it is DNG.
- Proprietary RAW acceptance is not the same as scientific admission.
- Exact sample decode is not the same as calibrated radiometry.
- Calibrated radiometry is not the same as a complete noise/color camera model.
- Scientific Master is never created merely to make the UI feel complete.
- Counterfactual/restoration/appearance never write back into captured-world evidence.
- The full historical recovery, PURE path, Open World, HDR, Restorer and water/detail line remain part of the project.
