# TruthRaw ↔ Adobe Lightroom real-HDR bridge v0.7

**Status:** RESEARCH — NOT MAIN-PROMOTED  
**Scope:** single-frame physical capture → open-ended TruthRaw scene → Adobe HDR presentation/editing  
**Forbidden shortcut:** exposure-stack semantics, fake HDR-Merge metadata, counterfactual illumination promoted as measured scene radiance

## Finding

Adobe Lightroom's current HDR editing path can be used without creating an HDR exposure stack.
Adobe explicitly documents that an HDR-editable image may be a **single-exposure raw file**. Enabling the HDR button changes the processing pipeline so output tones are no longer restricted to the normal SDR range. Lightroom's HDR histogram represents values above SDR white in one-stop / one-EV zones.

This means TruthRaw does **not** need to imitate Lightroom's `Merge to HDR` workflow to obtain access to Adobe's HDR controls.

The correct scientific interpretation is:

```text
one physical RAW/CFA frame
        ↓
TruthRaw measured/calibrated/reconstructed scene state
        ↓
open-ended scene-linear / TruthRange representation
        ↓
HDR presentation projection
        ↓
Adobe Lightroom HDR edit/output path
```

not:

```text
one frame
  ↓
fake exposures / fake illumination stack
  ↓
pretend HDR merge
```

## Two valid interoperability routes

### Route A — single-exposure RAW/DNG

A real single-exposure RAW/DNG can be opened in Lightroom and the user can enable **Edit in HDR mode**. Adobe then performs its own raw-development pipeline in HDR mode.

Advantages:
- no exposure merge;
- no fake relighting;
- sensor evidence remains one physical exposure;
- simplest route for immediate Lightroom testing.

Limitation:
- this route gives Adobe the RAW mosaic, not TruthRaw's already reconstructed RGB Scene Master or Dynamic Authority Field. Adobe controls demosaic, color and tone development after import.

Therefore Route A is an Adobe-RAW workflow, not a lossless import of the TruthRaw Scientific Master.

### Route B — TruthRaw Scene Master rendered HDR exchange

TruthRaw can project its scientifically admitted Scene Master to a rendered HDR exchange image and hand that result to Lightroom. Candidate Adobe HDR formats include AVIF, JPEG XL, TIFF, PSD and PNG; JPEG can carry HDR via a Gain Map.

Advantages:
- TruthRaw controls reconstruction/uncertainty/censoring before Adobe sees the image;
- dynamic range can come from the real scene representation rather than bracketed exposures;
- Adobe can be used as the finishing/presentation editor.

Limitation:
- rendered RGB is no longer a raw sensor mosaic;
- color-space/transfer-function/HDR metadata and real Adobe ingest behavior must be validated before production use;
- v0.7 defines the contract but does not yet implement an AVIF/JXL/TIFF HDR encoder.

A rendered Scene Master is therefore explicitly forbidden from masquerading as a raw DNG in v0.7.

## HDR headroom from real scene data

The v0.7 code derives two quantities relative to a chosen SDR-white scene reference `L_SDR`:

```text
nominal_headroom_EV = max(0, log2(L_max / L_SDR))
```

and a conservative uncertainty-aware form:

```text
supported_headroom_EV = max(0, log2((L_max - p95) / L_SDR))
```

where the second expression is evaluated only for positive values.

Only the following authorities may contribute scene radiance:
- `MEASURED`
- `CALIBRATED_ESTIMATE`
- `RECONSTRUCTED`

`COUNTERFACTUAL` and `APPEARANCE_ONLY` are rejected from the scientific HDR signal.

`UNKNOWN` contributes no HDR headroom.

A `CENSORED` highlight preserves the fact that the real value lies beyond a bound, but it does **not** provide the missing exact radiance. It therefore cannot be converted into invented bright HDR pixels merely to fill Lightroom's HDR range.

## Lightroom HDR Limit is not a Scientific Master limit

TruthRaw keeps:

```text
scientific_master_fixed_hdr_limit_ev = null
```

Adobe's HDR Limit/headroom controls belong to the presentation/editing layer. They may decide how much of an open-ended scene is mapped into a particular display/export, but they do not redefine what the sensor measured or what the Scene Master contains.

This is the same separation as:

```text
open-ended scene representation
        !=
finite current display headroom
```

## Gain Map classification

Adobe documents Gain Maps as an adaptive display mechanism that stores a base rendition plus a spatial gain relationship between SDR and HDR renditions. Adobe also notes that Gain Maps are standardized in ISO 21496-1.

For TruthRaw, a Gain Map is therefore classified as:

```text
PRESENTATION / EXPORT METADATA
```

and never as:

```text
new sensor evidence
new Scientific Master radiance
new independent exposure
```

Adobe-generated gain-map data may not write authority back into the Scientific Master.

## Automatic HDR activation

Adobe documents an application preference to enable HDR editing by default for HDR photos, but the explicitly auto-recognized file classes are narrower than the general list of files that can be manually edited in HDR. A generic single-exposure RAW is documented as usable in HDR mode, but Adobe does not document a public metadata flag that TruthRaw should forge in order to make every arbitrary DNG auto-open as HDR.

Therefore v0.7 policy is:

1. **Do not spoof `Merge to HDR` metadata.**
2. For a raw DNG, use the real single-exposure file and enable Lightroom HDR normally.
3. For automatic HDR recognition, prefer a genuinely HDR-encoded rendered exchange file once that encoder is validated.

## HONOR / Android status

Adobe's Android documentation is currently not fully consistent.

- An Adobe Android HDR page updated in February 2026 lists specific Pixel and Samsung families and says other Android devices may expose HDR controls while only showing SDR output.
- A newer Adobe mobile HDR page from June 2026 says Android HDR capture/editing also applies to other devices meeting minimum requirements such as Android 14+, ARM8, at least 3 GB RAM and DNG capture support.
- Adobe's current device lists still do not explicitly name the HONOR BKQ-N49 / Magic 8 Pro.

Therefore **native HDR display on the HONOR must be measured in Lightroom itself rather than inferred from hardware capability**. The HDR edit control may be present even when Adobe does not expose full HDR display headroom.

Runtime acceptance test for the phone:

```text
1. Import a real single-exposure tele DNG.
2. Open Edit > Light > Edit in HDR mode.
3. Check whether the HDR histogram contains a usable HDR section.
4. Check Adobe's in-app device-compatibility indicator.
5. Verify whether pixels above SDR white are displayed rather than merely marked red/out-of-display.
6. Export AVIF/JPEG HDR and inspect the result on a second known-HDR viewer/display.
```

Failure of the HONOR display test does not invalidate the HDR scene data; it only means that device is acting as an SDR preview/editor for an HDR-capable file.

## Adobe source basis checked 2026-09-15

Primary official references used for this research:

- Adobe Lightroom — HDR Optimization: https://helpx.adobe.com/lightroom/desktop/edit-photos/hdr-output.html
- Adobe Lightroom Classic — Edit and Export in HDR: https://helpx.adobe.com/lightroom-classic/desktop/process-and-develop-photos/hdr-output.html
- Adobe Camera Raw — HDR Optimization: https://helpx.adobe.com/camera-raw/using/hdr-output.html
- Adobe Camera Raw — Gain Map: https://helpx.adobe.com/camera-raw/desktop/hdr-and-advanced-output/gain-map.html
- Adobe Lightroom Mobile — Edit HDR photos: https://helpx.adobe.com/lightroom/mobile/adjust-light-and-color/edit-hdr-photos.html
- Adobe Lightroom Mobile — Export HDR photos: https://helpx.adobe.com/lightroom/mobile/share-save-and-export/export-hdr-photos.html

## v0.7 implementation

Code:
`tools/adobe_hdr_interop_v07.py`

Tests:
`tests/test_adobe_hdr_interop_v07.py`

The implementation fails closed on:
- multiple physical source frames;
- exposure-merge mode;
- fake HDR-merge metadata;
- counterfactual radiance entering the scientific HDR signal;
- appearance-only radiance entering the scientific HDR signal;
- rendered Scene Master pretending to be raw DNG.

## Next gate

The next useful experiment is not another synthetic HDR algorithm. It is a real Adobe interoperability round-trip:

1. feed one existing HONOR tele DNG to Lightroom HDR mode;
2. record Adobe's actual HDR headroom/histogram behavior;
3. compare that with TruthRaw's scene-linear/TruthRange and censoring information;
4. export a real HDR AVIF/JXL/TIFF or gain-map JPEG;
5. prove that display adaptation does not alter Scientific Master authority;
6. only then implement a production `TruthRaw Scene Master -> Adobe HDR exchange` encoder.
