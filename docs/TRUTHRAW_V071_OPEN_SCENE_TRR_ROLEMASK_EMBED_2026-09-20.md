# TruthRaw v0.71 — canonical Open Scene in Advanced/TRR + full role-mask embedding

Date: 2026-09-20

Branch:

`integration/truthraw-suite-v0-71-open-scene-trr-rolemask-embed`

App version:

`0.36-v0.71-open-scene-trr-rolemask-embed`

Status: **CI GREEN / APK BUILT — real-device artifact validation pending**.

CI run: `35506676249`

- host GCC PURE writer: SUCCESS;
- host Clang PURE writer: SUCCESS;
- Open Scene/Dynamic Authority/Restoration scientific contract tests: SUCCESS;
- Android arm64 debug APK: SUCCESS;
- documentation governance run `35506676288`: SUCCESS.

Artifact ID: `10604525855`

Artifact ZIP SHA-256:

`d58966ae55cdde57345534255620c55fddf60fea7423efbe81eced833255a3ee`

Extracted APK:

- bytes: `6,314,361`;
- SHA-256: `5653f33c5bbf3263473ee89cfc70e1d0807ce0a529b258b61e3325b791700e46`.

## Purpose

v0.71 closes the first two direct loose cables left after v0.70:

1. propagate the exact canonical Open Scene artifact identity into **Advanced** and the **full-resolution Restoration container itself**;
2. carry the **complete per-pixel Restoration role mask**, not only its SHA-256, inside every normal DNG/TIFF/OpenEXR Restoration projection.

The validated PURE v0.63 route and the v0.67 Restoration algorithm are not changed.

## One shared canonical Open Scene builder

`open_scene_canonical_v0_70` now exposes one source builder:

`canonical_scene::build_from_source(...)`

The builder reads the admitted RAW source in canonical 64×64 source cells and deterministically constructs:

- per-channel Dynamic Authority bytes;
- one exact per-pixel canonical Open Scene categorical state;
- Dynamic Authority SHA-256;
- Open Scene content SHA-256;
- policy SHA-256;
- final domain-separated Open Scene artifact SHA-256.

Permanent invariants remain:

- sampled CFA channel = `CALIBRATED_ESTIMATE` unless source-censored;
- two generic missing colour channels = `UNKNOWN` without admitted source-bound uncertainty;
- `physicalFrameCount=1`;
- `independentEvidenceCount=1`;
- counterfactual pixels = 0;
- Scientific-Master writeback = 0;
- creates-new-evidence = 0;
- chunking may not change scientific identity.

This builder is used by the new consumers instead of giving each consumer its own scene interpretation.

## Advanced now carries the exact Open Scene artifact identity

Advanced still remains a bounded downstream appearance derivative.

Before Advanced release, v0.71 now reconstructs the **full-frame canonical Open Scene identity from the sealed source** and verifies all fail-closed authority invariants.

The Advanced native packet is upgraded from 40 to 48 header integers.

Contract version:

`ADVANCED_CANONICAL_OPEN_SCENE_V0_71 = 3`

Header words 40..47 contain the exact 32-byte canonical Open Scene artifact SHA-256 in little-endian 32-bit words.

Android reconstructs the full hexadecimal SHA-256 and refuses the Advanced packet if the identity is all zero or the v0.71 contract is absent.

Therefore Advanced no longer merely implements equivalent Open-World concepts; it can identify the same full-frame canonical scene artifact that TN-3 and Restoration projections use.

## TRR now stores the canonical Open Scene identity itself

The base scientific Restoration container remains:

`magic=TRUTHRAW_FULLRES_RESTORATION_V0_67`

because the v0.67 Restoration pixel algorithm is unchanged.

v0.71 adds a versioned binding extension:

`binding_extension=TRUTHRAW_TRR_CANONICAL_OPEN_SCENE_ROLEMASK_V0_71`

The TRR header now stores:

- `open_scene_canonical_schema=TruthRawOpenSceneCanonicalState/0.70`;
- semantic parents v0.7 and v0.8;
- `dynamic_authority_artifact_sha256`;
- `open_scene_state_sha256`;
- `open_scene_policy_sha256`;
- `open_scene_artifact_sha256`;
- `restoration_role_mask_sha256`;
- zero counterfactual/writeback/evidence-creation state.

The role-mask SHA-256 is computed over the exact role bytes in the same canonical 64×64 cell-sequence order in which the TRR stores them.

Android staging and post-write verification require these fields before the TRR is accepted.

Old persisted v0.68/v0.70 Restoration job state is intentionally not reused; the job-store namespace is bumped so normal v0.71 projection requires a freshly produced, binding-extended TRR.

## Projection now requires exact TRR scene identity

The projection reader refuses a TRR unless the v0.71 binding extension is present.

It then:

1. verifies the complete TRR derivative raster digest;
2. reconstructs the exact role-mask byte stream and verifies its SHA-256 against the TRR declaration;
3. rebuilds canonical Open Scene from the sealed original RAW;
4. requires:
   `recomputed Open Scene artifact SHA == TRR declared Open Scene artifact SHA`;
5. requires the original source / Scientific Master / Zero-Line / scene-scale / Technical Backplane lineage;
6. only then creates DNG/TIFF/OpenEXR.

This means a detached TRR cannot silently be combined with an unrelated source or scene-state label.

## Full role-mask payload in every normal projection

Role semantics remain:

- `0 = PRESERVE_SCIENTIFIC_MASTER`;
- `1 = AESTHETIC_REINTEGRATION_ONLY`;
- `2 = UNRESOLVED_LOSS`.

Canonical serialization:

`CANONICAL_64X64_CELL_SEQUENCE_UINT8`

For an image of width × height, the embedded mask contains exactly:

`width * height`

bytes.

### Float32 DNG

The existing standards-oriented Float32 LinearRaw DNG path remains TIFF/DNG 1.4.

For a Restoration derivative only, `DNGPrivateData` now contains:

- role-mask SHA-256;
- mask encoding;
- exact mask byte count;
- `restoration_role_mask_embedded=1`;
- binary marker `TRUTHRAW_ROLE_MASK_BINARY_V1`;
- width;
- height;
- canonical tile edge;
- payload length;
- the complete raw role-mask bytes.

The historical PURE path leaves this span empty, so the derivative-only addition does not alter the frozen PURE v0.63 private-data contract.

### Float32 TIFF

The complete mask is embedded in private TIFF tag:

`65000`

Type:

`BYTE`

Count:

`width * height`

The ImageDescription records the role-mask hash, encoding and Open Scene identities.

Readers that do not understand the private tag can ignore it; TruthRaw-aware verification requires it.

### OpenEXR

OpenEXR receives a fourth full-resolution channel:

`TR_ROLE`

Type:

`UINT`

The existing B/G/R channels remain Float32 linear sRGB D65.

Every TR_ROLE sample contains the exact integer role value 0, 1 or 2. Because OpenEXR channels are first-class raster data, the complete role map is self-contained in the EXR.

The header provenance still binds the role-mask SHA-256 and canonical Open Scene artifact SHA-256.

## Projection result contract

Native projection result flags now require:

- derivative raster identity verified;
- source/master/Zero-Line/scene-scale/Backplane lineage verified;
- full resolution;
- complete role-mask bytes embedded;
- canonical Open Scene identity equal to the TRR declaration;
- recomputed role-mask hash equal to the TRR declaration;
- sealed source post-verified;
- v0.71 projection contract active.

Android additionally performs whole-file staging → destination SHA-256 equality.

## Remaining direct cables

After v0.71, direct cable A and B are considered **closed at implementation level**:

- canonical Open Scene identity is now present in TN-3, Advanced, TRR and normal projections;
- the complete Restoration role mask is now self-contained in DNG/TIFF/OpenEXR, not merely hash-bound.

Two empirical cables remain:

1. **independent format conformance**
   - DNG in independent TIFF/DNG readers;
   - TIFF private tag + Float32 tiled raster;
   - EXR B/G/R/TR_ROLE channels in an OpenEXR-compatible reader;
   - exact Float32/role round-trip where readers expose raw samples;

2. **effectful Restoration validation**
   - source with `censored_source_pixels > 0`;
   - prove only role-1 pixels may differ from base Scientific Master;
   - role-2 stays unresolved/base when support is insufficient;
   - projected DNG/TIFF/EXR carry the exact same role map and derivative identity.

Larger later lines remain current PTC/certificate, multi-vendor scientific admission and evidence-bounded physical Scene Physics.

## Non-regression

- PURE remains `TRUTHRAW_PURE_SELF_BINDING_V0_63`;
- v0.67 Restoration eligibility/support math remains unchanged;
- v0.68 transactional foreground commit discipline remains required;
- TN-3 / Open Scene / Restoration / projection remain one scientific lineage, not separate truth worlds;
- normal image formats are projections, not new evidence;
- role-1 never becomes measured;
- role-2 never becomes successfully restored scientific truth.
