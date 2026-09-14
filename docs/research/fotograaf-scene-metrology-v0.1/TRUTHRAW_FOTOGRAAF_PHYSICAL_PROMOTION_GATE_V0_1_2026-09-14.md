# TruthRaw FotoGraaf Physical Promotion Gate v0.1 — 2026-09-14

**Status: RESEARCH ELIGIBILITY GATE / FAIL-CLOSED / NO PRODUCTION SCIENTIFIC-MASTER CHANGE**

This step closes the missing authority bridge between a successful FotoGraaf shadow comparison and a future production calibration route.

A shadow model can demonstrate that a calibrated interpretation is internally coherent, identity-bound, censor-safe and worker-invariant. That is still not enough to feed it into the production Scientific Master.

The new chain is:

`controlled calibration bytes`
→ `validated CalibrationPack`
→ `exact Scene Admission`
→ `CalibrationBindingPacket`
→ `exact model/protocol bytes`
→ `shadow MeasurementLab comparison`
→ `held-out physical validation`
→ `uncertainty validation`
→ **`Physical Promotion Gate`**
→ `ELIGIBLE_FOR_EXPLICIT_PROMOTION_REVIEW`

The last state is deliberately **eligibility**, not automatic production activation.

## 1. Why another gate is necessary

`CALIBRATED_PHYSICAL_ADMITTED` means the scene is inside the validated scope/domain of a calibration claim and has an exact binding. It does not by itself prove that using the calibrated model in the production measurement path leaves all TruthRaw invariants intact.

Likewise a shadow PASS is diagnostic evidence only. It does not grant authority to change the Scientific Master.

The promotion gate requires both sides at once:

- **physical validation authority** — the model passed genuinely held-out measurements and its uncertainty model passed the predeclared acceptance protocol;
- **pipeline conservation authority** — source/master identity, censoring, GainMap exactly-once behavior, capture-domain handling and 1/2/4-worker invariance all remained correct in shadow execution.

## 2. Exact identity chain

A promotion request must bind one exact admitted claim. At minimum the gate cross-checks:

- `sourceEvidenceSha256`;
- `packId`;
- `datasetManifestSha256`;
- `protocolSha256`;
- `modelSha256`;
- `bindingSha256`;
- `quantity`;
- `modelId`;
- `uncertaintyModelId`;
- `validDomainSha256`;
- `validationReportSha256`;
- `uncertaintyReportSha256`;
- `acceptanceProtocolId`;
- `captureSampleDomainId`;
- `physicalFrameCount=1`;
- `independentEvidenceCount=1`.

Changing calibrated numeric model bytes while keeping the human-readable model name therefore invalidates the promotion chain.

## 3. Held-out physical validation

The exact physical-validation artifact whose canonical SHA-256 is named by the admitted claim must say PASS and must prove:

- the requested quantity is the same quantity;
- the exact `modelId` is the tested model;
- fit captures were not reused as held-out validation captures;
- the acceptance protocol was sealed before the final fit;
- the tested `captureSampleDomainId` covers this scene binding;
- the acceptance protocol matches the claim's `acceptanceProtocolId`.

The gate does not invent numerical thresholds. Those belong to the predeclared acceptance protocol and validation report.

## 4. Uncertainty validation

The exact uncertainty report whose SHA-256 is named by the admitted claim must separately PASS.

It must bind:

- the same quantity;
- the same `uncertaintyModelId`;
- a scope-bound valid domain;
- successful declared-coverage/acceptance testing.

A point estimate without a validated uncertainty model is not enough for production `CALIBRATED_PHYSICAL` eligibility.

## 5. Shadow-conservation requirements

The shadow report must remain explicitly `shadowOnly=true` and `productionRouteChanged=false`.

It must prove:

- exact `bindingSha256` match;
- exact model/protocol SHA match;
- source identity regression PASS;
- production Scientific Master identity unchanged before/after shadow evaluation;
- source clipping/censor state preserved;
- no double GainMap/lens-shading correction;
- capture/sample-domain regression PASS;
- all predeclared comparison thresholds PASS;
- threshold protocol equals the claim acceptance protocol;
- exact result fingerprints for 1, 2 and 4 workers are identical.

For a quantity whose calibration only changes noise/uncertainty, such as `read_noise`, the shadow report must additionally prove that the signal coordinate is bit-identical. A noise calibration is not allowed to move scene signal merely because it has a better noise model.

## 6. Dependency completeness

The gate reads the authoritative `promotionRules` from `TRUTHRAW_FOTOGRAAF_CALIBRATION_CONTRACT_V0_1.json`.

Every required C-module for the quantity must be present and validated in the exact pack.

Examples:

- `read_noise` requires C0 + C1;
- `capture_dynamic_range` requires C0 + C1 + C2;
- `relative_scene_radiance` requires C0 + C1 + C2 + C3 + C5;
- `absolute_scene_radiance` additionally requires C6;
- `validated_incident_light_inference` requires C0 + C2 + C7;
- `absolute_incident_irradiance` requires C0 + C1 + C2 + C3 + C5 + C6 + C7.

For absolute quantities, physical units and traceability remain mandatory. A unit string alone is not traceability.

## 7. What a PASS means

A gate PASS produces:

`ELIGIBLE_FOR_EXPLICIT_PROMOTION_REVIEW`

and a digest-bound eligibility record.

It means the supplied calibration evidence chain is structurally coherent and has passed all declared validation/conservation gates required by v0.1.

It does **not** mean:

- the gate independently repeated the laboratory experiment;
- the calibrated model is automatically activated in production;
- the Scientific Master changed;
- the Backplane/certificate v0.1 schema changed;
- calibration frames became scene evidence;
- source clipping was recovered;
- product HDR became physical HDR authority.

A separate explicit production-wiring change and regression review is still required after eligibility.

## 8. Failure policy

Any missing/mismatched link leaves the claim shadow-only or rejects promotion. In particular:

- model/protocol hash mismatch: reject;
- validation/uncertainty report hash mismatch: reject;
- fit/held-out leakage: reject;
- worker mismatch: reject;
- source/master identity regression failure: reject;
- double correction: reject;
- source censor loss: reject;
- wrong capture/sample domain: reject;
- missing dependency module: reject;
- missing traceability for an absolute claim: reject;
- changed scene evidence counts: reject.

There is no `best effort` physical promotion path.

## 9. Current project status after this step

This gate is designed to let TruthRaw finish the **authority architecture** before changing scientific reconstruction/measurement production code.

Therefore the intended status remains:

- CalibrationPack contract: validation infrastructure;
- Scene Admission: runtime exact-scope binding;
- MeasurementLab shadow adapter: side-by-side diagnostic evaluation;
- Physical Promotion Gate: eligibility decision;
- production calibrated Scientific Master path: **BLOCKED until real controlled calibration evidence passes the complete chain and an explicit production-wiring review is made**.

This ordering follows the project's conservation principle: diagnose, bind, validate and quantify uncertainty before changing authoritative scientific state.
