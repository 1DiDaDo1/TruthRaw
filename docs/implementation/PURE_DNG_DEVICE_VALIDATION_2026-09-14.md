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

## Completed source-bound Honor smoke run

A supplemental real-device Honor run has now been closed all the way from the exact sealed source bytes to the saved TRUTHRAW PURE artifact. This capture is **not one of the formal five regression sources listed below**, so it advances the real-device smoke/validator evidence but does not count as one of the required five-file gate entries.

Device/app binding:

- device: `HONOR BKQ-N49`, Android 16;
- app: `0.7-four-mode-certificate`;
- installed APK SHA-256: `1c1acd4855a77c2547efdc3e540ebc96a9455f396ff0ddf8eb69fa8a519392f6`;
- empirical wrapper validated scientific-route SHA: `a84186969b9c767c6e291d9697490e63867b7b82`.

Exact source:

- file: `IMG_260908_193753_205_015.dng`;
- bytes: `25899870`;
- dimensions: `4080x3072`;
- SHA-256: `757f6aaa0b45a0e68270ee8073d338895d2df284d8ae57bca5683a5ca2ba2573`;
- the empirical pre-probe and post-probe both reported this exact SHA and byte length.

Saved PURE artifact:

- file: `IMG_260908_193753_205_015_truthraw_pure_float32_v0_1.dng`;
- bytes: `151021352`;
- SHA-256: `a80390deb51e315e73973782f151555521eed819383ebe45a2906a9b321dc788`;
- `4080x3072`, `LinearRaw`, 3 samples/pixel, IEEE float32, uncompressed, chunky 64x64 tiling;
- 37,601,280 real XYZ-D50 float components checked and all finite;
- 288,739 serialized components are negative;
- 218,566 serialized components are greater than 1;
- measured serialized real-component range: approximately `-0.0091582136 .. 1.45196819`;
- 147,456 tile-padding components were checked and all were exactly zero.

Provenance/certificate binding:

- the actual source SHA, `DNGPrivateData.sealed_source_sha256` and `TRCERT01.sourceEvidenceSha256` are identical;
- `DNGPrivateData.scientific_master_sha256` and the certificate Scientific Master SHA agree at `dc747bc3bc9d7ab24614560ead67008455742c6732faf6f1a3167d56670eae97`;
- certificate record CRC32 verifies as `9bc43cf5`;
- Technical Backplane CRC32 in the certificate is `2144df1c`;
- certificate build identity matches `e8ed38cc9b92378760f60ae17d90dc6b3149f7e4405cc97eb7a9f36166cc8e55`;
- `physicalFrameCount=1` and `independentEvidenceCount=1` remain intact;
- signature state remains exactly `UNSIGNED DEVELOPMENT` with no issuer key/signature material.

Authority boundary remains unchanged: the artifact is a representation-only projection, not a new measurement; the empirical harness keeps `scientific_claim_allowed=false`, does not claim source-metadata color as independent physical calibration, and does not treat counterfactuals as evidence. Certificate v0.1 still does not serialize the two internal appearance/counterfactual booleans discussed above.

**Result for this supplemental run: PASS — exact-source artifact/projection integrity + device/app cross-binding.** This does not close the formal five-source device gate, Adobe/Lightroom interoperability, independent physical color calibration, or trusted cryptographic issuer validation.

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
