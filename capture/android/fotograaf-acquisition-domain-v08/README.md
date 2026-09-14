# TruthRaw FotoGraaf Acquisition Domain Probe v0.8

Research-only Camera2 probe for HONOR BKQ-N49.

The UI deliberately separates:

1. **Standard Camera2 inventory** — public Android capability discovery.
2. **HONOR inventory** — vendor-key observation only.
3. **4080x3072 tele RAW route-proof** — preferred `logical 0 -> physical 5` route.
4. **16320x12288 maximum-resolution RAW probe** — only when the exact size is advertised in the maximum-resolution high-resolution RAW_SENSOR list.

Every capture stays `CAMERA2_ACQUISITION_OBSERVATION_ONLY`; the app never grants calibration authority or seals C0 identity.

## Important v0.8 correction
Unlike the earlier standalone v0.7 200MP probe, v0.8 preserves the physical camera's logical parent. If camera 5 is exposed as a physical child of logical camera 0, v0.8 opens the logical camera and binds the `OutputConfiguration` to physical ID 5 instead of blindly trying to open ID 5 directly.

## Returned evidence
The app writes to its external-files evidence directory and can share the current session files:

- `truthraw_standard_camera2_inventory_v08.json`
- `truthraw_honor_vendor_inventory_v08.json`
- per-domain session-support JSON
- exact app-visible `.rawsensor` or `.rawbuffer` payload
- DNG when Android `DngCreator` succeeds
- `*_acquisition_domain_observation_v08.json`

The raw payload is hashed and streamed directly from the `Image.Plane` buffer without a whole-frame Java/Kotlin byte-array copy.

The observation records standard Camera2 pixel-mode/binning/UHR/remosaic fields, physical-result binding, timestamps, focal length, requested/applied stabilization controls, HONOR vendor results, payload identity and fail-closed proof gates.

## Scientific boundary
A successful run proves only an **app-visible Camera2 RAW sample lattice on the requested runtime route**. It does not prove untouched photodiode/ADC values or absence of upstream sensor/HAL processing. `SENSOR_INFO_LENS_SHADING_APPLIED` is recorded explicitly because `true` is an upstream-processing boundary.
