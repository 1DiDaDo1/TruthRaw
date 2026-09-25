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
