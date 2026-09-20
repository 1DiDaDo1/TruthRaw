# TruthRaw next-chat handoff — 2026-09-20

**READ THIS FIRST.**

Active branch:

`integration/truthraw-suite-v0-73-camera-source-admission-tn3`

Current app version:

`0.38-v0.73-camera-source-admission-tn3`

This branch is the current integration/research line. It is not a main/canonical promotion.

## Permanent law

> Measured where measured. Reconstructed where necessary. Never invented.

> Representation can exceed the source. Knowledge claims cannot exceed the evidence.

The original admitted source remains immutable. The free scientific world may build richer representations, but every value must preserve authority, ancestry, uncertainty/support and intervention history.

## What changed after v0.72

v0.72 remains the frozen projection-lifecycle baseline.

v0.73 makes the product camera tile a real camera ingress and connects the latest active Camera-5 capture route to the same Main-House DNG admission used by imported RAW.

The visible launcher keeps the polished v0.65 style and clean TruthRaw icon.

### Product UI

The launcher again shows:
- JPG;
- JPG XL, disabled / not yet admitted;
- TRUTHRAW PURE;
- TRUTHRAW ADVANCED with Natural HDR / Light / Detail / Restoration.

TruthNegative no longer consumes the JPG XL launcher card. TN-3 remains available in the scientific processing path.

Compact phone heights receive reduced header/card sizes so PURE and ADVANCED are not half cut off.

### Camera route

`Gebruik camera` now opens `FotoGraaf200MpStagedActivity` in production mode.

Production mode:
- requests CAMERA permission if necessary;
- runs Camera-5 capability admission automatically;
- opens a real logical-0 live preview at the tele route;
- exposes one RAW capture button instead of the staged laboratory controls.

The latest active capture basis is v0.53, not v0.55:
- v0.53 = active Android-17 replay of the proven v0.14 Camera-5 source-first route;
- v0.55 = read-only current Android/HONOR baseline only.

## Critical Camera-5 scientific boundary

Do not infer 200 MP measurement authority from the 16320x12288 Camera2 envelope.

Historical v0.19/v0.20 evidence found:
- envelope: 16320x12288, 401,080,320 bytes;
- only rows 0..767 populated in the qualifying sample;
- populated prefix: 25,067,520 bytes;
- exactly equals 4080x3072x2 bytes;
- 4080x3072 was the only runtime-advertised STANDARD RAW_SENSOR geometry with that byte count.

Therefore v0.73 enforces:

`sealed 16320x12288 envelope`
-> `RawSensorRasterAudit`
-> `RawPayloadGeometryDecoder`
-> unique exact advertised standard RAW byte-match required
-> exact prefix copied with no transform
-> derived processing DNG
-> normal Main-House DNG seal/admission.

If topology is ambiguous or DNG creation fails, the route stops before Scientific Master creation.

The envelope is never directly promoted to a 200 MP Scientific Master.

## Source ancestry

The original app-visible RAW_SENSOR buffer is sealed first and remains upstream acquisition evidence.

The derived processing DNG carries no inherited truth authority. Main House must independently re-seal/admit it.

The camera-origin RawHandle now records:
- acquisition evidence JSON path;
- upstream RAW_SENSOR SHA-256;
- upstream role `APP_VISIBLE_CAMERA2_RAW_SENSOR_SOURCE_FIRST_SEALED`;
- source route `CAMERA_CAPTURE`.

The Main UI states explicitly that authority is not inherited.

## TruthNegative

TruthNegative TN-3 is important, but only downstream.

Correct chain:

`sealed camera source`
-> `admitted sample-domain`
-> `Main-House DNG admission`
-> `Scientific Master + Zero-Line + scene-scale + Backplane`
-> `Dynamic Authority + Canonical Open Scene`
-> `TN-3`.

TN-3 is the full-resolution camera-native Scientific Negative/state carrier for the admitted Scientific Master. It may preserve reconstructed full-colour RGB and Open Scene identity at that admitted resolution.

TN-3 may not:
- turn the 16320x12288 HAL envelope into 200 MP evidence;
- invent missing measured colour;
- convert CENSORED or UNKNOWN into exact measurement;
- upgrade reconstructed channels to MEASURED.

## Frozen science

Do not change while closing v0.73:
- PURE pixel math / self-binding: `TRUTHRAW_PURE_SELF_BINDING_V0_63`;
- Zero-Line/L0/scene-scale/Technical Backplane semantics;
- v0.67 Restoration algorithm;
- v0.70 Canonical Open Scene semantics;
- v0.71 full role-mask embedding;
- v0.72 projection lifecycle/progress fixes.

## Read next

1. `state/CURRENT_PROJECT_STATE_2026-09-20.json`
2. `docs/TRUTHRAW_V073_CAMERA_SOURCE_ADMISSION_TN3_2026-09-20.md`
3. `docs/TRUTHRAW_V072_PROJECTION_LIFECYCLE_PROGRESS_FIX_2026-09-20.md`
4. `docs/HONOR_MAGIC8_PRO_TELE_PAYLOAD_GEOMETRY_V020_2026-09-17.md`
5. `docs/TRUTHRAW_V071_OPEN_SCENE_TRR_ROLEMASK_EMBED_2026-09-20.md`
6. `docs/TRUTHRAW_FULL_GENEALOGY_RECOVERY_AUDIT_2026-09-19.md`

## CI / APK status

Workflow:

`.github/workflows/android-truthraw-suite-v0-73-camera-source-admission-tn3.yml`

Initial run:

`35513839890`

At the time this handoff was created, CI had been queued. Do not claim the v0.73 APK is validated until both scientific-contract and Android-build jobs are green.

## Immediate continuation

1. Check v0.73 CI.
2. Fix any compile/guard failure without weakening the source/topology gates.
3. Download the green APK artifact.
4. Real-device test:
   `launcher -> Gebruik camera -> live preview -> one RAW capture -> source-first seal -> topology admission -> Main House`.
5. Verify the known device either admits the exact 4080x3072 standard prefix or fails closed if current Android/HONOR behavior changed.
6. After admission, test PURE and TN-3.
7. Upload the evidence JSON and admitted processing DNG for independent inspection.
8. Only after v0.73 acquisition is proven should the larger code/science upgrades (ancestry-aware Open Scene, v4.7j Detail integration, richer illumination/HDR/restoration) continue.
