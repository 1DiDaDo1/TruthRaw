# Full-Frame Streaming v0.1 — Fact Check

## Verified from current v4.7i API/source

- `DecodedDngFrame` owns full-frame `raw` and optional full-frame `gainField`.
- `ProcessResult` owns full-frame `sdrRgb`, optional `stage2Diagnostic`, and half-resolution `halfLogGain`.
- v4.7i reconstruction/appearance already execute internally by tiles.
- one global exposure plan is chosen after whole-frame neutral/scene histograms exist.
- HDR gain uses half-resolution scene maxima and a one-cell dilation of censor state.
- canonical appearance output is not used to choose the exposure histogram; neutral RGB is.

## v0.1 translation

- raw/gain become a tile-source contract;
- halfSceneMax/censor are recomputed tile-locally in pass 2;
- SDR/half gain/diagnostic become sink channels;
- fixed-size histograms remain resident;
- global exposure remains the exact existing v4.7i public function;
- pass 2 recomputes a tile rather than retaining the first-pass RGB frame.

## Scientific invariants

- physical frame count remains 1;
- independent evidence count remains 1;
- GainMap multiplication remains exactly once in the Stage-2 path;
- source WhiteLevel censor status remains based on source RAW sample value;
- reconstructed channels remain reconstructed;
- appearance never feeds evidence or scientific-master state;
- zero-line is not stored per pixel and is not modified by this adapter.

## Deliberate limitations

- v0.1 requires an even tile core so each 2x2 half-gain cell has exactly one owner.
- v0.1 uses one worker; concurrent streaming is a later performance gate.
- source/sink implementations must truthfully report their resident-memory upper bounds.
- v0.1 does not implement a tile-native TIFF/DNG decoder; it defines the boundary that such a decoder must implement.
