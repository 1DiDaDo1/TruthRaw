# D.RAW Universal Capture / Intake v0.2.0

Status: **UNIVERSAL SOURCE + FRONTSIDE INSPECTION CANDIDATE**

The app no longer requires a device-specific scientific map or a hand-entered
target to understand an input.

## One universal entrance

```text
unknown RAW/DNG/image source
        |
        +--> immutable bytes + SHA-256
        |
        +--> source/container facts
        |    maker/model when present
        |    raster/CFA/storage
        |    exposure/ISO/f-number/focal length
        |    black/white level
        |    DNG color/noise metadata
        |
        +--> frontside scene inspection
             visible proportions
             structural content bounds
             luminance structure
             edges / dominant orientations
             natural-feature readiness
        |
        v
capability + scene route
```

Unknown facts remain UNKNOWN.

## Frontside inspection

D.RAW now tries to decode a visible representation directly from the source or
from an embedded DNG JPEG preview. This is analogous to looking at the front of
a photograph rather than only reading its container metadata.

The v0.2.0 structural vision layer measures:

- visible aspect ratio and proportions;
- structural-content bounding box and centroid;
- luminance distribution and entropy;
- edge density and gradient strength;
- dominant edge orientations;
- a deterministic structural appearance signature;
- whether enough visible structure exists for later natural-feature matching.

This layer is `APPEARANCE_DERIVED_ONLY`. It can guide reconstruction,
orientation and geometry routing, but it cannot upgrade sensor/source evidence.

Higher-level semantic vision models may be added as versioned successors
without rewriting the original source or this historical result.

## No mandatory calibration target

An indexed target is not required for relative scene geometry.

If the scene itself contains enough natural structure, D.RAW may route the
source pair toward natural-feature geometry.

Absolute metric scale still requires actual scale evidence somewhere in the
source or scene. It is never invented.

## Device independence

MAIN, ULTRA_WIDE and similar names in the capture UI are workflow labels only.
They are not scientific authority.

Focal length is read from the source where present, but focal length alone does
not prove lens role or field of view across different sensor formats.

Camera2 remains optional.

## Authority law

- source bytes / SHA-256: measured;
- DNG/TIFF metadata: source-metadata-bound;
- frontside scene inspection: appearance-derived;
- lens role: UNKNOWN unless independently established;
- relative geometry: not admitted until later validation;
- absolute metric scale: UNKNOWN without scale evidence;
- fusion: not granted.

**Seal the evidence, not the thinking.**
