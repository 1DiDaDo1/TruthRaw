# Fact check — Professional RAW Decoder Adapter v0.1

## Proven by code/tests

- External lossless Bayer output cannot enter Direct-CFA admission unless source sealing/binding and decoded-sample equivalence are explicitly verified.
- Successful decoding alone is insufficient.
- External certified lossless Bayer output is classified separately from native strict-DNG output.
- X-Trans is not silently re-labeled Bayer.
- computational RAW remains derived.
- multi-shot composite remains a multi-capture class.
- decoder memory admission is orthogonal to scientific classification.
- native strict-DNG bridging compiles against the real `TileNativeDngSource v0.1` header/API.

## Not proven

- no Canon/Nikon/Sony/Fujifilm/Hasselblad/Phase One codec implementation is included;
- no real professional-camera RAW corpus has been decoded by this module yet;
- no per-model firmware/codec certification is claimed;
- no X-Trans/Foveon reconstruction backend is provided;
- no Android device memory benchmark is implied.
