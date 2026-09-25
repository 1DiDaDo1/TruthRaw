# TruthNegative Native Container v0.1

Status: **EXECUTABLE EXPORT/IMPORT CONTRACT**

The native container serializes the raster-independent scientific-negative source plane without reducing it to a display raster.

A fixed 4096-byte header binds source SHA-256, Scientific Master SHA-256, Open Scene authority-field SHA-256 and TruthNegative Continuous state SHA-256. The body stores canonical 64x64 Open Scene Field v0.85 encoded tiles. Those records contain the Float32 scientific value plus role, local authority, uncertainty/support and censor bounds.

The writer is random-access/streaming: it reserves the header, writes tile chunks, hashes the body, then patches the finalized header. It does not need to materialize the whole scientific negative in RAM.

The reader verifies the body digest, every tile payload digest, decodes every tile fail-closed, and exposes the imported container again as IFieldTileSource.

The host test performs a full export -> import -> record-by-record bit-identity round trip and rejects payload corruption.

This is the Step C/D foundation. Android export can now wrap this contract with an fd-backed sink and immediately reopen the written file for a native post-write import verification.

## Device-evidence follow-up — 2026-09-25

Two real Camera-5 Native v0.1 exports were admitted for structural inspection. Both are 4080x3072, 3072 canonical tiles and 37,601,280 channel records, but have different source/master/field/state/body identities. This confirms they are independent source-derived containers rather than duplicate artifacts. The previous TN-4 sample exposed 12,532,803 CALIBRATED_ESTIMATE, 957 CENSORED and 25,067,520 UNKNOWN channel records. Native v0.1 now writes an explicit role/authority/uncertainty/support/bound and negative/>1 value census into its sealed header so future device exports make this distinction directly inspectable without interpreting file size as scientific information.

The smaller Native v0.1 file size relative to TN-4 is not an authority loss claim: v0.1 intentionally carries the Float32 value plane plus canonical Open Scene Field payload and identity hashes, whereas TN-4 contains additional historical/backplane/open-scene serialization. Scientific equivalence is established by record/state round-trip, never by byte count.


## Camera-5 censor / RAW-code-bound audit — 2026-09-25

Real-device census evidence showed that CENSORED is capture-dependent rather than a fixed Camera-5 pixel count. The Native v0.1 diagnostic path therefore records, without changing scientific authority, the censored R/G/B census, the number of canonical tiles containing censored source samples, their source-coordinate bounding box, the minimum/maximum SourceRawCode bound carried by those records, and a fail-closed bound-domain mismatch count.

The audit preserves the existing scientific rule: a source-measured CFA sample may be CENSORED while its Float32 Scientific Master value remains outside [0,1]. A scene-linear value above 1.0 is not itself evidence of clipping. CENSOR authority continues to originate from the measured RAW sample reaching the admitted source white-level criterion. The spatial/bound diagnostics are evidence checks only; they do not rewrite values, promote reconstructed channels, create new evidence, or mutate the Scientific Master.

Implementation checkpoint: `59160a205e2964535b44a9461b0ca07d0fb3063f`. Host GCC, Clang and ASan/UBSan Native Container/Camera-5 gates are green at this checkpoint. Android APK integration must additionally pass before a device build is released.
