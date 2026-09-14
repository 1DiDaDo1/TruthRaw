# TruthRaw FotoGraaf Calibrated Claim Record v0.1 — 2026-09-14

**Status: RESEARCH AUTHORITY GATE / FAIL-CLOSED**

A `CALIBRATED_PHYSICAL` label is not a boolean success flag. It is a bounded scientific claim that must identify the model, its uncertainty, the domain in which it was validated, the acceptance protocol and the exact validation artifacts.

Every `CALIBRATED_PHYSICAL` claim therefore carries at minimum:

- `modelId` — the exact physical/statistical model used for this quantity;
- `uncertaintyModelId` — the uncertainty propagation/model identity;
- `validDomain` — a non-empty, scope-bound domain description; extrapolation is not silently authorized;
- `acceptanceProtocolId` — the predeclared protocol that decided PASS/FAIL;
- `validationReportSha256` — exact held-out validation report identity;
- `uncertaintyReportSha256` — exact uncertainty report identity.

Absolute quantities additionally serialize a physical `unit`. External traceability remains separately required by C6/C7; a unit string alone is not traceability.

The purpose is to prevent a pack from saying only `validated=true` and then gaining physical authority. The promotion path becomes:

`controlled capture bytes`
→ `dataset manifest SHA-256`
→ `fit model identity`
→ `held-out validation report SHA-256`
→ `uncertainty report SHA-256`
→ `bounded validDomain`
→ `CALIBRATED_PHYSICAL`

Any missing link fails closed.

This record does not alter reconstruction, the Scientific Master, the zero-line, TruthRange or evidence counts. It only controls what scientific claim may be attached to a quantity after calibration.

For incident light, the distinction remains strict:

- a C7-tested directional/relative incident-light inference can become calibrated only inside its tested geometry/material/light domain;
- absolute incident irradiance additionally requires C6 absolute traceability and a traceable cosine-corrected irradiance reference;
- neither class retroactively turns a camera pixel into a direct irradiance measurement.

Permanent interpretation:

**A calibrated value without a model identity, uncertainty model, validated domain and exact validation evidence is not `CALIBRATED_PHYSICAL`.**
