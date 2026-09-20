# TruthRaw v0.73 — Camera source admission + TN-3 continuity — 2026-09-20

## Status

Active integration branch:

`integration/truthraw-suite-v0-73-camera-source-admission-tn3`

Android app version:

`0.38-v0.73-camera-source-admission-tn3`

v0.73 is an integration/research step on top of the proven v0.72 projection lifecycle baseline. It does not promote the branch to canonical/main and does not change PURE v0.63 pixel mathematics, the v0.67 Restoration algorithm, or v0.71 Open Scene/role-mask projection semantics.

## Why this step exists

The product launcher already exposed two intended source routes:

`Open RAW / DNG`
and
`Gebruik camera`.

Before v0.73, the camera tile still opened the older generic FotoGraaf route while the later Camera-5 research line lived separately in diagnostic activities.

v0.73 connects the latest active physical-Camera-5 capture route into the normal product entry while preserving all scientific boundaries learned from Camera-5 v0.14 through v0.20 and the Android-17 v0.53 replay.

## Camera route selected

The newest capture-capable route is v0.53, not v0.55.

- v0.53 actively replays the proven v0.14 source-first Camera-5 path on Android 17.
- v0.55 is a read-only current Android/HONOR capability baseline and intentionally does not open a camera.

Product entry now opens `FotoGraaf200MpStagedActivity` in `EXTRA_PRODUCTION_CAMERA_ENTRY` mode.

The production mode automatically performs capability admission and live preview. The user-facing surface is reduced to the live camera and one RAW capture action; the detailed staged controls remain available to research mode.

## Capture law

The capture path remains one physical frame / one independent evidence item.

The route:

`logical 0 preview -> active physical 5 observation -> physical-5 bound MAX RAW output -> physical-scoped still request -> Image + physical result timestamp identity -> source-first RAW_SENSOR seal`.

The original app-visible `Image.Plane[0]` is persisted and SHA-256 sealed before advisory metadata is interpreted.

The app-visible Camera2 buffer remains bounded as:

`APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF`.

No vendor key name, maximum-resolution advertisement or envelope dimension is allowed to promote the buffer to untouched ADC or native optical 200 MP evidence.

## Critical 16320x12288 envelope rule

The v0.19/v0.20 result remains controlling evidence.

The known Camera-5 capture can expose a `16320x12288` RAW_SENSOR envelope of 401,080,320 bytes while only the first 25,067,520 bytes are populated. That byte count equals exactly one runtime-advertised standard RAW_SENSOR geometry:

`4080 x 3072 x 2 bytes`.

Therefore v0.73 explicitly forbids:

`16320x12288 envelope -> direct 200 MP Scientific Master`.

After sealing, v0.73 performs a read-only `RawSensorRasterAudit` and `RawPayloadGeometryDecoder`.

Only when:
- the sealed SHA-256 re-verifies;
- populated payload topology resolves uniquely;
- the payload byte count matches exactly one advertised STANDARD RAW_SENSOR geometry;
- the exact candidate prefix is copied with no transform;

may that candidate continue.

Any changed or ambiguous topology fails closed before Scientific Master creation.

## Derived processing DNG

When topology admission passes, v0.73 writes a processing DNG from the exact admitted RAW prefix with Android `DngCreator.writeInputStream`.

This DNG is explicitly:

`DERIVED_CONTAINER_FOR_SAME_MAIN_HOUSE_DNG_ADMISSION`.

It is not a replacement for the sealed upstream RAW_SENSOR evidence and does not inherit scientific authority merely because it came from the camera.

The processing DNG is then handed to the same `MainActivity` DNG ingress used by imported DNG files.

Main House re-seals/admit the DNG independently. The ingress object carries:
- camera-capture origin;
- acquisition evidence JSON path;
- upstream sealed RAW_SENSOR SHA-256;
- upstream role `APP_VISIBLE_CAMERA2_RAW_SENSOR_SOURCE_FIRST_SEALED`.

The UI explicitly states that authority is not inherited.

## TruthNegative role

TruthNegative is intentionally downstream from Main House admission.

Correct route:

`sealed camera RAW_SENSOR envelope`
-> `read-only topology admission`
-> `exact admitted sample-domain`
-> `derived processing DNG`
-> `same Main-House DNG seal/admission`
-> `Scientific Master / TruthRange / Technical Backplane`
-> `Dynamic Authority / Canonical Open Scene`
-> `TruthNegative TN-3`.

TN-3 can therefore preserve a full-resolution camera-native Float32 Scientific Negative for the admitted Scientific Master and bind the canonical Open Scene identity.

TN-3 must not manufacture 200 MP colour/detail from an unpopulated HAL envelope, convert censored values to exact measurements, or upgrade reconstructed channels to measured authority.

## UI restoration

The polished v0.65 product launcher is restored:
- the existing clean TruthRaw icon is retained;
- JPG and disabled JPG XL occupy the first output row;
- TRUTHRAW PURE and TRUTHRAW ADVANCED occupy the second row;
- Advanced again displays `Natural HDR · Light · Detail · Restoration`;
- TruthNegative remains available in the scientific processing flow instead of consuming a primary launcher card.

A new compact-height layout reduces header, input-card and output-card dimensions on shorter phone viewports so PURE and ADVANCED no longer appear half cut off at the bottom of the initial screen.

## Frozen downstream science

Unchanged:
- `TRUTHRAW_PURE_SELF_BINDING_V0_63`;
- exact Zero-Line / L0 / scene-scale / Technical Backplane binding;
- v0.67 Restoration algorithm;
- v0.70 canonical Open Scene semantics;
- v0.71 full role-mask embedding;
- v0.72 projection lifecycle/progress fixes;
- one physical frame / one independent evidence item.

## Validation

CI workflow:

`.github/workflows/android-truthraw-suite-v0-73-camera-source-admission-tn3.yml`

Initial run:

`35513839890`

At creation time this run was queued. Do not call the APK validated until Android compilation and scientific contract jobs are green.

## Required real-device test

1. Install the green v0.73 APK.
2. Open TruthRaw and tap `Gebruik camera`.
3. Confirm a real live camera preview opens automatically.
4. Capture exactly one RAW frame.
5. Confirm the source-first envelope is sealed.
6. Confirm topology admission either:
   - admits the exact standard RAW prefix and enters Main House automatically; or
   - fails closed without creating a Scientific Master.
7. If admitted, confirm the resulting Main-House source reports camera origin and upstream SHA ancestry.
8. Produce TN-3 and PURE only after Main-House admission.
9. Upload the evidence JSON and, if possible, the admitted processing DNG for independent inspection.

## Open scientific issue

The exact standard prefix is an app-visible admitted sample-domain, not proof of native sensor geometry, binning mechanism, remosaic mechanism or 200 MP optical resolution.

A genuinely new full-population Camera-5 domain requires its own black/white/noise/uncertainty/colour/shading/optics validation before stronger authority is granted.
