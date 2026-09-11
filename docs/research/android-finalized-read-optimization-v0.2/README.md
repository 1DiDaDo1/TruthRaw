# Android Finalized Read Optimization v0.2

Status: **RESEARCH PASS — HOST EQUIVALENCE + ANDROID ARM64 APK CI PASS; PHYSICAL SPEEDUP UNPROVEN**

## Purpose

Integrate `scientific-master-streaming-binding-v0.2` and `finalized-scientific-preview-release-v0.2` into the existing Android Finalized Scientific Preview route without changing scientific identity, release authority, preview pixels, source/color binding, or appearance semantics.

The execution backend in this proof remains the CPU NDK reference path. TruthRaw does **not** use Vulkan in this build.

## Scientific equivalence contract

The v0.1 and v0.2 finalized routes are required to produce identical:

- Scientific Master digest
- TruthRange zero-line / gauge identity
- 180-byte Technical Backplane
- finalized preview authority
- preview pixels

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

The workflow verified the finalized JNI entry point and empirical probe symbol in the arm64 native library.

## Read-amplification model

For the previously measured Honor 4080×3072 lineage:

- canonical Scientific Master core: 64×64
- canonical tiles per full scientific scan: `64 × 48 = 3072`
- v0.1 self-gauge scans: `4`, therefore `12288` scientific tile reads
- preview streaming: `768 + 768 = 1536` tile reads
- previous physically measured total: `13824` tile reads

v0.2 prediction:

- v0.2 self-gauge scans: `2`, therefore `6144` scientific tile reads
- unchanged preview streaming: `1536` tile reads
- predicted total: `7680` tile reads
- predicted reduction: `6144` tile reads (`44.444...%` of the previous total)

This read-count prediction is **not** a physical performance claim. Wall time, CPU time, RAW payload bytes, PSS, thermal behavior, and energy remain device measurements and must be re-measured on the same Honor with the same DNG.

## Physical baseline to compare against

The prior physical Honor/MotionCam run measured approximately:

- tile reads: `13824`
- pipeline wall time: `110.86 s`
- worker CPU time: `108.01 s`
- RAW payload read: `196.87 MB`
- PSS: approximately `85.2 MiB` before and `88.5 MiB` peak
- thermal status: `NONE` throughout
- finalized preview release: allowed
- physical frame / independent evidence: `1 / 1`
- full RAW materialized: `false`
- scientific color claim: blocked (`SOURCE_METADATA_BOUND`, not `FULL_PHYSICAL`)

## Claim boundary

**Proven by CI on this module:**

- v0.2 scientific streaming tests pass.
- finalized v0.1 ↔ v0.2 host equivalence passes under the stated contract.
- the Android finalized boundary selects v0.2.
- arm64 NDK/JNI linking and APK assembly pass.
- current TruthRaw execution backend has no Vulkan dependency.
- the artifact identity above is fixed and reproducible.

**Not yet proven:**

- that the Honor physical run reaches exactly 7680 tile reads;
- physical wall-time improvement;
- physical CPU-time improvement;
- physical payload-read reduction;
- energy improvement;
- thermal improvement;
- universal performance across devices, lenses, DNG layouts, or RAW formats;
- `FULL_PHYSICAL` color.

The next empirical step is an A/B run on the same Honor with the same original MotionCam DNG used for the 13824-read baseline. Only that result may promote the predicted read reduction to physical evidence.
