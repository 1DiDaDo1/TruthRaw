# D.RAW Core Vision — Lens-Independent Free World Observation Architecture

**Status: CANONICAL PROJECT ARCHITECTURE — SEALED BY v0.1 CONTRACT**

This document extends, without replacing, the canonical sealed Source Evidence,
Zero-Line / TruthRange, Scientific Master, TruthNegative Continuous and Free
World architecture.

Permanent rule:

> **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

Companion rule:

> **Measured where measured. Reconstructed where necessary. Never invented.**

## 1. The Free World is not a tele-camera world

D.RAW does not define a different scientific world for main, ultra-wide,
telephoto, front-camera, DSLR, mirrorless or future admitted image sources.

The camera, sensor, lens, readout path and capture settings describe **how one
observation was made**. They do not define the mathematical extent of the
world that observation may constrain.

The telephoto Camera-5 route remains important measured provenance, but it is
one Observation Procedure, not the architecture.

Canonical form:

```text
physical scene / Free World
        |
        | observed through
        v
Observation Procedure
(sensor + lens + CFA + readout + exposure + capture pipeline)
        |
        v
Sealed Observation / Source Evidence
        |
        v
measurement + calibration + authority
        |
        v
Scientific Master
        |
        v
TruthNegative Continuous
        |
        v
Free World Observation Graph
        |
        v
View / Appearance / finite output
```

## 2. Universal D.RAW Observation Contract

Every admitted physical image source is represented by one explicit
`DRAWObservation`.

An observation binds at least:

- immutable Source Evidence identity;
- physical-frame identity;
- capture time / interval;
- camera/sensor/lens/readout identity where known;
- source sample topology and raster provenance;
- exposure/gain/readout provenance;
- radiometric calibration binding and its authority;
- color binding and its authority;
- noise/uncertainty model and its authority;
- optics model and its authority;
- geometry/projection model and its authority;
- censor/bound state;
- Scientific Master identity;
- TruthNegative identity;
- source capability envelope;
- complete provenance ancestry.

Unknown fields remain UNKNOWN. Missing metadata is not silently filled.

## 3. Source Capability Envelope

The Free World consumes a source-neutral capability description rather than a
camera-specific world.

The envelope separates at least:

- `sampling_geometry`;
- `radiometry`;
- `color`;
- `noise_uncertainty`;
- `optics`;
- `geometry_projection`;
- `dynamic_range_bounds`;
- `temporal_state`;
- `calibration_scope`;
- `metadata_completeness`.

A stronger source may support tighter uncertainty, wider admitted bounds,
stronger color authority, stronger geometric support or better optical
knowledge.

A weaker source enters the same architecture with more UNKNOWN, wider
uncertainty or weaker authority.

**The world does not shrink to match the source. The claim surface does.**

## 4. Zero-Line / TruthRange is the common radiometric coordinate family

The canonical positive-light companion coordinate remains:

`T = log2(L / L0)`

where `L0 > 0` defines the zero-line gauge.

Permanent interpretation:

- `T=0` is not sensor black;
- `T=0` is not DNG BlackLevel;
- `T=0` is not display black;
- `T=0` is not clipping;
- `T=0` is not zero photons;
- the coordinate may extend mathematically toward `-infinity` and
  `+infinity`;
- each real observation supports only a finite, uncertainty-bounded subset.

The Zero-Line is therefore a bridge from source-specific measurement scales to
a common Free-World radiometric coordinate family.

## 5. A common coordinate is not automatically a common calibration

Two observations may both use TruthRange and still lack authority to assert
that equal numerical values represent equal scene radiometry.

Example:

```text
main observation:  T = +2.0, gauge A
tele observation:  T = +2.0, gauge B
```

These values are not scientifically interchangeable until an admitted
cross-observation gauge relation exists.

Every observation therefore carries:

- `scale_gauge_id`;
- `gauge_binding_authority`;
- optional relation to a shared Free-World gauge;
- uncertainty of that relation.

If the relation is absent:

`COMMON_GAUGE = UNPROVEN`

The Free World may store both observations but may not fuse or compare their
absolute radiometric level as if calibrated.

**The coordinate family may be universal. Equality between measurements must
still be earned by calibration.**

## 6. Dual numerical representation remains mandatory

D.RAW retains two complementary domains.

### A. Signed scene-linear estimator

Used for:

- de-ISP;
- reconstruction;
- residuals;
- unbiased numerical estimation;
- matrix/color operations;
- optimization;
- covariance/uncertainty propagation.

Small negative estimates are legal numerical states and do not represent
negative physical photons.

### B. Positive-light TruthRange companion

Used for:

- relative radiometric placement around the Zero-Line;
- evidence-supported dynamic range;
- high-side censor lower bounds;
- dark/noise-side upper bounds;
- unbounded tails;
- cross-observation gauge relations where admitted.

Negative scene-linear estimates are never naively passed through `log2`.

## 7. Float64 compute / Float32 canonical storage

Precision is stage-specific.

Canonical direction:

```text
exact packed/integer Source Evidence
 -> Float64 branch-sensitive reconstruction/calibration/optimization/covariance
 -> validated canonical Float32 scientific storage where equivalence permits
 -> Float64 again for later sensitive calculations when required
```

A later Float32-to-Float64 conversion cannot restore a branch decision already
lost in Float32.

Float32 storage therefore does not define the truth authority of the state.
Float64 arithmetic does not create evidence.

## 8. D.RAW Scientific RAW state

D.RAW may maintain its own scientific RAW/state representation downstream of
sealed Source Evidence.

This representation is not the original sensor RAW and may never impersonate
it.

The scientific state is designed to carry, per sample/field element where
applicable:

- signed scene-linear value;
- optional TruthRange estimate;
- TruthRange lower/upper bounds;
- uncertainty;
- authority;
- creation role;
- censor state;
- source footprint;
- source Observation ID;
- calibration identity;
- Zero-Line / gauge identity;
- geometry authority;
- radiometric authority;
- provenance ancestry.

A materialized raster is one storage/view of that state, not the definition of
the Free World.

## 9. TruthNegative role is now explicit

TruthNegative is the raster-independent, queryable, evidence-aware scientific
negative **of one admitted observation lineage**.

TruthNegative:

- binds Source Evidence, Scientific Master and local authority;
- preserves one-observation provenance;
- may answer arbitrary finite footprint queries;
- is not defined by a 4080x3072, 50 MP, 200 MP or other target lattice;
- creates no measured target pixels by resampling;
- does not become multi-camera merely because another lens exists.

TruthNegative is therefore the scientific bridge from one sealed observation
into the wider Free World.

It is not required to fuse observations itself.

## 10. Free World Observation Graph

The Free World may contain one or more separately sealed TruthNegative-bound
observations.

Example:

```text
Free World
  |
  +-- Observation A
  |     main camera
  |     -> TruthNegative A
  |
  +-- Observation B
  |     ultra-wide
  |     -> TruthNegative B
  |
  +-- Observation C
        telephoto
        -> TruthNegative C
```

The graph may express:

- common world-space hypotheses;
- registered image-plane relations;
- shared or unresolved radiometric gauges;
- geometry correspondences;
- visibility/occlusion relations;
- temporal relations;
- disagreements;
- UNKNOWN regions;
- counterfactual state.

No relation is promoted merely because two observations look similar.

## 11. Evidence counting remains physical

For each canonical single-frame TruthNegative:

- `physicalFrameCount = 1`;
- `independentEvidenceCount = 1` under its existing admitted contract.

A future Free World may contain multiple physical observations.

Then the graph must keep separate:

- observation count;
- physical frame count;
- source identities;
- correlation/independence assumptions.

Two cameras do not automatically equal two statistically independent noise
samples.

A synchronized multi-camera capture does not erase individual provenance.

A virtual exposure, interpolation, rendered view, animation frame, denoise
candidate, resampling or appearance transform contributes **zero** new physical
evidence.

## 12. Geometry and radiometry remain separate

A radiometrically strong observation may have only image-plane geometry.

A geometrically strong multi-view solution may still contain UNKNOWN or
CENSORED radiometry.

Every Free-World contribution therefore keeps at least:

- radiometric authority;
- geometry authority.

Neither may silently upgrade the other.

## 13. World dimensions are available, not automatically measured

The Free World may represent state over dimensions such as:

`S(x, y, z, t, view, direction, focus, wavelength, illumination, ...)`

This is representational freedom, not a statement that every dimension has
been observed.

Every populated field must retain authority, uncertainty, bounds and
provenance.

Unobserved dimensions remain UNKNOWN, INFERRED or COUNTERFACTUAL as applicable.

## 14. Do not flatten before a View requires it

The Free World may preserve multiple scene/deep contributions, source
observations, bounds or hypotheses until a finite View is requested.

A final output pixel is therefore a **resolve/projection**, not the primitive
truth object.

This applies to:

- preview rasters;
- DNG derivatives;
- EXR;
- HDR/SDR;
- crops;
- resized projections;
- virtual-camera views;
- future deep or multi-observation outputs.

## 15. View Contracts

The same Free World may expose different downstream contracts without creating
different scientific worlds.

### PURE — Scientific View

May expose:

- scientific values;
- authority;
- uncertainty;
- bounds;
- provenance.

May not silently include creative appearance or counterfactual state.

### ADVANCED — Appearance / Restoration View

May expose:

- scientific state;
- explicitly reconstructed restoration;
- appearance transforms;
- appearance-only denoise/sharpening/color/tone.

Must preserve measured/reconstructed/appearance separation.

No scientific writeback.

### PRO — Open Scene / Light Transport View

May expose:

- observation graph;
- Deep Scene;
- geometry hypotheses;
- material/light hypotheses;
- virtual camera;
- counterfactual scene state.

Hypotheses remain labelled by their actual authority.

## 16. Multi-source admission law

Main, ultra-wide and telephoto may enter the Free World without redesigning the
world.

Another phone or professional camera may also enter if it satisfies the same
Observation Contract.

A new source must never inherit calibration merely because it shares:

- a device;
- a lens family;
- a manufacturer;
- a megapixel count;
- a nominal ISO;
- a file format.

Transferability is separately admitted.

## 17. Relationship to existing project modules

This architecture does not replace:

- sealed Source Evidence / Direct CFA;
- Scientific Master;
- Dynamic Authority / Open Scene Field;
- Zero-Line / TruthRange;
- TruthNegative Continuous v0.5;
- Deep Scene;
- Light Transport;
- N2 research;
- Appearance/Display Resolve.

It defines how they compose when D.RAW becomes source- and lens-independent.

## 18. Sealed architectural law

The following is the canonical statement:

> **A lens is an observation instrument, not a world boundary. Every admitted
> RAW/DNG enters D.RAW as separately sealed Source Evidence, is interpreted
> through its own capability/calibration envelope, and may bind to a
> raster-independent TruthNegative. The Free World composes those
> observations without erasing their provenance or authority. The Zero-Line /
> TruthRange provides an unbounded common radiometric coordinate family, but
> cross-observation equality requires an admitted gauge relation. Float64 may
> improve numerical computation and Float32 may store validated scientific
> state; neither increases evidence. Finite pixels, rasters and displays are
> views of the world, never the world itself.**

Short form:

> **One Free World. Many sealed observations. One evidence law.**
