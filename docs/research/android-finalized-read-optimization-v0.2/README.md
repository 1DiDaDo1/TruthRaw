# Android Finalized Read Optimization v0.2

Status: **RESEARCH PASS — HOST EQUIVALENCE + ANDROID ARM64 CI + PHYSICAL HONOR READ-REDUCTION PASS FOR THE RECORDED DNG/APK**

## Purpose

Integrate `scientific-master-streaming-binding-v0.2` and `finalized-scientific-preview-release-v0.2` into the existing Android Finalized Scientific Preview route without changing scientific identity, release authority, preview pixels, source/color binding, or appearance semantics.

The execution backend in this proof remains the CPU NDK reference path. TruthRaw does **not** use Vulkan in this build.

## Scientific equivalence contract

The v0.1 and v0.2 finalized routes are required to produce identical:

- Scientific Master digest;
- TruthRange zero-line / gauge identity;
- 180-byte Technical Backplane;
- finalized preview authority;
- preview pixels.

Only bounded execution/resource metrics may differ.

The optimized Scientific Master self-gauge median keeps the same positive finite uncensored Stage-2 float32 ordering and median definition, but replaces four 8-bit radix refinement scans with two exact 16-bit radix scans.

## Validated implementation lineage

- optimized finalized scientific route implementation: `a84186969b9c767c6e291d9697490e63867b7b82`
- Android/provenance exact build head: `53ad42196f7d4509471120e4579fc57b8f85498a`
- workflow: `Android Finalized Read Optimization v0.2`
- workflow run: `34655957096`
- conclusion: `success`
- host-equivalence job: `success`
- arm64 APK job: `success`

The empirical report embedded in the APK identifies the validated scientific route as `a84186969b9c767c6e291d9697490e63867b7b82`.

## Android artifact

- app version: `0.4-readopt`
- ABI: `arm64-v8a`
- APK bytes: `4221045`
- APK SHA-256: `d83ef4ac5344dbc02b1216e7c995131955718c4d8e00b46568bf3e8166e53d03`
- GitHub artifact name: `truthraw-finalized-readopt-v0.2-debug-arm64`
- artifact ID: `10286210836`
- artifact ZIP bytes: `1338002`
- artifact ZIP SHA-256: `6e2bfe6c4086dd55689583fdc5e11f10e6da2787cba06a320b9a1dc38d316c97`

## Read-amplification model

For the previously measured Honor 4080×3072 lineage:

- canonical Scientific Master core: 64×64;
- canonical tiles per full scientific scan: `64 × 48 = 3072`;
- v0.1 self-gauge scans: `4`, therefore `12288` scientific tile reads;
- preview streaming: `768 + 768 = 1536` tile reads;
- previous physically measured total: `13824` tile reads.

v0.2 prediction was:

- v0.2 self-gauge scans: `2`, therefore `6144` scientific tile reads;
- unchanged preview streaming: `1536` tile reads;
- predicted total: `7680` tile reads;
- predicted reduction: `6144` tile reads (`44.444...%`).

The physical Honor run below reached **exactly 7680 tile reads**, promoting this read-count prediction to physical evidence for this exact device / APK / DNG lineage.

## Physical Honor evidence — 2026-09-11

Evidence file:

`evidence/HONOR_BKQ-N49_IMG_260830_143012_297_014_EMPIRICAL_2026-09-11.json`

Recorded device / source / app:

- device: HONOR BKQ-N49 / HNBKQ;
- Android: 16, SDK 36;
- source: `IMG_260830_143012_297_014.dng`;
- source SHA-256: `fac842110cfd5b527aae2fa46237824d444aa5701017c2b68021962ebce47da3`;
- source bytes: `25369034`;
- app: `0.4-readopt`, version code 4;
- installed APK SHA-256: `d83ef4ac5344dbc02b1216e7c995131955718c4d8e00b46568bf3e8166e53d03`;
- validated scientific route: `a84186969b9c767c6e291d9697490e63867b7b82`.

Source/color stability:

- pre-probe status: `0`;
- post-probe status: `0`;
- exact source SHA and byte length stable across wrapper: `true`;
- color-probe fingerprint stable across wrapper: `true`;
- metadata form: `DUAL_ILLUMINANT_V0_2`;
- dual illuminant used: `true`;
- ForwardMatrix used: `true`;
- CameraCalibration present / signature matched / applied: `true / true / true`;
- illuminants: `21 / 17`;
- resolved white temperature: `4841.036 K`;
- interpolation weight low: `0.267578212`;
- physical frame / independent evidence: `1 / 1`.

Finalized preview result:

- outcome: `READY`;
- preview authority: `FINALIZED_SOURCE_BOUND_SCIENTIFIC_PREVIEW`;
- source dimensions: `4080 × 3072`;
- tile reads: `7680`;
- RAW payload bytes read: `137065232`;
- metadata bytes read: `5389895`;
- full RAW materialized: `false`;
- source-bound appearance release allowed: `true`;
- Scientific Preview release allowed: `true`;
- scientific color claim allowed: `false`;
- preview passes: `768 / 768` tiles;
- ForwardMatrix used: `true`;
- CameraCalibration applied: `true`.

Runtime observations for the finalized loader only, excluding pre/post probes:

- wall time: `98153.604181 ms` (`98.154 s`);
- worker-thread CPU time: `97189.17089 ms` (`97.189 s`);
- PSS before: `87047 KiB` (`~85.01 MiB`);
- sampled PSS peak: `92176 KiB` (`~90.02 MiB`);
- PSS after: `86476 KiB` (`~84.45 MiB`);
- thermal start / peak / end: `NONE / NONE / NONE`.

Visible-run frame pacing:

- intervals: `7208`;
- p50: `16.589175 ms`;
- p95: `16.595701 ms`;
- maximum: `16.666667 ms`.

## A/B against previous physical Honor baseline

Previous physical baseline on the same source lineage was approximately:

- tile reads: `13824`;
- pipeline wall time: `110.86 s`;
- worker CPU time: `108.01 s`;
- RAW payload read: `196.87 MB`;
- PSS: `~85.2 MiB` before, `~88.5 MiB` sampled peak;
- thermal: `NONE` throughout.

Observed v0.2 change:

- tile reads: `13824 → 7680`: **-6144 / -44.444%**;
- wall time: `~110.86 s → 98.154 s`: **~11.5% lower**, about `1.13×` faster;
- worker CPU: `~108.01 s → 97.189 s`: **~10.0% lower**, about `1.11×` faster;
- RAW payload: `~196.87 MB → 137.065 MB`: **~30.4% lower**;
- sampled PSS peak: `~88.5 MiB → ~90.02 MiB`: **slightly higher**, so memory improvement is **not** claimed;
- thermal remained `NONE` throughout both runs.

The runtime comparison uses the prior recorded baseline values, which were rounded. The exact, strongest physical result is the tile-read reduction because both endpoints are exact integer measurements.

## Claim boundary

**Now proven for this exact Honor / APK / DNG evidence root:**

- v0.2 reaches the predicted `7680` tile reads physically;
- the exact `44.444%` tile-read reduction versus the prior `13824` run is physically observed;
- finalized release remains `READY` and `FINALIZED_SOURCE_BOUND_SCIENTIFIC_PREVIEW`;
- source and color audit remain stable pre/post;
- physical frame / independent evidence remain `1 / 1`;
- no full RAW is materialized;
- wall time, worker CPU time and RAW payload read are lower in this A/B;
- no thermal escalation was observed.

**Not proven / not promoted:**

- universal performance across devices, lenses, DNG layouts, or RAW formats;
- energy improvement, because no direct energy measurement was recorded;
- memory improvement, because sampled peak PSS rose slightly;
- `FULL_PHYSICAL` color;
- independent camera/lens color calibration;
- Vulkan equivalence or Vulkan performance;
- multi-capture fusion/HDR science.

Source-metadata-bound color remains source-bound. `scientific_claim_allowed=false` is preserved and is a success of the authority boundary, not a failure of the read optimization.

The canonical TruthRaw rule remains:

**Measured where measured. Reconstructed where necessary. Never invented.**
