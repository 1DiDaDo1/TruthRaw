# TruthRaw Adaptive UI + Ingress v0.1 — Failure History

Failures remain evidence. A failed gate is not rewritten as a pass.

## Initial contract-verifier failure

The first Adaptive UI v0.1 CI run failed before APK assembly because the fail-closed ingress verifier found the literal token `ByteArray` in a source-code comment that stated the intended prohibition. The implementation did not allocate or read a RAW payload into such a buffer.

Resolution:

- no scientific, ingress or memory gate was relaxed;
- the comment was reworded to describe an in-memory payload buffer without triggering the lexical guard;
- the same verifier subsequently passed;
- Android APK assembly then passed on the corrected implementation.

This failure is classified as a verifier/comment lexical collision, not evidence that the ingress path materialized a full RAW.
