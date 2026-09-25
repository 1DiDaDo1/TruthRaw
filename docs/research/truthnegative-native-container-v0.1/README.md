# TruthNegative Native Container v0.1

Status: **EXECUTABLE EXPORT/IMPORT CONTRACT**

The native container serializes the raster-independent scientific-negative source plane without reducing it to a display raster.

A fixed 4096-byte header binds source SHA-256, Scientific Master SHA-256, Open Scene authority-field SHA-256 and TruthNegative Continuous state SHA-256. The body stores canonical 64x64 Open Scene Field v0.85 encoded tiles. Those records contain the Float32 scientific value plus role, local authority, uncertainty/support and censor bounds.

The writer is random-access/streaming: it reserves the header, writes tile chunks, hashes the body, then patches the finalized header. It does not need to materialize the whole scientific negative in RAM.

The reader verifies the body digest, every tile payload digest, decodes every tile fail-closed, and exposes the imported container again as IFieldTileSource.

The host test performs a full export -> import -> record-by-record bit-identity round trip and rejects payload corruption.

This is the Step C/D foundation. Android export can now wrap this contract with an fd-backed sink and immediately reopen the written file for a native post-write import verification.
