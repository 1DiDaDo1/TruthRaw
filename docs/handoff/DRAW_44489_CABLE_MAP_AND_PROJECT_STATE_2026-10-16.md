# D.RAW / TruthRaw — 44489 Cable Map and Project State — 2026-10-16

Continuation code: **44489**  
Project: **D.RAW** (`TruthRaw` remains repository/history naming)  
Motto: **One Free World. Many sealed observations. One evidence law.**

## 1. Purpose

This document is the current human-readable registry of the project's major scientific, provenance, runtime, world, Appearance and output cables.

It answers four questions for every cable:

1. **What flows through it?**
2. **What authority may it carry?**
3. **What is actually proven now?**
4. **What is still blocked or explicitly forbidden?**

This document does not promote PR #136. The integrated scientific/runtime anchor remains `96be8a23c0d43f5dec304bdeb635824868cc80a7`.

Current unintegrated candidate source/runtime endpoint before this documentation refresh: `dfa80b2a5bfae587689b83c9c932c9f6658dd38b`.

Exact scientific/code safety checkpoint inside that line: `5ea0eb94a4659eaacc03e53d1d9904bdd2d98925`.

## 2. Status vocabulary

- **INTEGRATED** — present on the integrated/base architecture line.
- **PROVEN CANDIDATE** — execution/tests are proven on PR #136 but not integrated.
- **READ-ONLY / SHADOW** — may inspect/validate/report; may not modify scientific samples.
- **FAIL-CLOSED** — unsupported or contradictory input is rejected instead of guessed.
- **APPEARANCE ONLY** — human-facing rendering/output; never scientific evidence.
- **RESEARCH / PARTIAL** — useful architecture or experiments exist, but full runtime/scientific authority is not established.
- **OPEN GAP** — intentionally unresolved.
- **HISTORICAL PROVENANCE** — retained evidence of an older test/checkpoint, not current routing authority.

## 3. Cable registry

### A. Physical observation, seal and provenance

| ID | Cable | Current status | What is proven | Hard boundary |
|---|---|---|---|---|
| C01 | Real world -> lens -> sensor exposure | PHYSICAL ORIGIN | The physical exposure is the source of direct measurement. | Software cannot create a second physical observation from processing. |
| C02 | Sensor/CFA -> RAW_SENSOR or source DNG | INTEGRATED architecture | One physical frame is kept as one observation; camera/container identity is provenance/routing. | Vendor/model name cannot choose scientific truth. |
| C03 | Source bytes -> SourceSeal / source SHA-256 | INTEGRATED | Source identity is bound before downstream reasoning. | Sealed source bytes are immutable. |
| C04 | SourceSeal -> Observation Evidence Ledger | INTEGRATED | Occurrence/traversal provenance binds source SHA, byte ranges, IFD path/offset, tag/type/count and parser status. | Do not build a second parser/hash/ledger cable. |
| C05 | Evidence Ledger -> Source Capability Runtime Evidence Envelope | INTEGRATED read-only | Runtime can expose source-bound capability reports through one existing envelope. | Envelope success is not admission or use. |
| C06 | Lifecycle / authority labels -> consumers | INTEGRATED governance | `VALIDATED != ADMITTED != USED`; authority classes remain distinct. | A green validator never silently enables a consumer. |

### B. DNG structure and Raw-IFD identity

| ID | Cable | Current status | What is proven | Hard boundary |
|---|---|---|---|---|
| C07 | DNG IFD discovery -> Raw-IFD domain | INTEGRATED | Existing Raw-IFD validator/selector is the single domain route. Ambiguous CFA IFDs require explicit authority. | No second Raw-IFD detector. |
| C08 | Validated source-bound Raw-IFD authority -> `RawSourceOpenRequest.explicitRawIfdOffset` | PROVEN CANDIDATE | Executed native regression at `2f08e151...`. The exact non-zero authority reaches the actual open request. | Contradictory authority fails closed. |
| C09 | Open request -> native DNG selection -> `SourceAudit.rawIfdOffset` | PROVEN CANDIDATE | Actual decoder-selected Raw IFD is compared with expected authority. | Mismatch returns `SourceSealMismatch`; no silent fallback. |
| C10 | No explicit authority -> unique supported CFA IFD | PROVEN CANDIDATE compatibility | Legacy path remains available only when the decoder sees one supported CFA IFD. | Multiple candidates remain ambiguous/fail-closed. |
| C11 | Selected TIFF/DNG storage -> sealed raw tile samples | INTEGRATED supported subset | Tile-native source reads the selected uncompressed 16-bit CFA payload from strip/tile storage and keeps raw sample values separate from gain data. | This is not a claim of arbitrary-DNG support. |

### C. Radiometric metadata and raw-to-linear dependencies

| ID | Cable | Current status | What is proven | Hard boundary |
|---|---|---|---|---|
| C12 | DNG version/encoding metadata -> compatibility validators | INTEGRATED read-only | Version/encoding/format-domain consistency is validated separately from parser success. | Parseable does not mean scientifically usable. |
| C13 | `BlackLevelRepeatDim` + `BlackLevel` -> tile-native `blackPhase` | INTEGRATED supported subset | Scalar or four-phase BlackLevel is bound; 2x2 repeat is checked for four phase values. | Does not include spatial H/V deltas. |
| C14 | `BlackLevelDeltaH/V` -> Effective Black metadata semantics | READ-ONLY / SHADOW | Source Capability validators can describe the DNG dependency relation. | Tile-native sample route does not execute H/V deltas. |
| C15 | `BlackLevelDeltaH/V` -> selected Raw-IFD sample route | PROVEN CANDIDATE, FAIL-CLOSED | At `5ea0eb94...`, tags 50715/50716 on the selected candidate Raw IFD are detected and rejected before incomplete sample semantics can masquerade as full support. | No delta application has been promoted. |
| C16 | `WhiteLevel` -> encoding/normalization dependency | INTEGRATED supported subset | Tile-native binds positive scalar WhiteLevel and requires it to exceed bound BlackLevel phases. | WhiteLevel is not physical full-well calibration or scene-radiance truth. |
| C17 | `LinearizationTable` occurrence -> Observation Evidence / runtime diagnostic | INTEGRATED READ-ONLY | Tag remains physical key `TAG_50712`; explicit occurrence can be parsed/validated/reported. Lifecycle remains PARSED, admission `UNSUPPORTED_SEMANTICS`, authority `UNKNOWN_SEMANTICS`, zero consumers. | Parser identity must not be renamed casually; no sample application. |
| C18 | `LinearizationTable` -> selected Raw-IFD sample route | PROVEN CANDIDATE, FAIL-CLOSED | At `5ea0eb94...`, tag 50712 on selected candidate Raw IFD is rejected by the tile-native route because exact application is not implemented there. | `linearization_applied_to_image_samples=false`. |
| C19 | Effective Black / Normalization report -> Raw-to-Linear compositor | READ-ONLY / SHADOW | Existing report is computed once and reused by the existing compositor cable. | Metadata relation is not sample execution. |
| C20 | LinearizationTable report -> Raw-to-Linear compositor | READ-ONLY / SHADOW | Existing report is computed once and reused; no duplicate parser/read/hash. | Zero scientific consumers of table values. |
| C21 | Raw-to-Linear dependency order | READ-ONLY / SHADOW contract | Contract represents `Linearization -> Black subtraction -> normalization -> clipping`. | `full_raw_to_linear_mapping_validated=false`. |
| C22 | Unsupported 50712/50715/50716 -> source open | PROVEN CANDIDATE, FAIL-CLOSED | New dependency guard succeeds under GCC, Clang and Clang ASan/UBSan. | Refusal is not implementation. |
| C23 | DNG post-normalization clipping rule -> contract report | READ-ONLY / SHADOW | The DNG rule is represented separately as a dependency/specification rule. | Does not prove observed censoring, sensor saturation or full-well. |

### D. Sample execution, censoring, noise and colour

| ID | Cable | Current status | What is proven | Hard boundary |
|---|---|---|---|---|
| C24 | Raw CFA samples -> existing sample-stage numerical route | INTEGRATED legacy execution, SEMANTICALLY INCOMPLETE FOR GENERAL DNG | Existing scientific pipeline contains black/white normalization-style sample mathematics for supported cases. | It is not allowed to stand in for complete DNG ordering when LinearizationTable or BlackLevelDelta dependencies exist. |
| C25 | Raw-to-linear complete DNG semantics -> Scientific Master | OPEN GAP | No full proof yet binds all required DNG dependencies, exact source/IFD authority and exact operation order into one validated sample mapping. | All sample-application/full-mapping flags stay false. |
| C26 | Observed clipping pattern -> CENSORED / TruthRange authority | INTEGRATED separate subsystem | Observed-sample censoring remains a separate authority question from DNG WhiteLevel/clipping-rule metadata. | Never infer physical saturation merely from a metadata threshold. |
| C27 | NoiseProfile / sensor noise evidence -> uncertainty/covariance | INTEGRATED where evidence exists | Noise and covariance propagate separately from pixel value authority. | Unknown covariance is not zero covariance. |
| C28 | OpcodeList2 GainMap -> gain field | INTEGRATED supported subset | GainMap is parsed/evaluated separately; optional unsupported opcodes remain explicit in audit rather than silently synthesizing source samples. | Optional skipped operations do not become measured corrections. |
| C29 | Explicit colour binding -> camera RGB -> XYZ D50 | INTEGRATED | Colour transform is source/binding controlled and uncertainty-aware in downstream work. | A 3x3 camera transform is not exact arbitrary-spectrum human colorimetry. |
| C30 | CFA reconstruction -> Scientific Master | INTEGRATED reconstruction architecture | Measured anchors remain distinct from reconstructed values; reconstruction cannot rewrite sealed CFA evidence. | Reconstructed != MEASURED. |

### E. Scientific representation, world and geometry

| ID | Cable | Current status | What is proven | Hard boundary |
|---|---|---|---|---|
| C31 | Scientific Master -> dynamic authority + uncertainty | INTEGRATED architecture | Authority/uncertainty travel with scientific values. | Richer representation cannot imply richer evidence. |
| C32 | Scientific Master -> observation-bound continuous scientific representation | RESEARCH / PARTIAL architecture | Continuous negative/field representation is a downstream scientific representation per observation. | Interpolation density does not create physical samples. |
| C33 | Observation-bound representation -> Free World Observation Graph | RESEARCH / PARTIAL | Multiple observations may be linked only through proven registration/gauge relations. | Separate observations never collapse into invented common MEASURED truth. |
| C34 | World observations -> geometry/depth | RESEARCH / EVIDENCE-BOUND | Projective/registration work may create calibrated estimates or reconstructed geometry where constraints exist. | Single-view ambiguity and unproven scale remain explicit. |
| C35 | Scientific Master -> T5 Deep Scene -> Light Transport -> Room Capsule | EXISTING RESEARCH CABLE | The project has one named deep-scene/light-transport/Room-Capsule route with exact-preserving bypass when required evidence is absent. | No admitted geometry/material/illumination means no invented scientific scene truth. |

### F. Appearance, output and workbench

| ID | Cable | Current status | What is proven | Hard boundary |
|---|---|---|---|---|
| C36 | Scientific Master -> PURE / ADVANCED / PRO | INTEGRATED output architecture | Presentation modes are downstream of scientific state. | Appearance cannot rewrite Scientific Master. |
| C37 | Scientific state -> highlight/chroma/illuminant presentation | APPEARANCE ONLY | Warm-illuminant, highlight/chroma guards and related presentation experiments remain human-facing. | Visual plausibility never grants scientific authority. |
| C38 | Unified Output State -> Free Raster | INTEGRATED from PR #131 line | Free Raster can choose output position/resolution downstream. | Output density does not create new evidence or MEASURED detail. |
| C39 | Unified Output -> high-fidelity JPEG | INTEGRATED / device-tested historically | Preview-independent high-quality derived JPEG path has real-device evidence. | JPEG is a derived presentation output, not source evidence. |
| C40 | Workspace -> reversible edit state -> presentation/export | INTEGRATED non-destructive workbench | Source and reversible edit state remain separate; export creates a new derived output. | Never overwrite the immutable source with Appearance edits. |
| C41 | Preview -> UI | INTEGRATED presentation support | Preview is a convenience/display object. | Preview is never scientific authority or a hidden high-resolution master. |

### G. Gauge, CI, APK and promotion firewall

| ID | Cable | Current status | What is proven | Hard boundary |
|---|---|---|---|---|
| C42 | Exact Gauge / PassArtifact -> downstream validation | INTEGRATED | Exact Gauge Retained Artifact v0.3 and related pass-artifact governance remain part of the base architecture. | Gauge validity does not promote unrelated scientific claims. |
| C43 | Scientific candidate -> CI | ACTIVE | Current raw-to-linear guard, tile-native compatibility and Raw-to-Linear Contract checks are green on their exact tested commits. | Report exact workflow scope; do not say “all CI green”. |
| C44 | Documentation -> Documentation Governance workflow | KNOWN RED / SEPARATE | Current failure class predates the new Raw-IFD/dependency patch and concerns documentation authority/pointers. | It is not evidence that raw-to-linear science failed, and it should not be silently ignored. |
| C45 | Candidate branch -> stable-signed APK | PROVEN CANDIDATE | APK built successfully at `ec29f23...`; signing identity verification succeeded. APK SHA-256 `28a0eb22...94141`. | An APK proves executable packaging/device behavior, not scientific semantics by itself. |
| C46 | APK -> real-device source/IFD/dependency evidence | CURRENT NEXT TEST | Use tele/main RAW/DNG to observe source SHA, selected Raw IFD and whether unsupported dependencies fail closed. | Visual change is not a success criterion. |
| C47 | PR #136 -> integration/promotion | NOT AUTHORIZED | PR remains draft/open/unmerged and mergeable at the pre-refresh head. | No merge, no admission/use promotion and no Scientific Master writeback without an explicit later decision. |

## 4. The critical active cable in one diagram

```text
SEALED RAW / DNG
    |
    v
SourceSeal / source SHA
    |
    v
Observation Evidence Ledger
    |
    v
Raw-IFD domain + validated source-bound Raw-IFD authority
    |
    v
RawSourceOpenRequest.explicitRawIfdOffset
    |
    v
TileNativeDngSource actual CFA-IFD selection
    |
    +--> SourceAudit.rawIfdOffset == expected ? ---- no ----> FAIL CLOSED
    |                                                 SourceSealMismatch
    v
Inspect selected Raw IFD dependencies
    |
    +--> 50712 LinearizationTable present? ----------> FAIL CLOSED for this sample route
    +--> 50715 BlackLevelDeltaH present? ------------> FAIL CLOSED for this sample route
    +--> 50716 BlackLevelDeltaV present? ------------> FAIL CLOSED for this sample route
    |
    v
Supported subset may continue
    |
    v
existing tile/raw + metadata path
    |
    v
Scientific pipeline
```

The important scientific meaning is asymmetric:

**“Allowed to continue” means only that this specific unsupported-dependency guard found no blocker. It does not mean the entire DNG raw-to-linear mapping has been validated.**

## 5. Hard false firewall — unchanged

The following remain false until direct sample-domain proof exists:

- `linearization_applied_to_image_samples=false`
- `black_subtraction_applied_to_image_samples=false`
- `normalization_applied_to_image_samples=false`
- `clipping_applied_to_image_samples=false`
- `table_application_to_image_samples_validated=false`
- `full_raw_to_linear_mapping_validated=false`
- `record_lifecycle_effect=false`
- `scientific_admission_effect=false`
- `downstream_scientific_consumer_enabled=false`
- `scientific_master_pixel_effect=false`
- `appearance_pixel_effect=false`
- `source_values_modified=false`
- `creates_new_evidence=false`
- `scientific_writeback_allowed=false`

## 6. Latest exact checkpoints

| Meaning | Exact checkpoint |
|---|---|
| Integrated scientific/runtime anchor | `96be8a23c0d43f5dec304bdeb635824868cc80a7` |
| Native Raw-IFD authority bridge execution proof | `2f08e15163377672d7b8d15894d8657dccd951f1` |
| Fail-closed unsupported raw-to-linear dependency code | `5ea0eb94a4659eaacc03e53d1d9904bdd2d98925` |
| Temporary stable-signed APK trigger/build | `ec29f23a8c8ce407f1fdd4f83f6e9d4d9b921e6c` |
| Trigger cleanup / source-runtime endpoint before docs refresh | `dfa80b2a5bfae587689b83c9c932c9f6658dd38b` |
| APK SHA-256 | `28a0eb22cc773852af7c65529593b1072b008c5345a91763a9a66c403cf94141` |

## 7. What is now safer than before

1. Raw-IFD authority no longer stops at a metadata claim; the exact authority is proven to reach the native decoder selection.
2. Decoder selection is audited after open and mismatches fail closed.
3. DNG files requiring LinearizationTable or spatial black deltas cannot silently pass through the simpler tile-native sample route as if full DNG semantics were implemented.
4. The existing evidence/Source Capability/Raw-IFD cables remain singular; no duplicate parser or pixel pipeline was created.
5. The hard false promotion firewall remains intact.

## 8. What is still not solved

1. Exact sample-domain LinearizationTable execution.
2. Exact sample-domain BlackLevelDeltaH/V execution.
3. A single proven end-to-end DNG sample mapping with exact order:
   `Linearization -> effective Black -> normalization -> clipping`.
4. Full coupling of that mapping to the same source SHA, Raw IFD, CFA coordinates and authority state.
5. Physical saturation/full-well/scene-radiance calibration from DNG metadata alone.
6. Real-device evidence from the newly built APK on current genuine tele/main observations.
7. The independent Documentation Governance pointer failure.

## 9. Immediate next device protocol

Use the latest stable-signed APK and provide the resulting D.RAW/RAW/DNG capture bundle and diagnostics.

For tele and main separately:

- preserve original source file;
- record source SHA;
- record selected/validated Raw-IFD authority;
- record decoder `SourceAudit.rawIfdOffset`;
- inspect occurrence of 50712/50715/50716;
- expect fail-closed behavior when one of those unsupported dependencies is required by the selected Raw IFD;
- verify no new Scientific Master/Appearance authority is created;
- compare PURE/ADVANCED/PRO only as regression evidence, not as scientific proof.

## 10. Evidence Law — never weaken

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`

`VALIDATED != ADMITTED != USED`

- Seal the evidence, not the thinking.
- Representation may become richer than the source; knowledge claims may not exceed evidence.
- Stable outside. Flexible inside. Evidence law unchanged.
