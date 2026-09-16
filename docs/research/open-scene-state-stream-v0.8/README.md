# TruthRaw streamed full-frame Open Scene State v0.8

Status: **RESEARCH / FULL-FRAME AUTHORITY SIDECAR / NO MAIN PROMOTION**

Date: 2026-09-16

v0.8 aggregates validated Open Scene Region v0.7 decisions into one full-frame authority sidecar without changing the Scientific Master or creating new evidence.

## Core purpose

Open Scene Region v0.7 binds light, colour, structure, HDR and restoration authority at region level. v0.8 makes that state full-frame and hardware-independent.

Input spans must be contiguous in global raster order. Adjacent spans carrying the same scientific region-state hash are canonically coalesced before hashing. Therefore a different execution chunk size does not change the full-frame scientific identity as long as the same scientific state sequence is represented.

This preserves the long-standing TruthRaw rule:

> **Compute resources may change execution strategy, never truth authority.**

## Bound identity

The full-frame state binds:

- source evidence SHA-256;
- Scientific Master SHA-256;
- Dynamic Authority artifact SHA-256;
- frame id and dimensions;
- exactly one physical frame;
- exactly one independent evidence item.

Every admitted v0.7 region must match all three identities.

## Fail-closed admission

A region is rejected if:

- `pass_contract=false`;
- source/master/Dynamic-Authority identity mismatches;
- evidence count differs from one/one;
- the region claims to create new evidence;
- detail or restoration claims scientific writeback;
- any restoration decision is blocked.

This last guard is deliberately duplicated even though v0.7 normally sets `pass_contract=false` for blocked restoration. A forged/inconsistent v0.7 object therefore still cannot enter the full-frame state.

## Canonical streaming identity

The writer retains only one pending scientific run plus summary counters. It does not need a full-frame authority object in RAM.

Canonical content hashing uses coalesced runs:

`start + length + region_scene_state_sha256`

Splitting one run into 32-pixel, 256-pixel or 4096-pixel execution chunks produces the same content SHA-256.

A genuine scientific state change creates a new run and changes the content identity.

## Full-frame summaries

v0.8 summarizes:

- per-channel Dynamic Authority counts;
- per-channel HDR status counts;
- colour-calibration authority by pixel;
- illumination authority by pixel;
- detail/structure status by pixel;
- independently-held-out physical-colour claim count;
- counterfactual-illumination pixel count.

The summary explicitly fixes:

- `creates_new_evidence=false`;
- `scientific_master_writeback_allowed=false`;
- `physical_frame_count=1`;
- `independent_evidence_count=1`.

## Files

- runtime: `tools/open_scene_state_stream_v08.py`
- tests: `tests/test_open_scene_state_stream_v08.py`

## Boundary

v0.8 is an authority/provenance sidecar only. It does not reconstruct pixels, perform colour conversion, sharpen detail, relight a scene, fill missing content, infer HDR radiance, or alter the Scientific Master.
