# TruthRaw Room ABI v0.2 — failure history

## Run 34548027485 — GCC failure

- Head: `9968d9865b3663ba6700179a8c9293faa681d827`
- Upstream integrity: PASS
- GCC Release: FAIL
- Clang / ASan+UBSan: skipped after GCC failure

Review of the new integration test found an unqualified `Status::Ok` inside helpers that also imported `building_runtime::v0_1`; both Room ABI v0.2 and Building Runtime define `Status`. The correction explicitly qualifies Building Runtime status values.

No budget, evidence, ISO, zero-line or scientific gate was relaxed.

## Corrected run 34548187649 — PASS

- Head: `3daf140f3df0be0f38bdc14c82a4b791b92776ee`
- Upstream integrity: PASS
- GCC Release: PASS
- Clang Release: PASS
- ASan+UBSan: PASS

Documentation Governance run `34548187651` on the same head also passed.
