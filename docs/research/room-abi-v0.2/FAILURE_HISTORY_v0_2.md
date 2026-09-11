# TruthRaw Room ABI v0.2 — failure history

## Run 34548027485 — initial all-room GCC failure

- Head: `9968d9865b3663ba6700179a8c9293faa681d827`
- Upstream integrity: PASS
- GCC Release: FAIL
- Clang / ASan+UBSan: skipped after GCC failure

Review of the new integration test found an unqualified `Status::Ok` inside helpers that also imported `building_runtime::v0_1`; both Room ABI v0.2 and Building Runtime define `Status`. The correction explicitly qualified Building Runtime status values.

No budget, evidence, ISO, zero-line or scientific gate was relaxed.

## Corrected run 34548187649 — PASS

- Head: `3daf140f3df0be0f38bdc14c82a4b791b92776ee`
- Upstream integrity: PASS
- GCC Release: PASS
- Clang Release: PASS
- ASan+UBSan: PASS

Documentation Governance run `34548187651` on the same head also passed.

## Run 34549927676 — concrete streaming-sink GCC failure

- Head: `82164be04c9550334c6a372fa989204af63f0a50`
- Upstream integrity: PASS
- GCC Release: FAIL
- Clang / ASan+UBSan: skipped after GCC failure

This failure occurred after adding the concrete file-backed `IStreamingSink` and the end-to-end `TileNativeDngSource -> StreamingTruthRawProcessor -> BoundedRecordFileSink` test.

The sink implementation itself compiled. The new test brought canonical `truthraw::Status` and `room_abi::v0_2::Status` into the same scope, so an unqualified `Status::Ok` in the Room ABI endpoint assertion was ambiguous.

The correction fully qualified:

`truthraw::room_abi::v0_2::Status::Ok`

The verification was tightened at the same time: instead of reading the complete output transport back into a vector, the test now verifies file size and only the fixed 8-byte `TRSINK01` magic. This keeps the validation itself consistent with bounded-streaming intent.

No source-evidence, reconstruction, uncertainty, ISO, zero-line, Backplane, memory-budget, single-frame or scientific-authority gate was relaxed.

## Corrected concrete streaming implementation — PASS

- Validated implementation head: `7d2e1a3829117cde559eef38dad07ca7a8ecc0e8`
- Push run: `34550111870` — PASS
- PR run: `34550113743` — PASS
- Upstream integrity: PASS
- GCC Release: PASS
- Clang Release: PASS
- ASan+UBSan: PASS
- Documentation Governance push run: `34550111956` — PASS
- Documentation Governance PR run: `34550113839` — PASS

The successful end-to-end test reports:

- source resident bound: `4812` bytes;
- sink resident bound: `65536` bytes;
- output transport: `14336` bytes;
- SDR records: `12`;
- half-log-gain records: `12`;
- Stage-2 diagnostic records: `12`;
- adapter-owned full-frame buffers: `0`;
- scene ISO axis: absent;
- physical frame count: `1`;
- independent evidence count: `1`.

This is host-CI proof for the bounded streaming contract. It is not Android RSS/thermal/throughput proof and does not establish a production DNG/JPEG/HEIF/AVIF encoder.
