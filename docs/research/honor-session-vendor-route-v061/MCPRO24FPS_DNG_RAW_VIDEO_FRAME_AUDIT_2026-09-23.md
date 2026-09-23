# TruthRaw v0.61 — mcpro24fps DNG RAW-video frame audit

Date: 2026-09-23

Status: HOST-SIDE READ-ONLY FILE AUDIT / NO PRODUCTION CHANGE

## Files

### Frame 00000000
- file: `V260923-192008-00000000.DNG`
- bytes: `16,605,437`
- SHA-256: `75fea2e8d749b34eb7c8fb1d26952451725c7a61596543fc9a1b9f0017bfdf9f`

### Frame 00000022
- file: `V260923-192008-00000022.DNG`
- bytes: `16,605,437`
- SHA-256: `ee4f74e84bae0a0105100fb6d0adfc9d17c06f9a15923d4ade15a5abf6273068`

The two full-file hashes differ.

## DNG / camera identity

Both files report:

- Software: `mcpro24fps`
- Make: `HONOR`
- Model: `BKQ-N49`
- UniqueCameraModel: `HONOR BKQ-N49 HNBKQ BKQ-N49 5:5`
- ReelName: `V260923-192008-60fps-auto_exp.mcpro24fps.zip`
- FrameRate DNG tag: `60/1`
- focal length: approximately `22.48 mm`
- aperture: approximately `f/2.6`
- ISO: `51200`
- exposure: approximately `1/120 s`
- orientation: 6
- CFARepeatPatternDim: 2x2
- CFAPattern bytes: `02 01 01 00`
- PhotometricInterpretation: CFA
- DNG 1.4

This is file/container metadata; it is not independent proof of physical sensor identity or untouched ADC provenance.

## Declared raster and storage

Both DNGs declare:

- width: `3840`
- height: `2160`
- BitsPerSample: `16`
- SampleFormat: unsigned integer
- Compression: none
- one strip
- RowsPerStrip: `2160`
- StripOffsets: `8`
- StripByteCounts: `16,588,800`

The strip byte count is exactly:

`3840 * 2160 * 2 = 16,588,800`

Therefore these DNGs store one 16-bit container word for every declared raster position.

## Numerical RAW code domain

The DNG WhiteLevel is `1023`.

Frame 0:
- observed min/max: `0 / 528`
- bitwise OR of all sample codes: `0x3ff`
- no samples above 1023

Frame 22:
- observed min/max: `0 / 482`
- bitwise OR: `0x1ff`
- no samples above 1023

So the file is a 16-bit DNG storage representation carrying a source code domain compatible with <=10 significant bits in these two frames.

This does **not** prove the sensor ADC itself is 10-bit. It proves only the numerical sample domain present in these DNG files.

## Critical payload-population finding

Although both files declare a full `3840x2160` active raster, the pixel payload does not populate all 2160 rows.

For both frames:

- rows `0..1839`: image/sample data present
- rows `1840..2159`: exact zero
- exact all-zero row count: `320`
- exact zero-tail bytes: `2,457,600`
- populated-raster prefix: `3840x1840`
- populated-prefix bytes: `14,131,200`
- zero rows are exactly `320 / 2160 = 14.8148148%` of declared height

The DNG metadata simultaneously declares:
- ActiveArea = full `3840x2160`
- DefaultCropSize = `3840x2160`

Therefore the zero tail is **not described by the DNG metadata as an intentional inactive/cropped region**.

The first all-zero raster row is exactly row 1840.

### Prefix identities

Frame 0 populated-prefix SHA-256:
`b96ca21ba08875b8a36fe6c3a95b69ce6553389bae68f8ee9a5feba8ff6ad2e7`

Frame 22 populated-prefix SHA-256:
`9d38584db1510670562a7fdacbc713710ecd6f2bbe44f4c90fe8b88af8641b23`

The populated prefixes differ, proving these are distinct frame payloads rather than identical cached pixel data.

The `3840x320x2` zero-tail SHA-256 is identical in both:
`54f980b5b3be8ce80cb6490c527e38d681deade50f239f2cb7d23cf9d0108c37`

## Black / white metadata

Both files report a 2x2 black-level pattern equivalent to approximately:

- 64.00
- 63.75
- 63.75
- 63.50

WhiteLevel = `1023`.

The zero tail therefore lies far below the declared per-plane black offsets and must not be interpreted as measured dark scene signal.

## Per-frame dynamic metadata

Most camera/DNG metadata is identical between the two frames.

Expected frame-dependent differences include:
- EXIF sub-second capture time;
- DNG TimeCodes (`0` versus frame-code `22`);
- pixel payload;
- OpcodeList2 content.

`OpcodeList2` is 3908 bytes in both files but has a different SHA-256 per frame:
- frame 0: `d951f14a111623d73d1a1c758c62a59d2e0fbed60fffbf1369aaa2ee625eade4`
- frame 22: `476773a6312a93d43748dd71b9e6886d0152b5ceb8708f865bb158fc979d8f47`

The list begins with four DNG opcodes and opcode ID 9, consistent with DNG GainMap-style stage-2 metadata. This is container processing metadata, not extra independent scene evidence.

`OpcodeList3` is identical across both frames:
`48441d2e0e4693a0a22a88c5e26401baa1f22d9119eb0f54117c13fc731f710f`

## Bounded classification

For these two mcpro24fps DNG frames, the measured file topology is:

`APP_VISIBLE_3840x1840_CFA_PAYLOAD_EMBEDDED_IN_3840x2160_DNG_RASTER`

This classification is about the **DNG files as written**, not about the physical sensor.

It does not yet establish whether the missing 320 rows arise from:
- the Camera2/HAL producer;
- mcpro24fps's RAW-video buffer handling;
- its DNG writer;
- a stride/height contract mismatch;
- an upstream crop/padding convention not represented in the written DNG tags;
- another stage.

## Importance for TruthRaw

This is a second independent software path, separate from TruthRaw's own Camera2 probes, showing that a nominal high-capability camera path can expose a larger raster/container than the actually populated sample domain.

The next useful experiment is source-bound:

1. obtain several more consecutive DNG frames from the same mcpro24fps RAW-video sequence;
2. verify whether the exact `1840 + 320 zero rows` topology is invariant;
3. compare another resolution/frame-rate profile;
4. if mcpro24fps can export/log the underlying Camera2 Image format/rowStride/height, compare that producer geometry directly with the DNG writer output;
5. never promote the 3840x2160 DNG envelope itself to 3840x2160 measured CFA evidence without a population audit.

Permanent law:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**


## Extended consecutive-frame validation: frames 33 through 42

Ten additional consecutive DNG frames from the same mcpro24fps RAW-video sequence were audited read-only.

Files:
- `00000033`
- `00000034`
- `00000035`
- `00000036`
- `00000037`
- `00000038`
- `00000039`
- `00000040`
- `00000041`
- `00000042`

Every file is exactly `16,605,437` bytes and declares the same `3840x2160` uncompressed 16-bit CFA raster with WhiteLevel `1023`.

### Exact topology invariant

All ten frames independently have:

- non-zero/image-bearing rows: exactly `0..1839`
- all-zero rows: exactly `1840..2159`
- non-zero row count: `1840`
- zero row count: `320`
- last non-zero row: `1839`
- first all-zero row: `1840`
- populated prefix bytes: `14,131,200`
- zero tail bytes: `2,457,600`
- zero-tail SHA-256: `54f980b5b3be8ce80cb6490c527e38d681deade50f239f2cb7d23cf9d0108c37`

The zero-tail hash is byte-identical across all ten frames and matches the previously audited frames 0 and 22.

Therefore the same payload boundary has now been observed in **12 distinct frames** from this sequence.

### Distinct live frame payloads

All ten full-file SHA-256 values are unique.
All ten populated-prefix SHA-256 values are also unique.

Consecutive populated prefixes 33→42 differ strongly:
- approximately `98.58%..98.60%` of sample positions change between adjacent frames;
- mean absolute code difference is approximately `23.1..23.3` codes;
- median absolute code difference is `19` codes.

This excludes an identical/cached populated payload explanation for the repeated geometry boundary.

### Numerical code-domain observations

Per-frame maxima for frames 33..42:
`526, 456, 518, 543, 598, 540, 477, 581, 584, 425`.

All remain below DNG WhiteLevel `1023`.

The bitwise OR across each complete frame is either `0x1ff` or `0x3ff`, consistent with these frames occupying no more than the lower ten code bits. This remains a file-domain observation only and is not promoted to physical ADC bit-depth evidence.

### Time-code continuity

The DNG TimeCodes for frames 33..42 carry the matching frame-number BCD sequence:
`33,34,35,36,37,38,39,40,41,42`.

This strengthens the interpretation that the audited files are consecutive members of the same RAW-video sequence rather than arbitrary exports.

### Opcode behavior

`OpcodeList3` is byte-identical across all ten added frames.

`OpcodeList2` is not constant across the sequence; it changes in groups of frames. This confirms that some DNG processing metadata is dynamic while the `3840x1840 + 320 zero rows` payload topology remains invariant.

## Strengthened bounded conclusion

Across 12 independently differing mcpro24fps DNG frames from one 60-fps tele RAW-video sequence, the measured raster topology is invariant:

`DECLARED_3840x2160_U16_CFA__POPULATED_3840x1840__EXACT_ZERO_ROWS_1840_TO_2159`

This makes an accidental single-frame write failure unlikely.

It still does not identify the stage that created the truncation. The remaining hypotheses must stay separate:
- Camera2/HAL producer surface is itself 3840x1840;
- source buffer is larger but mcpro24fps copies only 1840 rows;
- DNG writer declares 2160 while consuming a 1840-row producer;
- row/height/stride metadata disagreement occurs upstream;
- another private producer convention is involved.

A direct Camera2 Image width/height/rowStride/plane-byte observation from the mcpro24fps source path would discriminate these possibilities.
