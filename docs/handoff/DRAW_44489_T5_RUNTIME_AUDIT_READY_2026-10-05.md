# D.RAW 44489 — T5 Runtime Audit candidate — 2026-10-05

Continuation code: **44489**  
Project: **D.RAW** (`TruthRaw` remains repository history)  
Repository: `1DiDaDo1/TruthRaw`  
PR: **#130**  
Candidate branch: `fix/android-exact-gauge-pass-artifact-v03`

## Exact runtime candidate

The T5 read-only corridor-audit implementation was completed on runtime/source commit:

`6a557cda2b8f10db7f4c81dde50b5dc8c77ea210`

This commit is the exact source state used for the current device-test APK. Any later documentation-only commit must not be confused with the runtime candidate.

The implementation audits the already-existing corridor only:

`v0.4 Deep Scene Contribution -> v0.5 Deep Scene Binding -> v0.6 Light Transport -> existing Room Capsule -> v0.7 Appearance Resolve`

No second corridor or Room-Capsule architecture was created.

## T5 audit contract implemented

The runtime now records/validates, read-only:

- sealed source SHA-256;
- Scientific-Master SHA-256 and explicit source/Scientific-Master binding;
- v0.4 runtime count + lineage digest;
- v0.5 runtime count + lineage digest;
- v0.6 runtime count + lineage digest;
- Room-Capsule runtime count + lineage digest;
- v0.7 runtime count + lineage digest;
- physical-frame count and independent-evidence count;
- mutation/evidence/writeback firewalls;
- v0.5 geometry authority remains `IMAGE_PLANE_BOUND`;
- v0.6 geometry/material/illumination authority remains `INFERRED` for the current seed path;
- v0.7 `exposureApplicationCount == 1`;
- candidate/scientific writeback remains false.

The T5 serializer is diagnostic-only. Missing stage evidence is `UNKNOWN`; an explicitly rejected v0.6 build may be `BLOCKED`. Aggregate corridor failure is not falsely attributed to v0.6.

## Expected current no-world-evidence result

For the current single-observation route with no admitted Room geometry/material/illumination package:

- v0.6 expected state: `READY` when all parent/authority/firewall checks pass;
- Room Capsule expected state: `EXACT_PRESERVING_BYPASS`;
- v0.7 expected state: observed downstream Appearance only;
- `exposureApplicationCount`: exactly `1`;
- `creates_new_evidence=false`;
- `scientific_writeback_allowed=false`.

Any different runtime result must fail closed and be investigated before promotion.

## Files changed by the T5 implementation

The T5 delta from the frozen pre-round checkpoint `d2c26de69ea192ba23ade949aa2656ed6907e9ec` is deliberately narrow. It changes only the existing Appearance/runtime-diagnostic corridor:

- `docs/research/free-world-appearance-resolve-v0.7/native/free_world_appearance_resolve_v0_7.h`
- `docs/research/free-world-appearance-resolve-v0.7/native/free_world_appearance_resolve_v0_7.cpp`
- `suite_android/app/src/main/cpp/truthnegative_continuous_preview_bridge.cpp`
- `suite_android/app/src/main/java/com/truthraw/adaptiveui/TruthNegativeContinuousPreview.kt`
- `suite_android/app/src/main/java/com/truthraw/adaptiveui/T5CorridorAuditV01.kt`

No change was made in this T5 round to sealed CFA evidence, measured anchors, Scientific-Master implementation, Exact Gauge v0.3 core, canonical v0.2 core, or reconstruction behavior.

## Exact APK build proof

GitHub Actions run:

`37232652035`

Workflow:

`D.RAW Free World Research APK`

Exact workflow head:

`6a557cda2b8f10db7f4c81dde50b5dc8c77ea210`

Result: **SUCCESS**.

The following build steps completed successfully:

- checkout;
- Java/Android/NDK setup;
- stable D.RAW development signing materialization;
- `assembleDebug`;
- APK existence/signing verification;
- artifact upload.

Artifact:

`DRAW-free-world-research-debug-arm64-stable-signed`

Artifact ID:

`11314697580`

Artifact ZIP size:

`3,234,916 bytes`

Artifact ZIP SHA-256:

`6b9377cb5c2afbedb471781b826f5fe0798cbff7a2978f9b7526e16e36c4f78f`

Contained APK:

`app-debug.apk`

APK size:

`8,588,087 bytes`

APK SHA-256:

`31bba97ddb220284059eeb0c0402ba87b1b840935ccf2f39e848a865cd1b03aa`

The checksum stored inside the workflow artifact matches the independently verified APK digest above.

## Repository/CI qualification

Do **not** call the whole PR promoted or globally green from the APK workflow alone.

For exact runtime head `6a557cda...`, the head-bound Actions query returns the APK workflow above and that workflow is green.

PR #130 remains:

- open;
- draft;
- not merged;
- currently reported by GitHub as `mergeable=false` against base `fix/research-fresh-rerun-v01`.

The pre-existing recovery documents also record historical red governance/lifecycle gates. Those must be re-evaluated separately before any merge/promotion. Do not mix base-conflict resolution into the physical T5 evidence run.

## Next action: real-device T5 validation

Use **only** the APK bound to runtime commit `6a557cda...` and APK SHA-256 `31bba97d...b03aa` for the next physical test.

Follow the existing test handoff:

`docs/handoff/DRAW_44489_TEST_START_2026-10-04.md`

The immediate device round is:

1. install/update the exact APK while preserving package/signing continuity;
2. record whether the install is upgrade-in-place or clean install;
3. launch and verify the expected D.RAW build;
4. use one real DNG/RAW_SENSOR-derived observation through the current Foundation/Open-World route;
5. export/upload the machine-readable observation/diagnostic JSON produced by the app;
6. verify source SHA, Scientific-Master SHA, T5 stage lineages, authority, firewalls, Room-Capsule result and `exposureApplicationCount`;
7. preserve the resulting evidence bundle before any new runtime/scientific code changes.

### Expected T5 pass signature

- T5 telemetry schema = 1;
- source SHA non-zero;
- Scientific-Master SHA non-zero;
- source/Scientific-Master binding verified;
- v0.4 observed count = target pixels;
- v0.5 observed count = target pixels;
- v0.6 observed count = target pixels;
- Room Capsule applied count = 0;
- Room Capsule exact-bypass count = target pixels;
- v0.7 observed count = target pixels;
- v0.4/v0.5/v0.6/Room/v0.7 lineage digests non-zero;
- firewall bits = `0xff`;
- physical frame count = 1;
- independent evidence count = 1;
- v0.5 image-plane-bound = true;
- v0.6 inferred authority = true;
- candidate applied = false;
- exposure application count = 1.

### Stop conditions

Stop before any further implementation if:

- APK SHA/package/version/signer is not the intended candidate;
- source or Scientific-Master identity does not bind correctly;
- v0.4/v0.5/v0.6/Room/v0.7 counts or lineage are missing/inconsistent;
- Room Capsule applies a non-bypass effect without admitted world evidence;
- inferred geometry/material/illumination becomes `MEASURED`;
- source/Scientific Master/measured anchors are modified;
- `candidate_applied` or scientific writeback becomes true;
- exposure application count is anything other than 1;
- any missing evidence is silently converted from `UNKNOWN` to certainty.

## Permanent scientific boundary

This checkpoint proves buildability and a valid signed APK candidate. It does **not** yet prove physical runtime T5 success. Physical runtime authority remains unpromoted until the device evidence bundle is inspected.

**One Free World. Many sealed observations. One evidence law.**
