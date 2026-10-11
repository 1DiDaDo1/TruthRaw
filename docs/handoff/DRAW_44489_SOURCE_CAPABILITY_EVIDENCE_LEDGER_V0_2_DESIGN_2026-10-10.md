# D.RAW 44489 — Source Capability Envelope v0.2 / Observation Evidence Ledger — 2026-10-10

Status: **DESIGN / DOCUMENTATION ONLY — NOT IMPLEMENTED, NOT PROMOTED**  
Scope: sealed non-vendor-authority RAW/DNG intake.

This design refines the project entrance without changing a single scientific sample. It is intentionally upstream of colour reconstruction, denoise, Appearance and Free Raster.

---

## 1. Goal

Improve the D.RAW entrance by extracting and classifying more information that is genuinely present in a sealed RAW observation while preserving the rule:

`source identity/provenance != scientific authority`.

The system may learn more from the file. It may not assume more because of the brand, app, camera model or filename.

---

## 2. Evidence lifecycle

Every candidate datum follows:

`PRESENT -> PARSED -> VALIDATED -> ADMITTED -> USED`

A datum may stop at any earlier state. Stopping is a valid result.

Recommended record fields:

- observation/source SHA-256;
- container/IFD/subIFD/path/tag identity;
- raw encoded value or safe canonical representation;
- parser + parser-version identity;
- parsed semantic type;
- validation result;
- admitted authority/class;
- units/reference frame;
- uncertainty/known limitations;
- contradiction set;
- rejection reason;
- consuming scientific/product modules;
- first-use and last-use provenance where useful.

No later module may silently reinterpret a rejected/provenance-only field as calibration authority.

---

## 3. Candidate harvest classes

### 3.1 Sampling structure

- dimensions and active area;
- CFA pattern;
- exact CFA phase/origin;
- masked/optical-black regions where explicitly represented;
- sample packing;
- bit depth / stored range;
- crop/orientation metadata without altering original source geometry.

### 3.2 Radiometric boundaries

- BlackLevel and repeat dimensions;
- black-level deltas where present;
- WhiteLevel;
- linearization tables/curves;
- clipping/censoring patterns observed in the samples;
- saturation metadata only after consistency checks against actual representation.

A metadata WhiteLevel is not automatically proof that every finite value below it is uncensored colour truth.

### 3.3 Exposure/acquisition context

- exposure time;
- aperture;
- ISO/gain metadata;
- timestamp/readout-related metadata where semantics are known;
- baseline exposure or analogous source hints classified according to specification semantics.

ISO/gain is context; it is not by itself a calibrated noise model.

### 3.4 Colour/calibration context

Where present and valid:

- ColorMatrix / CameraCalibration style matrices;
- calibration illuminants;
- AsShotNeutral or equivalent source white information;
- analog balance or equivalent standardized fields;
- forward matrices/profile data only according to their defined semantics.

Every transform must retain its stated source/target space and calibration domain. Do not flatten all matrices into one undifferentiated “camera profile”.

### 3.5 Noise/support context

- standardized/source-contained noise profile when semantics are clear;
- masked-region statistics when those pixels are genuinely available and authority-bound;
- local repeated-pattern evidence that can be measured without modifying the source;
- uncertainty that arises from missing/contradictory information.

A source-contained noise profile may be admitted as source metadata but must still be tested before it controls scientific denoise/reconstruction.

### 3.6 Optical/geometric context

Where standardized and semantically valid:

- lens-shading metadata;
- distortion/crop geometry;
- focal/aperture/focus-distance context;
- source transformation/opcode metadata.

Lens model/vendor identity alone may not select a scientific correction profile.

### 3.7 Opaque/proprietary blocks

MakerNotes and proprietary app/vendor blocks:

- preserve byte/provenance linkage;
- parse only when semantics are independently known;
- default to `PROVENANCE_ONLY` or `UNKNOWN_SEMANTICS`;
- never elevate because a known brand/model appears in the file.

---

## 4. Contradiction handling

Contradictions are evidence and must not be silently resolved by convenience.

Examples:

- metadata WhiteLevel disagrees with stored/sample behavior;
- CFA tag and actual layout are inconsistent;
- two calibration records claim incompatible domains;
- orientation/crop metadata produces impossible dimensions;
- source-contained noise profile conflicts with held-out residual behavior.

Recommended states:

- `CONSISTENT_ADMITTED`;
- `PRESENT_UNVALIDATED`;
- `CONTRADICTED_FAIL_CLOSED`;
- `PROVENANCE_ONLY`;
- `UNSUPPORTED_SEMANTICS`;
- `ABSENT`.

Contradiction must increase uncertainty or close the route; it must never be hidden by a default profile.

---

## 5. Proposed Source Capability Envelope v0.2

Conceptual envelope sections:

```text
SourceCapabilityEnvelopeV0_2
  observation_binding
  container_structure
  sampling_geometry
  radiometric_boundaries
  acquisition_context
  colour_calibration_context
  noise_context
  optical_geometry_context
  temporal_context
  evidence_ledger_digest
  contradictions
  admitted_capabilities
  unavailable_capabilities
```

The envelope describes what this observation can support. It is not itself a reconstructed image.

---

## 6. Observation Evidence Ledger

The ledger is append-only with respect to the sealed observation interpretation history. Parser improvements may add a new interpretation record; they must not erase the older provenance chain.

Each `USED` transition should identify which downstream computation consumed the datum and under what contract.

Examples:

- BlackLevel used for source-bound normalization;
- ColorMatrix used for a calibrated estimate with domain label;
- exposure time used for acquisition context;
- ISO retained but not used because no calibrated gain/noise mapping exists;
- MakerNote stored as provenance only;
- NoiseProfile parsed but held research-only pending residual validation.

---

## 7. Zero-pixel-change first milestone

The first implementation milestone must be introspection only:

1. parse richer source information;
2. emit deterministic Evidence Ledger records;
3. emit Source Capability Envelope v0.2;
4. compare against existing intake output;
5. prove sealed bytes and all Scientific-Master pixel values unchanged;
6. prove no product/Appearance output changes when the new fields are not explicitly consumed.

This is deliberate. The project should first prove it understands the source better before allowing the new knowledge to change reconstruction.

---

## 8. Validation gate

Before any ledger datum may control scientific reconstruction:

- real fixtures from multiple sources/containers where available;
- exact parser tests;
- semantic tests against the applicable public specification;
- contradiction/fail-closed tests;
- missing-field tests;
- no camera/vendor-name truth selection;
- source SHA binding;
- deterministic serialization/digest;
- source mutation false;
- scientific writeback false.

---

## 9. Relationship to colour/noise/continuous-field research

This ledger is the shared upstream evidence source for later research:

`sealed RAW -> Evidence Ledger / Capability Envelope -> scientific calibration + noise likelihood + optical support -> Scientific Master / observation-bound scientific field -> Appearance -> Free Raster`

The ledger does not authorize any of those later stages. It makes their inputs explicit and auditable.

---

## 10. External specification anchor

Adobe's public DNG resources describe DNG as a non-proprietary RAW container and provide the current DNG specification/SDK resources. D.RAW may use standardized DNG semantics as parsing definitions while still independently validating what a specific sealed observation actually contains:

https://helpx.adobe.com/camera-raw/desktop/dng-and-file-formats/digital-negative.html

This reference informs parsing semantics only; it does not make any particular tag scientifically correct for every source.
