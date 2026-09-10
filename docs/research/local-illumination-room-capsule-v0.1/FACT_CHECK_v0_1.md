# Fact check — Local Illumination Room / Room Capsule v0.1

1. **Local domain is sufficient for many photographic lighting edits.** A photographer often needs to light the subject/room, not model the entire environment. TruthRaw can therefore restrict expensive geometric state to a chosen ROI while preserving the global Scene Master.
2. **This saves both persistent storage and peak RAM.** Geometry is lower-resolution and ROI-only; rendering is tile-based. The output itself remains full resolution.
3. **A room capsule must not pretend the rest of the world does not illuminate the room.** v0.1 therefore includes a Boundary Illumination Envelope rather than a full external 3D environment.
4. **Single-image physical relighting is underdetermined without intrinsic material/geometry/illumination information.** v0.1 is therefore appearance-relative, not a physical-truth claim.
5. **Light-state descriptors are cheap.** Multiple sun/night/art-light variants reuse one capsule rather than storing multiple images.
6. **Android compatibility requires a CPU baseline and bounded allocations.** GPU use is optional because Vulkan support/capability varies; the planner takes an app memory-class budget and low-RAM flag and coarsens geometry before exceeding it.
7. **CICM relation.** Room Capsule is a spatially sparse front-end to CICM-style counterfactual worlds. It changes the hypothetical world, not the evidence ledger.
8. **Best Conditioning remains separate.** Room-light multipliers cannot be reinterpreted as Best Conditioning EV or as additional sensor evidence.
9. **Fail-closed means non-applicable output.** A blocked physical-relight request must return an invalid result, not a valid appearance result plus a warning status; otherwise a careless caller could still apply it.
10. **Explicit memory ceilings are hard ceilings.** If the app gives Room Capsule less than the minimum safe workspace, the correct result is `BUDGET_TOO_SMALL`, never silently allocating more.
