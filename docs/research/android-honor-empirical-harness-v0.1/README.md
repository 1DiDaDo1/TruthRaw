# Android Honor / MotionCam Empirical Harness v0.1

Status: **RESEARCH PASS — ARM64 EMPIRICAL HARNESS APK CI PASS; PHYSICAL HONOR EXECUTION UNPROVEN**

Date: 2026-09-11

Validated empirical implementation SHA: `c7eaef3f95eb906a78699196c958357731ae2787`

Validation workflow run: `34647603680`

Preserved validated Scientific Preview route SHA: `42b49ba16a6c5a0d2d6dbc407330acde3e161a46`

## Purpose

This research layer prepares the next empirical TruthRaw boundary: run the already-validated finalized Scientific Preview route on a physical Honor device with a real MotionCam/Honor DNG and record what actually happened without allowing runtime measurements to alter scientific processing.

The empirical harness is an **observer around** the validated route, not a replacement for it and not a new scientific authority.

The intended physical sequence is:

`real MotionCam/Honor DNG via Android document provider`
→ empirical pre-probe of exact source seal + DNG color metadata
→ unchanged finalized Scientific Preview route from `42b49ba...`
→ empirical post-probe of exact source seal + DNG color metadata
→ runtime/resource observations
→ optional empirical JSON export.

## Preserved scientific route

CI on exact empirical implementation SHA `c7eaef3f95eb906a78699196c958357731ae2787` verifies that these two critical Android scientific-route source files are byte-identical to the already-validated v0.2 implementation:

- `source_bound_color_preview_bridge.cpp` Git blob: `67a2ece0c6dc8a865821e580a669c7a688c74b2b`
- `NativeTilePreview.kt` Git blob: `8cc2fe259a3cca57188bae085e7c5f6599134f46`

The empirical Kotlin runner still calls the existing `TilePreviewLoader.load(resolver, job)` for the finalized result. The measurement layer does not substitute another renderer, matrix solver, TruthRange mapping, zero-line, evidence rule, or release gate.

The workflow explicitly reports:

- `scientific_route_bytes=UNCHANGED_FROM_42b49ba16a6c5a0d2d6dbc407330acde3e161a46`
- `empirical_probe=READ_ONLY_PRE_POST_SOURCE_AND_COLOR_AUDIT`
- `runtime_observations=PSS_LATENCY_CPU_THERMAL_UI_FRAME_PACING`
- `scientific_authority_from_empirical_layer=NONE`

## Read-only source/color probe

The new JNI entry point is:

`Java_com_truthraw_adaptiveui_NativeEmpiricalBridge_probeSourceColor`

Its job is measurement only. It:

1. receives the same document-provider file descriptor class used by the Android route;
2. computes an exact SHA-256 source seal through `seal_source_sha256`;
3. runs DNG Color Binding Producer v0.2 against the sealed source;
4. returns a bounded audit packet to Kotlin;
5. never passes those observations back into reconstruction, color, TruthRange, Backplane, release policy, or Scientific Master state.

The audit can record:

- exact source SHA-256 and byte length;
- producer/binding status;
- metadata bytes read and IFD entries visited;
- parser/hash workspace observations;
- exact v0.1 single-illuminant delegation versus v0.2 dual-illuminant use;
- third-calibration detection;
- ForwardMatrix state;
- CameraCalibration presence, signature match, and application state;
- CalibrationIlluminant1/2;
- resolved dual white temperature and interpolation weight when applicable;
- neutral-solve iteration count;
- source-metadata color authority code;
- physical-frame and independent-evidence counts.

A valid dual-illuminant interpolation endpoint weight of exactly `0.0` is retained as data rather than misinterpreted as a null sentinel. Seal presence is derived from probe status rather than testing whether the 256-bit digest bytes happen to be all zero.

## Pre/post stability check

The same source is probed before and after the unchanged finalized route.

For a successful empirical wrapper, the harness compares:

- source SHA-256;
- source byte length;
- stable color-profile audit fingerprint.

If the finalized preview itself was ready but the empirical wrapper observes source identity or color-profile audit mutation across the run, the empirical boundary fails closed and recycles that ready bitmap rather than presenting the run as stable empirical evidence.

This is an additional observational safety boundary. It does not weaken or replace the source re-verification already inside the finalized route.

## Runtime observations

For the existing `TilePreviewLoader.load` call, excluding the pre/post probes, the harness records:

- elapsed wall time;
- worker-thread CPU time;
- process PSS before, sampled peak, and after;
- Android thermal status at start, sampled peak, and end.

PSS/thermal sampling interval in v0.1 is 50 ms. Therefore the sampled peak is a bounded observation, not a claim of perfect instantaneous process-memory or temperature telemetry.

UI frame pacing uses `Choreographer` and records frame-interval p50, p95, and maximum across the visible empirical run, including the pre/post probes.

The report also hashes the installed APK (`applicationInfo.sourceDir`) at runtime so a returned empirical JSON can be tied to the actually installed package bytes.

## Empirical JSON

The app exposes `Empirical JSON opslaan` after an empirical attempt. The report schema is:

`TRUTHRAW_ANDROID_HONOR_EMPIRICAL_V0_1`

The report explicitly records:

- `measurement_only = true`;
- `changes_scientific_route = false`;
- preserved validated scientific-route SHA;
- device/model/Android information;
- installed APK SHA-256 when readable;
- pre/post source/color probes;
- source/profile stability result;
- finalized preview outcome/authority and bounded execution metrics when available;
- PSS/CPU/wall/thermal/frame-pacing observations;
- the claim boundary that source-metadata color is not independent physical calibration and this harness cannot authorize `FULL_PHYSICAL` color.

A failed finalized preview can still produce an empirical report. That is intentional: unsupported/malformed real-world Honor/MotionCam metadata is evidence about the current software boundary and must not be hidden by retuning.

## Exact-head CI evidence

Workflow run `34647603680` on exact SHA `c7eaef3f95eb906a78699196c958357731ae2787` completed successfully.

Host regression:

- DNG Color Binding Producer v0.2 revalidation: PASS;
- finalized Scientific Preview release-gate revalidation: PASS.

Android/build contract:

- preserved scientific-route Git blobs: PASS;
- empirical `measurement_only` / `changes_scientific_route=false` contract: PASS;
- no `android.permission.CAMERA` / no capture authority: PASS;
- Java 17 / Android SDK 35 / Build Tools 35.0.0 / CMake 3.22.1 / NDK 27.2.12479018: PASS;
- Kotlin compilation: PASS;
- Android arm64 CMake/NDK/JNI link: PASS;
- APK assembly: PASS (`BUILD SUCCESSFUL in 2m 31s`);
- empirical JNI symbol present in packaged arm64 native bridge: PASS;
- non-arm64 preview-bridge ABIs rejected by workflow check: PASS.

Existing non-fatal warnings remain scoped and were not used to modify frozen scientific bytes:

- the known `setDecorFitsSystemWindows` Java deprecation warning;
- the known frozen canonical v4.7i `-Wmisleading-indentation` warning.

Validated APK:

- size: `4,194,605` bytes;
- SHA-256: `bb68c2408d715241204fb15be712f37514e36665b0de5dc6580fb89e988fb4f9`.

Uploaded workflow artifact:

- name: `truthraw-honor-empirical-v0.1-debug-arm64`;
- artifact ID: `10282845816`;
- ZIP size: `1,331,377` bytes;
- ZIP SHA-256: `96b32ce416041ee3688f4201c2756f18f19b6792d3f9e07dd417c37eb2af9118`.

## Superseded intermediate evidence

Earlier empirical run `34647295844` was green, but is not the final proof head. It was superseded by the hardening at `c7eaef3f95eb906a78699196c958357731ae2787`, which corrected two audit-semantics edge cases:

1. an all-zero 256-bit digest is not used as an absence sentinel;
2. a valid dual-illuminant interpolation weight of exactly zero remains a real value.

The final exact-head run `34647603680` revalidated the full host and Android build after those changes.

## Authority boundary

The empirical layer has **no scientific authority**.

It cannot:

- turn source metadata into independent camera/lens calibration;
- promote `SOURCE_METADATA_BOUND` color to `FULL_PHYSICAL`;
- create photons or independent evidence;
- change `physicalFrameCount = 1` or `independentEvidenceCount = 1`;
- alter source ISO provenance or make ISO a scene axis;
- alter TruthRange or its zero-line;
- modify reconstruction thresholds to make a physical test pass;
- turn counterfactual or appearance state into measured evidence.

The canonical TruthRaw rule remains:

**Measured where measured. Reconstructed where necessary. Never invented.**

## What this proves

This proves that the hardened Honor empirical harness:

- compiles and links for arm64 Android;
- preserves the previously validated scientific Android route bytes;
- revalidates the relevant host color/release science;
- packages the read-only empirical JNI probe;
- can produce an APK whose identity is known;
- has code paths for source/profile audit and runtime measurements without granting them scientific authority.

## What this does not prove

This does **not** yet prove:

- that the APK installs or runs successfully on the physical Honor Magic 8 Pro;
- that a real MotionCam/Honor DNG reaches a ready finalized Scientific Preview;
- that the Honor document provider is seekable/compatible for the complete route;
- whether the tested real DNG is single-illuminant, supported dual-illuminant, unsupported custom/triple, or malformed for the current subset;
- actual physical-device RSS, latency, CPU behavior, UI frame pacing, or thermal state;
- physical optimality of vendor DNG matrices;
- independent per-lens camera color calibration;
- `FULL_PHYSICAL` color truth;
- arbitrary DNG support;
- multi-capture fusion/HDR science.

Until a report exported by the physical device is examined, **physical Honor execution remains UNPROVEN**.

## Physical test protocol

For the next empirical step:

1. install the validated APK identified above on the Honor target;
2. open a real, untouched MotionCam/Honor DNG through `RAW kiezen`;
3. allow the empirical wrapper to run the pre-probe, unchanged finalized route, and post-probe;
4. preserve either the ready result or the exact fail-closed state — do not change thresholds to force success;
5. choose `Empirical JSON opslaan`;
6. retain the original DNG together with the exported JSON;
7. verify that the JSON's installed APK SHA-256 matches the APK identity when Android exposes the installed package bytes as expected;
8. analyze the JSON before making any new claim about Honor compatibility, performance, thermals, DNG color form, or physical color quality.

The first useful physical corpus should include at least one real MotionCam Direct-CFA candidate from each lens/mode of interest. Telephoto remains especially important, but every file remains an independent evidence root unless a future explicit multi-capture contract says otherwise.

Independent camera/lens color calibration remains a separate scientific program. This empirical harness measures what the current source-bound pipeline actually does; it does not replace that calibration program.