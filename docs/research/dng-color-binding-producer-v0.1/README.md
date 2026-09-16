# DNG Color Binding Producer v0.1

Status: RESEARCH CANDIDATE — 2026-09-11

Base: `research/scientific-preview-source-binding-v0.1-2026-09-11` after the DNG color-transform study note.

## Purpose

Turn **source-bound DNG metadata** into the `ScientificColorBindingRecord` required by Scientific Preview Source Binding v0.1, without using the old Android identity/sentinel preview binding.

The producer is upstream of Main House color admission. It does not modify v4.7i, the 180-byte Technical Backplane, the Scientific Master, zero-line, or evidence counts.

## v0.1 flow

```text
sealed exact DNG bytes
      |
      +--> SHA-256 reverify (bounded)
      |
      +--> parse primary camera profile in IFD0
      |      - ColorMatrix1
      |      - AsShotNeutral
      |      - AnalogBalance (optional)
      |      - CameraCalibration1 (optional/signature governed)
      |      - ForwardMatrix1 (optional)
      |
      +--> DNG-specified 3-channel transform
      |      ForwardMatrix route OR ColorMatrix + Bradford route
      |
      +--> SHA-256 reverify again
      |
      +--> SOURCE_METADATA_BOUND ScientificColorBindingRecord
      |
      +--> existing Scientific Preview admission
      |
      +--> TileNativeDngSource / Main House reconstructed-color preview
```

## Scientific boundary

`SOURCE_METADATA_BOUND` means the transform is derived from metadata sealed inside the same source DNG. It is appropriate for a source-faithful color preview, but it is **not** independent camera/lens calibration and is not `FULL_PHYSICAL` color truth.

Camera-color metadata is not treated as lens calibration. No lens shading, spectral lens transmission, flare, PSF/MTF, CA or target-based lens calibration is invented here.

## DNG rules implemented

The implementation follows the DNG 1.7.1 color model for the deliberately narrow single-calibration / three-channel case:

- `XYZtoCamera = AB * CC * CM`.
- CameraCalibration is applied only when CameraCalibrationSignature equals ProfileCalibrationSignature; otherwise identity is used.
- If `ForwardMatrix1` is present, v0.1 uses the DNG forward-matrix formulation to obtain CameraToXYZ_D50.
- Without ForwardMatrix, the 3x3 XYZ-to-camera matrix is inverted, the `AsShotNeutral` is mapped back to a source white, and a linear Bradford adaptation maps that white to D50.
- `AnalogBalance` defaults to identity when absent.

## Deliberate fail-closed subset

v0.1 rejects rather than approximates:

- a second or third color calibration set;
- BigTIFF;
- missing ColorMatrix1;
- missing AsShotNeutral;
- non-3x3 matrix cardinality;
- wrong matrix/vector TIFF types;
- singular/non-finite matrices;
- source mutation before or during metadata parsing.

Dual/triple-illuminant interpolation is a future version. DNG 1.2+ requires interpolation based on selected white balance, so v0.1 must not silently choose ColorMatrix1 when ColorMatrix2/3 is present.

## Resource policy

The metadata parser scans IFD0 entry-by-entry and retains only a fixed set of relevant tag references. It does not materialize the RAW image or a whole TIFF directory. Source SHA-256 verification uses the existing bounded 64 KiB hashing workspace; parser metadata workspace is small and megapixel-independent.

The full-file SHA-256 passes are CPU/I/O work, but not full-file RAM ownership. This is acceptable for the research proof and preserves the cheap-phone memory invariant. Later optimization may safely reuse an already authenticated immutable file handle only if the same source-identity guarantee is preserved.

## Required validation

- contract gate;
- GCC Release;
- Clang Release;
- Clang ASan+UBSan;
- ForwardMatrix path;
- ColorMatrix + Bradford path;
- CameraCalibration signature match/mismatch behavior;
- dual-calibration fail-closed;
- missing-neutral fail-closed;
- singular-matrix fail-closed;
- source-tamper rejection;
- producer record accepted by the existing Scientific Preview admission with frame/evidence `1/1`.

## Still open after v0.1

- dual- and triple-illuminant interpolation;
- custom IlluminantData processing;
- `AsShotWhiteXY` alternative path;
- extra camera-profile IFD selection;
- 4+ color-plane cameras / ReductionMatrix path;
- physical-device Android integration;
- real MotionCam/Honor DNG validation;
- independent camera/lens target calibration.
