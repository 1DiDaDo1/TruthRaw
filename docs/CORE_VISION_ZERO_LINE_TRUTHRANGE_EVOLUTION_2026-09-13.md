# Zero-Line / TruthRange Evolution — recovered and clarified 2026-09-13

Purpose: preserve the recovered historical development of the zero-line without rewriting the older documents that were created at each stage.

This is a genealogy/history document. The current semantic authority remains the core zero-line architecture plus the current project map/state.

## 1. Earliest recovered formulation

The earliest currently recovered screenshot evidence is around 23:49 in the original discussion. It already states the essential mathematical idea:

`T = log2(L / L0)`

with `L0` as a **reference/gauge**, not a black point.

Consequences:

- `T = 0` means `L = L0`;
- `L -> infinity` gives `T -> +infinity`;
- `L -> 0+` gives `T -> -infinity`;
- multiplying all positive scene-light values by the same factor translates the coordinate unless the gauge is scaled consistently.

This is the conceptual birth of the unbounded scene address space.

The recovered screenshot is evidence that the idea existed by that point. It is not proof that no still-earlier formulation existed.

## 2. Sensor range was separated from representation range

Around 23:50 the discussion explicitly retained the physical sensor terms:

- black offset;
- gain/readout;
- exposure;
- clipping;
- noise.

Those quantities remain necessary to interpret where source measurements and uncertainty belong.

The new idea was not to remove finite sensor dynamic range. It was to stop using the finite source container/sensor limits as the numerical boundary of the reconstructed scene representation.

Canonical interpretation:

**The sensor constrains evidence. It does not have to constrain the coordinate address space used to represent the inferred scene.**

## 3. The house vision

Around 23:55 the architectural metaphor became explicit: the reconstructed/new house should have a zero-line/reference, with the coordinate extending conceptually upward toward light and downward toward darkness without inheriting RAW10/WhiteLevel as a hard representational ceiling/floor.

This is an architectural visualization of the log-ratio coordinate, not a claim of infinite measured photons.

## 4. Clipping became censoring, not a scene endpoint

Around 00:31-00:35 the project tested a crucial implication: saturation must not be interpreted as an exact latent light value.

In the discussed ISO400-12800 sequence, source clipping mapped to roughly +4.6 stops above the selected internal zero-line after the relevant normalization. Once a source sample saturated, the correct interpretation was conceptually:

`T >= approximately +4.6 EV`

not:

`T = +4.6 EV and the scene ends here`.

The exact threshold is source/calibration dependent. The important general rule is the inequality/censoring semantics.

This is one of the most important scientific guards in TruthRange: an unbounded coordinate does not recover hidden values from a clipped sample; it merely allows the unknown latent value to remain above a finite evidence bound.

## 5. Multiplicative scale ambiguity

Around 00:37 the discussion explicitly connected the zero-line with the free multiplicative scale of radiance reconstruction.

If a reconstructed scene is only known up to a common positive factor, selecting/binding `L0` removes that coordinate ambiguity. The gauge fixes the coordinate origin; it does not add measurement information.

This distinction later became central to the self-gauge work.

## 6. TruthRange v0.1

By around 00:51, the project had a first practical TruthRange form and made the canonical clarification that “infinite dynamic range” referred to the **TruthRange address space**, not to infinite physical sensor information.

The recovered v0.1 result reported approximately 0.093 EV p50 spread over an ISO100-12800 sequence as a practical stability check.

However, the temporary implementation used an ISO x exposure-derived gauge. That gauge was explicitly treated as an implementation detail rather than the final canonical definition.

Therefore v0.1 had the correct architectural idea but not the final gauge independence.

## 7. TruthRange v0.2 — self gauge

v0.2 solved the main conceptual weakness of v0.1 by deriving the zero-line/gauge from the scene's own eligible uncensored Stage-2 evidence rather than requiring ISO/exposure inside the relative TruthRange formula.

Two modes emerged conceptually:

- `SELF_GAUGE` — calibration-free relative TruthRange within one scene;
- `COMMON/PHYSICAL_GAUGE` — optional mode when absolute/cross-scene comparability is supported by additional calibration.

A synthetic multiplicative invariance test scaled a complete master by x37 and observed only about `1.39e-7 EV` change after self-gauge normalization, i.e. near floating-point numerical precision for that test.

Important interpretation:

- this validates the intended **relative scale invariance** of that implementation/test;
- it does not establish absolute radiometric calibration;
- ISO and exposure remain capture provenance and can still matter to physical sensor/noise/forward models.

## 8. TruthRange v0.3 — explicit dense uncertainty

v0.3 strengthened the scientific contract by pairing scene coordinates with uncertainty/support roles.

The measured CFA role received source/NoiseProfile-bound uncertainty in the appropriate Stage-2 scale. Reconstructed/missing channels used conservative reconstruction/transport proxies rather than being silently treated as measured.

This made the measured/reconstructed distinction explicit at the per-sample scientific layer.

## 9. Later tiled/streaming evolution

A later engineering realization was that “dense scientific field” must not imply “all dense structs for the full image resident in RAM at the same time”.

The scalable route became:

- one coherent scene/master identity and global gauge binding;
- tile-wise uncertainty/TruthRange evaluation;
- bounded workspace;
- deterministic summaries/reductions;
- no loss of scientific semantics due to tiling.

Tile-size invariance tests then became necessary: different execution tilings must produce the same declared scientific summary/result.

## 10. Virtual Observation Manifold

Once the latent scene and TruthRange coordinate were explicit, the earlier multi-EV idea was reinterpreted correctly:

`one sealed RAW -> one latent scene -> many virtual numerical observations`

Virtual EV/ISO/gain/camera views can inspect/reparameterize the same scene or improve numerical conditioning. They are not independent physical evidence and do not lower the original measurement uncertainty merely by being numerous.

Any solver that uses them must ultimately remain bound to the one original measurement likelihood/evidence root.

## 11. Canonical three-part distinction

All current documentation should preserve this distinction:

### A. Address space

TruthRange's mathematical coordinate for positive light can extend without a fixed finite numerical ceiling/floor.

### B. Evidence support

The source measurement is finite, noisy, quantized, and may be censored/clipped. Reconstruction outside directly measured support carries explicit uncertainty/role.

### C. Gauge

The zero-line/gauge determines the coordinate origin. It removes a scale ambiguity; it never creates photons, measurements, dynamic range, or confidence.

## 12. Phrases to use / avoid

Use:

- “unbounded TruthRange address space”;
- “finite/censored sensor evidence”;
- “self-gauge relative scene coordinate”;
- “source clipping gives a bound, not an exact latent value”;
- “representation can exceed the source; knowledge claims cannot.”

Avoid unless immediately qualified:

- “infinite sensor dynamic range”;
- “zero-line removes clipping”;
- “ISO no longer matters scientifically”;
- “virtual exposures recover new measured photons”;
- “the zero-line proves absolute brightness”.

## 13. Historical conclusion

The recovered genealogy is now:

`zero-line house vision`

`-> T = log2(L/L0)`

`-> finite sensor DR separated from unbounded representation`

`-> clipping expressed as censoring/bounds`

`-> TruthRange v0.1 with temporary ISO/exposure gauge`

`-> TruthRange v0.2 SELF_GAUGE`

`-> TruthRange v0.3 explicit uncertainty roles`

`-> tiled/streaming TruthRange + Virtual Observation Manifold`

The important historical correction is therefore:

**v0.2 did not invent the zero-line. v0.2 made the already-existing zero-line concept scene-internal/self-gauged and removed the temporary ISO/exposure dependency from the relative coordinate definition.**
