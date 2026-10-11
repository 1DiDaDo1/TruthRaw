# D.RAW 44489 — JPEG route-mismatch fail-closed device protocol — 2026-10-06

Continuation code: **44489**

This protocol is the next physical acceptance step after the real-device PASS for preview-independent full-resolution JPEG and the +90° clockwise full-resolution orientation path.

It is a product/runtime freshness test only. It cannot promote scientific authority.

## Repository boundary at protocol creation

- repository: `1DiDaDo1/TruthRaw`
- active PR: **#131**
- branch: `feat/draw-workspace-free-raster-v01`
- resolved head before protocol creation: `0ba71c821b1834379dab851cd927c5e634211325`
- frozen scientific/audit reference: PR #130 / `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f`
- tested runtime implementation remains based on `de0d49ef9f9abda777a8126a2a98a028030eafa1`

## Read-only code audit before device test

The JPEG request freezes these downstream fields before Android's document picker opens:

- source job ID
- source URI
- output route: PURE / ADVANCED / PRO
- route/appearance flags
- user quarter-turn orientation

After the picker returns, `MainActivity` does **not** trust the old request blindly. It reads the current route, current route flags and current orientation and passes them into:

`DrawPhotoOutputCableV01.validateCurrentOutputContext(...)`

That validator first revalidates the source/job/URI and safety contract, then explicitly blocks when:

- `currentRoute != binding.route`
- `currentRouteFlags != binding.routeFlags`
- `currentQuarterTurns != binding.userQuarterTurns`

The route-mismatch message is:

`JPG-output geblokkeerd: uitvoerroute veranderde tijdens de bestandsdialoog.`

The appearance mismatch message is:

`JPG-output geblokkeerd: appearance-instellingen veranderden tijdens de bestandsdialoog.`

The orientation mismatch message is:

`JPG-output geblokkeerd: downstream oriëntatie veranderde tijdens de bestandsdialoog.`

This validation is explicitly independent of `TilePreviewUiState.Ready` and `UnifiedOutputPreviewResult`.

### Static audit result

**PASS — FAIL-CLOSED CHECK EXISTS IN THE ACTUAL JPEG RESULT PATH.**

Physical lifecycle acceptance is still required because static source inspection does not prove that a real Android document-picker round trip preserves/reaches the expected pending binding on the tested device.

## Preferred physical route-mismatch test

Use the already installed, previously tested APK. Do not install a modified runtime just to perform this test.

Starting condition:

- use the same known-good DNG if convenient;
- return orientation to `0°` unless intentionally testing another frozen field;
- set route to **PRO**;
- keep the source selected in `MainActivity`.

Steps:

1. In `MainActivity`, press **JPG · full resolution professional**.
2. Wait until Android's create-document/file picker is open. Do **not** confirm the save yet.
3. While that picker request is still pending, bring the D.RAW launcher to the foreground without completing/cancelling the picker, if the device/task UI permits it.
4. Change the preferred route from **PRO** to **PURE** (ADVANCED is also valid, but PURE is the clearest mismatch).
5. Return to the still-pending Android file picker.
6. Choose a new JPEG destination and confirm/save.
7. Return to `MainActivity`.

Required result:

- no full-resolution JPEG worker may start for the stale PRO request;
- the app must report the route-mismatch block message;
- the stale request must not silently become a PURE export;
- it must not fall back to the preview;
- no source mutation or Scientific Master writeback may occur.

A provider may create an empty/placeholder destination document before D.RAW receives the result. If that happens, a zero-byte/unused provider placeholder is not a successful JPEG export. The acceptance question is whether D.RAW refuses to render/commit stale output into it.

## Android task-stack limitation

Some Android vendor/task-stack combinations may always return the user to the existing DocumentsUI activity when the D.RAW icon is tapped, making it impossible to change D.RAW's route while the picker is still pending through normal UI navigation.

If the tested device cannot expose D.RAW's launcher while DocumentsUI is pending, classify that attempt as:

`TEST_NOT_REACHABLE_WITH_NORMAL_DEVICE_UI`

—not as a D.RAW failure and not as a PASS.

Do not weaken the product architecture or add a second renderer merely to make the test reachable.

If natural UI reachability fails, the next acceptable engineering step is a narrowly scoped diagnostic that exercises the exact same `validateCurrentOutputContext(...)` runtime contract without performing an export. Such a diagnostic must remain presentation/freshness-only and must not gain scientific authority.

## Evidence to return

Best evidence package:

- screenshot immediately before starting the PRO JPEG request;
- screenshot showing PURE selected while the original picker request remains pending, if reachable;
- screenshot of the final D.RAW block message;
- if a destination file appears, include it so its size/content can be checked;
- do not delete the source DNG for the test.

## Following test

After route mismatch is physically accepted, perform **source-switch/reprocess stale-state clearing**. The pending JPEG binding must be cleared or fail closed if the active sealed observation changes before the picker result is consumed.

## Permanent scientific boundary

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`

**Stable outside. Flexible inside. Evidence law unchanged.**

A stale presentation/output request must never be silently rebound to a different route, source, appearance state or orientation. Freshness validation protects provenance; it does not create new scientific evidence.