# TruthRaw Illumination Room v0.2 — report

Status: **RESEARCH_CANDIDATE_CI_PASS**

Branch: `research/illumination-room-v0.2-2026-09-11`

Base `main`: `514f2f4bde6aba5a6709e176c03b22c3b9aea912`

Initial validated candidate head: `59fa38ca4706e9c1b0c1b0c05e3499b4e7956c29`

GitHub Actions run: `34546376103` — **SUCCESS**

## What passed

1. CICM v1 upstream integrity.
2. Local Illumination Room / Room Capsule v0.1 upstream integrity.
3. Room ABI v0.1 upstream integrity.
4. Technical Backplane v0.1 upstream integrity.
5. GCC Release build + test.
6. Clang Release build + test.
7. Clang ASan + UBSan build + test.

## Implemented v0.2 contracts

- Building Runtime `ResourcePolicy` is authoritative for room budget and tile size.
- The Illumination Room does not derive a second independent memory-class policy.
- Existing Room ABI v0.1 leases own the concrete packed-geometry, vector-boundary and tile-workspace allocations.
- No hidden full-frame image allocation is introduced.
- Technical Backplane source/master/zero-line/scene-scale identities are converted into the CICM `SceneBinding` only after backplane validation.
- Missing/corrupt lineage identity fails closed.
- Relative day/night/custom scenario labels are explicit counterfactual semantics.
- Relative EV is delegated to CICM `RELATIVE_RADIANCE_SCALE_ONLY`.
- Physical SNR remains unavailable on the relative path.
- The v0.1 local normal/visibility/confidence light solver is reused rather than forked.
- The same local sample/light input yields the same lighting equation result independent of low/high resource plan.
- `CalibratedIntrinsicRelightReserved` remains fail-closed.
- Counterfactual light states cannot increase evidence count, modify the scientific master, or modify the TruthRange zero line.

## Mobile adaptation exercised by tests

The test constructs a 16320x12288 local-domain planning case with two runtime policies:

- low tier: 32 MiB total working-set budget, 8 MiB heavy-room budget, tile 128;
- high tier: 256 MiB total working-set budget, 64 MiB heavy-room budget, tile 512, optional Vulkan preference.

The low tier must choose a geometry representation no finer than its budget permits; the high tier may retain finer geometry. Both execute the same counterfactual lighting equations.

This is a resource/fidelity adaptation, not a change in truth authority. Full rendered appearance may still differ when the retained geometry resolution differs; such differences must remain inside the counterfactual/appearance domain.

## Scientific boundary

This candidate does **not** establish calibrated physical sun, moon, sky spectrum, intrinsic reflectance, BRDF, global illumination or exact new shadows from one frame.

It also does not convert relative EV into a physical ISO/noise claim. Physical electron/SNR capture prediction remains restricted to the separately calibrated CICM forward path.

The sealed source, scientific Scene Master and global TruthRange zero-line remain immutable.

## Promotion boundary

`RESEARCH_CANDIDATE_CI_PASS` means the implementation and tested contracts passed host CI. It does not make the module canonical, does not prove on-device Android memory/RSS/thermal behavior, and does not close FULL_PHYSICAL blockers.

The next architectural use is as an input contract for Room ABI v0.2 / Adaptive All-Room Binding.
