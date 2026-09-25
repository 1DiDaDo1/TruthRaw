# TruthNegative Bounded Residual Estimator v0.1

Status: EXECUTABLE N2 RESEARCH ESTIMATOR — NOT PRODUCTION-PROMOTED.

This is downstream of the Structure Preservation Gate. It never edits Direct CFA, Scientific Master or TruthNegative state.

For an admitted sample it decomposes:
observation = localEstimate + residual

and records exactly:
- original residual;
- removed residual;
- retained residual;
- suppression fraction;
- derived output.

Suppression is bounded by the upstream gate (currently <=75%) and tapers to zero as |residual| approaches 2 sigma. Residuals above 2 sigma are preserved as possible scene structure/outliers.

This is intentionally conservative. The localEstimate is NOT defined by this module: a later neighborhood estimator must prove its own edge/censor/authority semantics. Until then this component is a research primitive only.

createsNewEvidence=false; scientificWritebackAllowed=false.
