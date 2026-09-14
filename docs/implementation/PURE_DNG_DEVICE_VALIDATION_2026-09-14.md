# TRUTHRAW PURE DNG device-validation gate — 2026-09-14

Status: **ACTIVE VALIDATION SUPPORT / DOES NOT CHANGE SCIENTIFIC AUTHORITY**

This document adds a downstream validation tool around the already-green PR #23 export path. It deliberately does **not** change the sealed source model, reconstruction, Scientific Master, color authority, Technical Backplane, float32 writer, certificate record, or Android export algorithm.

## Why this exists

PR #23 is host-validated, but production claims remain blocked on real-device Honor/MotionCam end-to-end output. The next gate requires inspection of the actual DNG saved by Android, rather than inferring success from host tests or from an in-process return packet.

Use:

```text
python3 tools/verify_truthraw_pure_dng_artifact_v0_1.py \
  --source <exact-source.dng> \
  --output <android-produced-truthraw-pure.dng> \
  --json-out <report.json>
```

If the Android telemetry records exact negative and greater-than-one component counts, add:

```text
  --expected-negative-count <N> --expected-over-one-count <N>
```

## What the verifier proves

For the current `TRUTHRAW_PURE_FLOAT32_DNG` artifact contract it fails closed unless it can verify, from the saved file and exact source:

- the saved output dimensions equal the source image dimensions;
- classic little-endian TIFF/DNG structure used by the current writer;
- `PhotometricInterpretation=34892` (`LinearRaw`), 3 samples/pixel, IEEE float32 sample format, no TIFF compression, chunky layout and canonical 64x64 tiling;
- DNG 1.4 tags, D50 calibration illuminant, identity `ColorMatrix1`, D50 `AsShotNeutral`, and the current writer identity strings;
- every real XYZ-D50 float component is finite, while exact negative and `>1` counts are measured from the serialized artifact;
- the pre-existing TruthRaw projection metadata remains present in `DNGPrivateData`;
- the appended 296-byte `TRCERT01` Certificate v0.1 is structurally valid and has a correct record CRC;
- certificate projection class is TRUTHRAW PURE float32, claim class is reconstructed, and state is exactly `UNSIGNED DEVELOPMENT` / no signature;
- source SHA-256 matches the exact supplied source, the private projection record and the embedded certificate;
- Scientific Master SHA-256 agrees between the private projection record and certificate;
- `physicalFrameCount=1` and `independentEvidenceCount=1` remain intact;
- the public build/pipeline identity matches the current Android PURE v0.1 export path.

A PASS is **artifact/projection integrity**, not proof of physical truth, independent sensor calibration, Adobe interoperability, or cryptographic issuer authenticity.

## Explicit v0.1 limitation found during this continuation

The Android exporter currently checks `appearanceApplied == false` and `counterfactualObservationCreated == false` before it commits/releases a PURE artifact. Certificate v0.1 and the current `DNGPrivateData` record do not independently serialize those two booleans.

Therefore the saved file can prove its PURE projection/claim class, representation-only role and 1/1 evidence lineage, but it cannot independently re-derive those two internal booleans from file bytes alone. The verifier reports both fields as `NOT_SERIALIZED_IN_V0_1` instead of upgrading a code-path guarantee into a stronger artifact-level claim.

This is a validation finding, not permission to redesign the certificate during the current frozen-export gate. A later certificate/schema revision may add explicit treatment/appearance/counterfactual fields if that can be done without weakening backward compatibility or authority boundaries.

## Five-source device run

Run the verifier on Android-produced PURE outputs made from the existing physical regression set:

- `IMG_260816_134122_304_005.dng`
- `IMG_260816_134204_911_008.dng`
- `IMG_260816_143716_085_041.dng`
- `IMG_260830_143012_297_014.dng`
- `IMG_260908_193845_662_027.dng`

The five files remain **test evidence, not calibration evidence**. Keep each source SHA, output SHA, verifier JSON report, Android exporter telemetry and device/app build identity together as one validation bundle.

## Remaining gate after a five-file PASS

A five-file verifier PASS advances the real-device artifact/integrity gate, but it does not close Adobe/Lightroom interoperability. Test Adobe Camera Raw / Lightroom ingestion separately, and preserve any clipping/rendering differences as downstream reader behavior rather than rewriting the Scientific Master to accommodate a reader.
