# TruthRaw Open Scene Region Runtime v0.7

Status: **RESEARCH / AUTHORITY INTEGRATION / NO MAIN PROMOTION**

Date: 2026-09-16

v0.7 is the first explicit region-level contract that combines the existing Dynamic Authority field and Structure Evidence runtime with the newer scene-physics, colour/calibration, scientific-HDR and conservation/restoration rules.

It is intentionally not a renderer and not a generative restoration engine.

## Purpose

Previous research had already separated:

- local radiometric authority (`MEASURED`, `CALIBRATED_ESTIMATE`, `RECONSTRUCTED`, `CENSORED`, `UNKNOWN`, ...);
- Structure Evidence / appearance-detail permission;
- source-bound versus stronger colour calibration;
- counterfactual illumination;
- restoration/loss-compensation authority.

v0.7 binds those decisions to one region identity so later rooms cannot silently mix them.

## Bound identity

Every region state binds:

- `source_evidence_sha256`;
- `scientific_master_sha256`;
- `dynamic_authority_artifact_sha256`;
- region id and raster dimensions;
- `physicalFrameCount = 1`;
- `independentEvidenceCount = 1`.

A region with a mismatched or malformed identity fails closed.

## Light / illumination

Illumination authority is kept separate from RGB authority:

- `UNKNOWN`;
- `SOURCE_BOUND_ESTIMATE`;
- `RECONSTRUCTED`;
- `INDEPENDENTLY_MEASURED`;
- `COUNTERFACTUAL`.

Counterfactual illumination can produce a useful hypothetical view, but `captured_world_writeback_allowed=false`.

## Colour

Colour-calibration authority is explicit:

- `UNKNOWN`;
- `SOURCE_METADATA_BOUND`;
- `INDEPENDENT_HELDOUT_VALIDATED`;
- `APPEARANCE_ONLY`.

A source/DNG matrix may be reproducible and source-bound without becoming independent physical calibration. `FULL_PHYSICAL`-style colour claims are enabled only by `INDEPENDENT_HELDOUT_VALIDATED` with bound calibration and holdout evidence hashes.

## Structure/detail

v0.7 consumes the existing `RuntimeStructureResult` from v0.4.

Important: even when v0.4 returns `ADMITTED_APPEARANCE_ONLY`, detail/acutance remains an appearance permission. It does not change reconstructed pixels into newly measured spatial detail.

If the uncertainty domain is out of range, v0.7 carries `OUT_OF_DOMAIN` forward rather than borrowing 4080x3072 support into another readout domain.

## Scientific HDR

Per-channel HDR status is derived from Dynamic Authority:

- measured/calibrated finite value -> `EVIDENCE_SUPPORTED_FINITE`;
- reconstructed finite value -> `RECONSTRUCTED_FINITE_NOT_MEASURED`;
- censored value -> `CENSORED_BOUND_ONLY`;
- unknown -> `NO_SCIENTIFIC_HEADROOM`;
- counterfactual/appearance -> non-scientific view only and prohibited from captured-world scene input.

No display/PQ/GainMap state is used to upgrade these categories.

## Restoration

The existing conservation/restoration authority guard is applied per channel.

Examples:

- valid measured support cannot be replaced by `LOSS_COMPENSATION_RECONSTRUCTED`;
- supported unknown loss may become `RECONSTRUCTED` when provenance-bound and retreatable;
- censored data cannot be converted into an exact restored scientific value;
- aesthetic reintegration has no scientific writeback.

## Why this matters

The project previously had the correct individual laws, but a later room could still theoretically confuse one domain with another. v0.7 creates a single cryptographically identifiable region decision that says simultaneously:

- what radiometric authority exists;
- what colour-calibration authority exists;
- whether illumination is captured/reconstructed/counterfactual;
- what structure/detail permission exists;
- what HDR headroom class exists;
- what restoration action is permitted.

This is the first direct implementation of the principle that light, colour, structure, HDR and restoration belong to one scene state while retaining independent authority axes.

## Files

- runtime: `tools/open_scene_region_runtime_v07.py`
- tests: `tests/test_open_scene_region_runtime_v07.py`

## Permanent boundary

v0.7 creates no new evidence and performs no source writeback. It cannot promote source metadata to physical calibration, censored values to exact radiance, appearance detail to measured structure, or restoration/counterfactual state to captured evidence.
