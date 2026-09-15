# TruthRaw Project Update Discipline — 2026-09-15

## Purpose
TruthRaw research, code, evidence, status, and handoff documentation must remain synchronized. A new chat must not have to reconstruct material findings from conversation history when those findings already changed the project state.

## Rule
For every material project step, update the repository in the same work cycle.

A material step includes any of the following:
- new measurement or real-file evidence;
- new scientific conclusion or falsification;
- new code path, numerical backend, calibration path, reconstruction path, or export path;
- changed authority/promotion status;
- discovered discrepancy, regression, provenance problem, or unresolved scientific question;
- changed current-next-step.

## Minimum synchronized update
A material step should update, where applicable:
1. implementation or research code;
2. machine-readable evidence/status;
3. human-readable report/status;
4. bootstrap/handoff/current-next-step when the project-wide state changed.

Do not let README/status language silently outrun code or evidence. Do not let code/evidence advance while START_HERE/handoff continues to direct a new chat to an obsolete step.

## Evidence law
Repository synchronization never promotes evidence by itself. Keep these states separate:
- observed/measured;
- reconstructed/derived;
- research-only;
- CI-validated;
- device-validated;
- independently calibrated;
- promoted/canonical.

Unresolved discrepancies must be written down rather than normalized away.

## Free Scientific Space
The original RAW/CFA is evidence, not a numerical prison. TruthRaw reconstruction may use a freer scientific representation, including higher precision, expanded dynamic range, different lattices or color coordinates. Knowledge claims remain bounded by evidence even when representation is not.

## Current application
The precision-independent Scientific Master work therefore keeps the repository synchronized while resolving:
- the aggregate mixed-precision reconstruction counts discrepancy;
- the binding of real local uncertainty/covariance to the F64-compute -> optional-F32-storage audit;
- later transfer of the proven precision policy to the physical-ID5 maximum-resolution/200MP route.
