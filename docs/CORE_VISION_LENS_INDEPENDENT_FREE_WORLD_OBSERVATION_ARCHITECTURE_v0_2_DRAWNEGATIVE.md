# D.RAW Core Vision — Lens-Independent Free World v0.2 — D.RAWnegative

**Status: CURRENT CANONICAL SUCCESSOR TO SEALED v0.1**

Parent:

`docs/CORE_VISION_LENS_INDEPENDENT_FREE_WORLD_OBSERVATION_ARCHITECTURE.md`

The parent v0.1 bytes remain sealed historical provenance. This v0.2 successor
changes the current public scientific-negative identity from **TruthNegative**
to **D.RAWnegative** without rewriting any parent bytes, hashes, schemas or
stable implementation symbols.

Permanent law:

> **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

Canonical short law:

> **One Free World. Many sealed observations. One evidence law.**

## 1. Current scientific-negative name

The current D.RAW scientific negative is:

**D.RAWnegative**

Definition:

> **D.RAWnegative is the raster-independent, observation-bound,
> authority-aware scientific negative developed from one admitted D.RAW
> observation lineage.**

The word `TruthNegative` remains valid only where historical/compatibility
identity requires it.

## 2. TruthNegative is ancestry, not a second world

The existing validated TruthNegative Continuous v0.5 state is retained as the
computational parent of D.RAWnegative v0.1.

```text
sealed Source Evidence
 -> measurement / calibration
 -> Scientific Master
 -> local authority / Open Scene
 -> TruthNegative Continuous v0.5      [sealed/legacy computational parent]
 -> D.RAWnegative v0.1                 [current scientific-negative identity]
 -> Free World Observation Graph
 -> View Contract
 -> finite projection
```

D.RAWnegative incorporates the parent TruthNegative state identity into its
own state SHA-256.

This means:

- historical reproducibility is retained;
- the old state is not deleted;
- old schemas do not lie about their original identity;
- D.RAWnegative is not a cosmetic rename of bytes;
- D.RAWnegative is a new explicit binding layer.

## 3. Observation binding

Every D.RAWnegative belongs to exactly one admitted D.RAW Observation lineage.

It binds at least:

- sealed Source Evidence identity;
- Scientific Master identity;
- authority-field identity;
- parent TruthNegative Continuous identity;
- D.RAW Observation ID;
- Zero-Line / TruthRange gauge ID;
- gauge-relation authority;
- precision/storage contract;
- provenance ancestry.

A target raster is not part of D.RAWnegative identity.

## 4. Lens independence

Main, ultra-wide, telephoto, front camera and external cameras do not receive
different scientific worlds.

Each physical observation receives its own:

```text
Source Evidence
 -> Source Capability Envelope
 -> Scientific Master
 -> D.RAWnegative
```

Those D.RAWnegative states compose in the Free World Observation Graph.

No lens label upgrades authority.

No calibration transfers implicitly between lenses.

## 5. Zero-Line / TruthRange

The positive-light companion coordinate remains:

`T = log2(L/L0)`

Every D.RAWnegative carries a `scaleGaugeId`.

A source-local gauge permits the observation to inhabit the common TruthRange
coordinate family without claiming equivalence to another observation.

For a source-local gauge:

- shared Free-World gauge = absent;
- cross-observation radiometric equality = forbidden;
- cross-observation radiometric fusion = forbidden.

A shared relative or absolute gauge relation may enable those operations only
after that relation is independently admitted.

**Same coordinate family is not the same as proven same calibration.**

## 6. Signed scene-linear and TruthRange remain dual

D.RAWnegative preserves the existing two-domain law:

1. signed scene-linear estimator for reconstruction, residuals, matrix/color
   work, optimization and covariance;
2. positive-light TruthRange companion for Zero-Line-relative radiometric
   placement, bounds and censor tails.

Negative numerical scene-linear estimates are not negative photons and are not
naively transformed through log2.

## 7. Float64 / Float32

Canonical direction:

```text
exact integer/packed Source Evidence
 -> Float64 branch-sensitive scientific compute
 -> controlled Float32 canonical storage where validated
 -> Float64 later again where a later sensitive computation requires it
```

Float64 does not create authority.

Float32 storage does not reduce authority merely because storage is Float32
when its equivalence gate has passed.

Float32 -> Float64 later cannot restore an already changed Float32 branch
decision.

## 8. D.RAWnegative state v0.1 boundary

The first D.RAWnegative implementation deliberately does not invent a new
per-sample TruthRange field.

It currently binds:

- the validated parent scientific-negative state;
- Observation ID;
- source/shared gauge identity;
- gauge relation;
- precision/storage policy;
- evidence/writeback invariants.

`perSampleTruthRangeMaterialized=false` is valid.

A later D.RAWnegative version may materialize per-sample TruthRange only with
explicit uncertainty/bound/censor semantics.

## 9. Multi-observation Free World

One D.RAWnegative is not a multi-camera fusion object.

```text
Free World Observation Graph
  +-- D.RAWnegative A <- main observation
  +-- D.RAWnegative B <- ultra-wide observation
  +-- D.RAWnegative C <- tele observation
```

Relations between them may express:

- registration;
- common geometry;
- time;
- visibility;
- gauge calibration;
- uncertainty/correlation;
- disagreement.

The graph owns those relations. Individual negatives retain their identity.

## 10. Evidence counts

A current single-frame D.RAWnegative retains:

- one physical frame;
- one independent evidence item under the current single-frame contract.

Resampling, virtual exposure, animation, appearance, denoise proposals and
output rasters add zero physical evidence.

Multiple admitted observations retain separate source identities and explicit
correlation/independence assumptions.

## 11. Geometry/radiometry separation

D.RAWnegative and the Free World retain separate geometry and radiometry
authority.

Strong geometry cannot turn UNKNOWN color into measured radiometry.

Strong radiometry does not prove 3D geometry.

## 12. Public naming and compatibility policy

New public/UI/project-current wording:

**D.RAWnegative**

Allowed historical/internal wording:

- `TruthNegative` C++/Kotlin namespaces/classes;
- legacy schema strings;
- legacy JNI symbols;
- historical branch/file names;
- historical hashes;
- `.tnc` container schema/magic;
- TN-3/TN-4 historical exports.

These are compatibility identities, not competing current product names.

Do not mass-rename them if doing so changes bytes, hashes, ABI, wire format or
reproducibility.

## 13. Legacy .tnc boundary

The current `.tnc` format remains:

**legacy TruthNegative native-container compatibility format**

D.RAWnegative v0.1 may bind a state above the same parent data, but the new
D.RAWnegative state must not be falsely claimed as stored inside a legacy
`.tnc` file if the bytes do not contain it.

A future native D.RAWnegative container must receive its own versioned
schema/magic and explicit migration/parent binding.

## 14. View contracts

Current public route semantics remain:

```text
D.RAW PURE     = Scientific View
D.RAW ADVANCED = Appearance / Restoration View
D.RAW PRO      = Open Scene / Light Transport / D.RAWnegative diagnostics
```

Appearance never writes back into D.RAWnegative.

N2 never writes D.RAWnegative unless a future separately admitted contract
explicitly changes that law.

## 15. Current canonical chain

```text
sealed physical observation / Source Evidence
 -> D.RAW Observation Contract
 -> Source Capability Envelope
 -> Float64 measurement/calibration/reconstruction
 -> Scientific Master
 -> validated Float32 scientific storage where admitted
 -> legacy TruthNegative Continuous parent
 -> D.RAWnegative per observation
 -> Free World Observation Graph
 -> Deep Scene / Light Transport
 -> View / Appearance
 -> finite projection
```

## 16. Canonical D.RAWnegative law

> **D.RAWnegative is the developed scientific negative of one sealed D.RAW
> observation: free from the source container's raster/numeric limits, bound
> forever to the source's provenance, authority, uncertainty and admitted
> gauge.**

Equivalent short form:

> **Richer negative, same evidence law.**
