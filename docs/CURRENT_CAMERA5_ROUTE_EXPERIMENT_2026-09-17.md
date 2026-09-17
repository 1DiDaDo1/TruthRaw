# CURRENT Camera-5 route-control experiment — 2026-09-17

Status: **CURRENT EXPERIMENT TRACK; v0.20 REMAINS SOURCE/PAYLOAD AUTHORITY; v0.30 FULL-FACTORIAL MATRIX HAS 8/16 UNIQUE PROFILES WITH INDIVIDUALLY READABLE DEVICE EVIDENCE**

This document tracks upstream HONOR/QTI route-control experiments after the completed v0.20 payload-topology result. It does not replace `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md` for source authority.

Permanent boundary:

`APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF`

## Control authority

TruthRaw v0.20 remains the source/payload control:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

Measured control topology:

- app-visible envelope: `16320x12288`, `401,080,320` bytes;
- populated source prefix: `25,067,520` bytes;
- first `768` declared rows populated, remaining `11,520` rows zero;
- unique advertised standard RAW U16 byte match: `4080x3072`;
- exact prefix copied without transform as payload candidate.

## Completed representation-oracle sequence

Resolved native representations:

- v0.23 `EnableIdealRAW`: tag `0x801F0027`, native `BYTE`;
- v0.25 `RawCbSourceType`: tag `0x801F0009`, native `INT32`;
- v0.27 `EnableXCFAOptimization`: tag `0x801F0036`, native `BYTE`;
- v0.29 `HALOutputBufferCombined`: tag `0x801F0034`, native `INT32`.

Each oracle used disposable request metadata only and did not promote vendor semantics.

## Completed single-factor interventions before the matrix

- v0.24 `EnableIdealRAW=BYTE(1)`;
- v0.26 `RawCbSourceType=INT32(1)`;
- v0.28 `EnableXCFAOptimization=BYTE(1)`.

All three were accepted and attached on the tested route, yet none produced a measurable differential in RAW envelope or populated-payload topology versus v0.20. Numeric value `1` remains an experimental representation-level value, not a proven semantic value.

## v0.30 design

Experiment:

`CAMERA5_VENDOR_ROUTE_FULL_FACTORIAL_2_LEVEL_4_FACTOR`

Factors (`ABCD`):

- A = `EnableIdealRAW`, native `BYTE`, tag `0x801F0027`;
- B = `RawCbSourceType`, native `INT32`, tag `0x801F0009`;
- C = `EnableXCFAOptimization`, native `BYTE`, tag `0x801F0036`;
- D = `HALOutputBufferCombined`, native `INT32`, tag `0x801F0034`.

Levels:

- `0` = `UNSET_NO_WRITE`;
- `1` = numeric `1` written with the device-resolved native representation.

All `2^4 = 16` combinations occur once in the designed matrix. The run order is:

`0000 -> 1111 -> 0101 -> 1010 -> 0011 -> 1100 -> 0110 -> 1001 -> 0001 -> 1110 -> 0010 -> 1101 -> 0100 -> 1011 -> 1000 -> 0111`.

The matrix preserves the v0.20 source-first capture/audit order. A matrix hit would establish a combination-level route/topology differential first; it would not immediately establish vendor semantics for a key.

## v0.30e recovery UI

Because the original v0.30 UI and the v0.30c/v0.30d scroll approaches were not reliably usable on the device, v0.30e fixes the essential controls above the detail pane and adds first-missing recovery logic.

Build:

- branch `integration/truthraw-suite-v0-30e-fixed-controls-matrix-recovery`;
- head `a99861d3d4468849aeedbd8070805ffbdbd3d294`;
- run `35266284068`;
- job `105354172823`;
- APK SHA-256 `942b21518860af983be5383298b915d30f118684a83c19783b098decb313d112`;
- artifact ZIP SHA-256 `b2d1f9b306a52699b5dbbbb8eaad26a46a6a85f14a7a20ea66f880401b8ad55e`.

The scientific matrix, acquisition ordering, source-seal ordering, Stage 3.6 and Stage 3.7 were not changed by the UI recovery build.

## Hard device evidence currently available

Only individually readable v0.30 evidence JSON files count toward matrix completion here.

Hard-evidenced unique profiles:

- R01 `0000` — 0 keys written — matrix all-UNSET control;
- R02 `1111` — 4 keys written;
- R04 `1010` — 2 keys written;
- R11 `0010` — 1 key written;
- R13 `0100` — 1 key written;
- R14 `1011` — 3 keys written;
- R15 `1000` — 1 key written;
- R16 `0111` — 3 keys written.

A second independent R16 `0111` capture is also available as a structural replicate.

Therefore the current hard completion state is:

`8 / 16 UNIQUE PROFILES`

Missing individually readable profiles:

`R03, R05, R06, R07, R08, R09, R10, R12`

Equivalent ABCD values:

`0101, 0011, 1100, 0110, 1001, 0001, 1110, 1101`.

## Common result across all eight hard-evidenced profiles

Every currently hard-evidenced profile remains in the same measured topology class as v0.20:

- physical Camera `5`;
- app-visible source envelope `16320x12288`;
- source bytes `401,080,320`;
- row stride `32,640`;
- pixel stride `2`;
- only the first `768` declared rows populated;
- `11,520` rows all zero;
- populated payload byte count `25,067,520`;
- unique advertised standard RAW byte match `4080x3072`;
- no measured envelope/populated-prefix topology differential versus v0.20.

Small numbers of zero-valued samples can occur inside the populated prefix and are not treated as route changes.

## Current bounded negative results

R01 `0000` shows that the matrix framework itself reproduces the v0.20 topology when all four factors are UNSET.

R02 `1111` shows that simultaneously writing numeric `1` to all four currently type-resolved factors is **not sufficient** to change the measured app-visible RAW envelope/populated-prefix topology.

The current hard set additionally shows no topology differential for:

- R11 `0010` — C high only;
- R13 `0100` — B high only;
- R15 `1000` — A high only;
- R04 `1010`;
- R14 `1011`;
- R16 `0111`, twice independently.

D-only `0001` remains missing as hard evidence.

These are bounded negative differentials. They do not prove the controls are globally ineffective, do not establish the semantics of numeric value `1`, and do not replace the need to test the remaining matrix profiles.

## Bundle handling

Several `TRUTHRAW_CAM5_V030_MATRIX_EVIDENCE_BUNDLE_*.zip` files have been uploaded. They are retained as auxiliary provenance containers. Archive inspection repeatedly timed out in the current analysis environment, so no run is promoted from bundle filename, bundle size, UI progression or recollection alone.

The hard matrix completion set is defined by individually readable evidence JSONs until bundle contents can be independently parsed.

## Detailed partial-result record

See:

`docs/HONOR_CAMERA5_V030_VENDOR_ROUTE_MATRIX_PARTIAL_DEVICE_RESULTS_2026-09-17.md`

Machine-readable state:

`state/CAMERA5_V030_PARTIAL_MATRIX_V030E_FIXED_CONTROLS_STATE_2026-09-17.json`

## Immediate continuation

Continue only the missing hard-evidence profiles under v0.30e first-missing recovery:

`R03, R05, R06, R07, R08, R09, R10, R12`

Save every evidence JSON individually. Bundle export remains useful as redundancy, but it is auxiliary rather than the sole authority.

## Authority rules

- v0.20 remains source/payload authority until newer device evidence genuinely changes the bounded source/topology claim.
- representation oracles establish metadata type only.
- vendor-key names and numeric value `1` are not semantic authority.
- matrix low level means UNSET, not explicit zero.
- source bytes remain sealed before result/vendor interpretation.
- failed/rejected experiments remain provenance.
- appearance and diagnostics never write back into scientific evidence.
