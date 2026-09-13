# TruthRaw

TruthRaw is a single-frame RAW reconstruction research project and software-ISP built around one rule:

> **Measured where measured. Reconstructed where necessary. Never invented.**
>
> **Echt gemeten. Echt gereconstrueerd. Geen verzinsels.**

## Start here

For the current project state and reading order, use:

1. `START_HERE_NEW_CHAT.md`
2. `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md`
3. `state/CURRENT_CANONICAL_STATE_2026-09-10.json`
4. `docs/DOCUMENT_STATUS_INDEX_2026-09-10.md`

Dated state files and research README files are preserved as provenance/history. They are **not** global current-state authorities unless the current document-status index explicitly says so.

## Scientific foundation

The original RAW/CFA and capture metadata are the **sealed original house**: immutable measurement evidence.

TruthRaw constructs a separate scientific Scene Master — the **new house**. Its numerical representation is not forced to inherit RAW10 range, source WhiteLevel as an output ceiling, source ISO as working scale, SDR range, integer storage, or DNG container limits.

That representational freedom never enlarges the evidence:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

Measured, reconstructed, censored/unknown, counterfactual and appearance quantities remain distinguishable. A derived LinearRaw/DNG is an export or compatibility projection, not a relabelling of reconstructed values as original sensor measurements.

## TruthRange and zero-line

For positive physical scene light TruthRaw may use:

`T = log2(L/L0)`

`L0` is the chosen zero-line reference. `T=0` is not sensor black, DNG `BlackLevel`, display black, clipping or “zero photons”.

The TruthRange coordinate may be unbounded as a representation, while every real capture supplies only finite evidence. Signed scene-linear estimates remain a separate companion representation; a negative numerical estimate is not negative physical light.

## The renewed house

TruthRaw separates scientific authority from execution resources.

The Building Runtime defines 12 logical rooms. A room's truth floor determines what it may read, claim or modify. RAM, CPU, GPU and thermal state determine only execution resources.

A faster phone may open more compatible rooms in parallel, retain more rebuildable caches or use larger tiles. It does **not** receive more evidence or permission to make stronger scientific claims.

Corridors carry compact handles, provenance and authority, not duplicate full-frame images.

## Preview architecture

TruthRaw does **not** define preview by one file/container format.

The later finalized preview architecture separates:

- Scientific Master — camera-native reconstructed scientific RGB;
- Finalized Scientific Preview — derivative bound to source/master/zero-line/Backplane identity;
- runtime preview representation — bounded `ARGB_8888/sRGB`;
- portable compatibility preview — baseline JPEG/sRGB;
- optional HDR display derivative;
- embedded DNG JPEG preview — compatibility transport only.

The same preview pixels may be represented as a live bitmap, standalone JPEG or embedded DNG preview without any of those encodings becoming scientific evidence.

The older v0.7 `IFD0 + LinearRaw SubIFD + JPEG preview SubIFD` layout remains useful interoperability evidence, not the definition of the modern preview architecture.

## Permanent boundaries

- original CFA/sample bytes and capture metadata remain immutable;
- physical frame count and independent evidence count remain one for the single-frame master;
- WhiteLevel clipping is censored evidence, not exact latent radiance;
- GainMap is applied exactly once where the canonical chain requires it;
- appearance never modifies the scientific master;
- counterfactual simulations never become evidence for the captured world;
- virtual EV/ISO views never become independent measurements;
- no generative/semantic texture enters scientific evidence;
- APK/GCam/computational-RAW content does not determine TruthRaw;
- `canonical/ptc/v1.1` means **Pure Truth Certificate**, not photon-transfer calibration;
- FULL_PHYSICAL remains blocked where independent sensor/color/illuminant/optics calibration is missing;
- no open-source LICENSE is added unless explicitly chosen.

## Active branch-local RGB / LinearRaw restoration candidate — 2026-09-13

Branch:

`research/restore-rgb-linearraw-output-v0.2-workbase-2026-09-13`

Read:

`docs/research/linear-dng-projection-v0.2/README.md`

Preview restoration source:

`docs/research/linear-dng-projection-v0.2/evidence/MODERN_FINALIZED_PREVIEW_ARCHITECTURE_2026-09-13.md`

The branch restores finite RGB LinearRaw headroom (`2x`, `BaselineExposure=+1 EV`) and source camera identity on top of the finalized Scientific Preview / bounded-streaming route.

Host validation and the arm64 Android v0.2 build are green. Exact built APK identity before embedded-preview integration:

- version: `0.6-linear-dng-restore`
- bytes: `4335889`
- SHA-256: `93e51245e0950c5c3140b83f2f3429d2f52ad48adcd5630409134e4c430b5654`

Still open are physical Honor execution, Lightroom interoperability, independent validation of a newly produced DNG, and embedding the **modern finalized JPEG representation** in DNG without altering the RGB LinearRaw payload.

The exact historical phase label remembered as approximately `1.3` / `1.4` is not assigned until an exact source proves it.

## Repository navigation

Use `docs/DOCUMENT_STATUS_INDEX_2026-09-10.md` before treating any dated README, report, audit or state file as current.

Historical material is intentionally retained to preserve failures, rejected candidates, validation context and scientific provenance.
