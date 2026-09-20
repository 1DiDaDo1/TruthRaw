# Finalized Scientific Preview Release v0.3

Status: **INTEGRATION CANDIDATE — science-neutral performance release**

v0.3 keeps the v0.2 finalized-preview contract and changes only the Scientific Master execution binding from v0.2 to the proven v0.3 bounded-stripe implementation.

Required invariants against v0.2:

- exact Scientific Master SHA-256;
- exact L0 binary64 bits and Zero-Line semantics;
- exact 180-byte Technical Backplane;
- exact preview ARGB pixels;
- exact authority/frame/evidence state;
- no new scientific claim;
- fewer logical source-tile calls.

The preview streaming passes remain the existing single-worker full-frame-streaming v0.1 path. This release therefore targets only the Scientific Master/TruthRange source-call overhead observed in the physical v0.73 camera run.

No PURE, TruthNegative, Restoration or Advanced output route is changed by this module.
