# TruthRaw FotoGraaf physical acquisition handoff — 2026-09-14

Status: **ACTIVE CALIBRATION-ACQUISITION CONTINUATION OVERLAY**

This handoff preserves the newest FotoGraaf work after the original knowledge-preservation pack was created. It must be read before continuing C0/C1/C2 physical calibration acquisition.

## Current chain

The research software path is now:

`capture-time identity evidence`

`-> finalized RAW/DNG bytes`

`-> Capture Evidence Seal v0.1`

`-> C0 exact identity seal`

`-> C1/C2 Calibration Intake v0.1`

`-> one dataset-manifest candidate per exact C0 scope`

`-> later model fit + held-out validation + uncertainty validation`

`-> CalibrationPack`

`-> exact scene admission`

`-> MeasurementLab shadow comparison`

`-> Physical Promotion Gate`

No stage changes a later photographed scene's normal `physicalFrameCount=1` or `independentEvidenceCount=1`.

## C0 / intake implementation anchors

C0 capture identity gate was added at commit:

`ba89ba40a1308f55cf6c9b94f82c35c46c2355ec`

C0 -> C1/C2 intake was added at:

`11d40d994c87760baec6ab7b82acd1677228e63b`

The dedicated `FotoGraaf Calibration Intake v0.1` workflow passed on that implementation line.

The intake builder:

- re-runs C0 for every capture;
- rehashes source and metadata bytes;
- keeps FIT/VALIDATION role information;
- partitions by exact `scopeKeySha256`;
- therefore prevents ordinary and exact-ISO8192-associated sample domains from being flattened together;
- does not allow ISO magnitude to select/merge sample domains;
- permits `CANDIDATE_INCOMPLETE` checkpoints but grants no calibration authority;
- recognizes `PHASE_A_C1_C2_COMPLETE_CANDIDATE` only after the planned acquisition-count matrix is satisfied.

Official Phase-A C1/C2 minimums remain:

- C1: seven acquisition anchors, four exposure times per anchor, sixteen independent darks per anchor/exposure cell;
- C2: seven acquisition anchors, twelve signal levels per anchor, eight repeats per level, four held-out validation levels, three saturation-bracket levels.

The nominal ISO anchors are planning anchors only. They do not define the actual gain/readout/sample domain.

## Capture Evidence Seal v0.1

Added at commit:

`39c6b538d7f46b835c33f55f48460c92c19ccf66`

Dedicated `FotoGraaf Capture Evidence Seal v0.1` job: **PASS** on that head.

The sealer formalizes a two-stage authority model:

1. acquisition-time capture evidence is recorded while camera/source-stack state still exists;
2. after the RAW/DNG bytes are finalized, those exact bytes are hashed and bound into the metadata snapshot + C0 record.

Explicitly rejected as authority shortcuts:

- `DNG_METADATA_ONLY` for physical camera ID;
- `EXIF_ONLY`;
- `ISO_ONLY`;
- `ISO_DERIVED`;
- filename/user guessing.

Important architectural finding:

The current TruthRaw Android UI is an **import path** using `ACTION_OPEN_DOCUMENT`. It can verify imported bytes but cannot retroactively invent acquisition-time `physicalCameraId` or equivalent hard C0 evidence. This limitation is intentional and scientifically correct.

Therefore a real Honor calibration run requires a controlled Camera2/source-side capture companion or another source application that emits equivalent acquisition-time identity evidence.

## Exact ISO8192 rule remains active

The previously observed exact-ISO8192-associated sample domain remains an observation, not a causal sensor diagnosis.

Two independent exact ISO8192 dark captures reproduced the high-scale/censored domain while nearby ISO8184 and higher ISO10244 remained ordinary.

Therefore:

- simple `ISO >= 8192` threshold: **REJECTED**;
- exact-ISO8192-associated discrete sample domain: **PASS AS OBSERVATION**;
- physical cause: **OPEN**;
- calibration selection by ISO magnitude: **FORBIDDEN**.

The new C0/intake path enforces this by using exact sample-domain identity instead of numeric ISO ordering.

## Old research frames remain test evidence

Previously supplied scene ISO ladders, response tests and covered dark frames remain valuable source-bound research evidence. They are **not silently promoted** to protocol-compliant calibration captures because they predate the complete acquisition-time C0 evidence envelope and controlled CalibrationPack acquisition protocol.

## Camera2 acquisition companion now exists

A source-side Camera2 companion has now been implemented. It records one `RAW_SENSOR` frame plus capture-time Camera2 observations and saves an observation JSON after the DNG has been finalized and hashed.

Current science boundary remains:

- capture observation is not calibration authority;
- `captureSampleDomainId` and `gainReadoutStateId` are intentionally not inferred from ISO;
- physical camera identity is accepted only when it comes from Camera2 result evidence rather than a filename/EXIF guess;
- the Scientific Master/reconstruction route is not changed to solve acquisition identity.

## Real BKQ-N49 physical-camera findings

Three real Camera2 acquisition observations exposed an important Honor-specific behavior.

Observed on logical camera `0`:

- `logicalMultiCamera=true`;
- advertised physical IDs: `2`, `4`, `5`;
- no physical ID was requested in the original generic route;
- Camera2 reported active physical ID `2`;
- RAW topology was `4096x3072`, BGGR;
- dynamic WhiteLevel was `1023`.

Two other observations used direct Camera2 ID `1`, produced `4096x3072` GRBG RAW, and were not the desired tele path.

Conclusion:

**letting the Honor logical camera choose automatically is not acceptable when calibration requires a specific physical lens/sensor.**

The project-history mapping remains a hypothesis/hint until capture-time result evidence confirms it:

- physical ID `2` -> main candidate;
- physical ID `4` -> ultrawide candidate;
- physical ID `5` -> tele candidate.

## Forced physical tele route

A dedicated `HonorTeleActivity` now requests physical ID `5` through the logical multi-camera output configuration.

Its intended admission rule is fail-closed:

`requested physical ID 5 -> physical output -> physical TotalCaptureResult for 5 -> timestamp identity -> finalized RAW hash`

If the vendor stack silently falls back to the main camera or cannot produce a physical result for ID `5`, the route must fail rather than save a file labelled tele.

Device validation of this forced route remains OPEN.

## Physical-camera capability probe v0.1

Before adding separate main/wide/tele/max-resolution capture buttons, the project now has a read-only capability inventory:

`HonorCapabilityProbeActivity.kt`

Research document:

`docs/research/honor-physical-camera-capability-probe-v0.1/HONOR_MAGIC8_PRO_PHYSICAL_CAMERA_CAPABILITY_ARCHITECTURE_2026-09-14.md`

The probe inventories every advertised physical route and records independently:

- standard `RAW_SENSOR` output sizes;
- maximum-resolution `RAW_SENSOR` sizes;
- standard and maximum-resolution RAW14 sizes if the runtime exposes `ImageFormat.RAW14`;
- regular/max-resolution pixel and active arrays;
- CFA;
- RAW, MANUAL_SENSOR and ultra-high-resolution capability flags;
- sensitivity/exposure range;
- focal lengths;
- minimum focus distance and focus-distance calibration;
- AF modes including macro;
- OIS modes.

The report class is:

`CAMERA2_CAPABILITY_OBSERVATION_ONLY`

and explicitly grants neither capture nor calibration authority.

## Native maximum-resolution / 200 MP rule

A marketed 200 MP sensor or a stock-camera 200 MP JPEG is not proof of a public native RAW route.

TruthRaw first requires Camera2 to advertise a maximum-resolution RAW stream for the relevant route. A maximum-resolution RAW of at least `180,000,000` pixels is only marked as a **200 MP-class candidate**.

It becomes capture proof only after:

`maximum-resolution stream -> matching sensor pixel mode -> physical output session -> requested physical camera result -> RAW/result timestamp identity -> sealed source SHA-256 -> C0`

If Camera2 exposes only a lower-resolution RAW while Honor can generate a 200 MP processed image, TruthRaw must not invent a 200 MP Direct-CFA source.

Main, ultrawide and tele all use this same rule.

## Macro / close-focus rule

Macro is not inferred from a marketing label or crop.

The capability probe records:

- `LENS_INFO_MINIMUM_FOCUS_DISTANCE`;
- focus-distance calibration;
- `CONTROL_AF_AVAILABLE_MODES`;
- whether `CONTROL_AF_MODE_MACRO` is actually advertised.

A future ultrawide macro RAW route must prove the physical sensor actually captured the RAW. The vendor camera may otherwise be switching sensors or using a processed/private mode.

Tele should be called **tele close focus** until measured focus/reproduction evidence supports the stronger word `macro`.

## Android 17 / RAW14 rule

RAW14 is treated as an independent capability axis, not as something automatically gained merely because the operating system is Android 17.

The current branch compiles against API 35. The capability probe therefore detects `ImageFormat.RAW14` through runtime reflection so the Android-16 code path remains buildable while an Android-17+ runtime can reveal the new format.

Required RAW14 sequence:

1. runtime exposes RAW14;
2. selected physical camera advertises a RAW14 stream size;
3. physical session/capture succeeds;
4. packed 14-bit byte/sample semantics are independently validated;
5. original packed source bytes remain sealed.

RAW14 and maximum-resolution/200 MP are separate capabilities. TruthRaw must never combine them unless the exact camera/mode advertises and proves both together.

## Next implementation gates

The next order is now:

1. run the physical-camera capability probe on BKQ-N49;
2. inspect the exact JSON for physical IDs `2`, `4`, `5`;
3. classify standard/max-resolution RAW candidates per physical route;
4. inspect wide/main/tele focus/macro characteristics;
5. on Android 17 later, inspect RAW14 advertisement per physical route;
6. only then build runtime session probes and capture routes for the modes actually advertised;
7. require physical capture proof before binding any route into C0;
8. only C0-passing routes may enter C1/C2/C3/... calibration acquisition.

Do not modify the Scientific Master/reconstruction route to solve this acquisition problem.
