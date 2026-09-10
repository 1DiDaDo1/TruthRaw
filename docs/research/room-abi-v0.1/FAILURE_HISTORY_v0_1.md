# Room ABI v0.1 — Failure History

Failures are preserved as engineering evidence and are not rewritten as passes.

## F1 — 8 MiB automatic stack arena

**Result:** FAIL before ABI execution.

The first local harness allocated an 8 MiB fake memory arena as an automatic stack object. The process exhausted the stack before exercising the ABI. The harness was corrected to static backing storage; production ABI limits were not weakened.

## F2 — stale Room Capsule planner stub

**Result:** expected FAIL after peak-accounting hardening.

The first ABI draft checked only explicit Room Capsule lease bytes. Exact upstream inspection showed `estimatedPeakBytes` also includes fixed metadata/headroom. After the ABI was corrected to reserve the full planner peak, the old stub failed because it modeled only the concrete buffers. The stub was updated to the verified upstream formula. Stub success is not real integration evidence.

## F3 — host overgrant above runtime budget

**Result:** contract gap found during pre-CI audit; corrected before repository candidate.

A request with `minimumBytes <= budget < preferredBytes` could previously permit a host to grant the preferred capacity above the runtime budget. v0.1 now validates the actual granted capacity against the effective room budget and releases a violating active lease. The new negative fixture passes only when the overgrant is rejected.

## F4 — wrong truth-floor authority was not explicitly bound at the ABI

**Result:** contract gap found during pre-CI audit; corrected before repository candidate.

A valid-looking corridor token with correct evidence counts but `Scene` floor could reach the generic lease layer for a Room Capsule execution context. Room Capsule lease acquisition now requires `TruthFloor::Counterfactual` before any host request.
