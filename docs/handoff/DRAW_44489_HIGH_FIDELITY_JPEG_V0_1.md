# D.RAW High-Fidelity JPEG v0.1

The JPEG output path is downstream presentation only. It consumes the exact full-resolution RGB24 sibling emitted before NV21/JPEG chroma reduction and encodes it at requested quality 100.

Acceptance is fail-closed. The resulting JPEG itself must prove:

- baseline SOF0;
- exact frozen output dimensions;
- three components with sampling factors 1x1 / 1x1 / 1x1 (4:4:4);
- 8-bit quantization tables 0 and 1 containing only value 1 for this admitted Q100 encoder.

No encoder request, UI label or visual appearance is sufficient proof. A candidate failing any check is deleted and cannot be committed.

This output path cannot create evidence, mutate sealed source data, modify Scientific Master or authorize scientific writeback.

---

## 2026-10-07 validated implementation state

### Runtime checkpoint

The exact runtime/code checkpoint validated for this contract is:

`dd2ce23fae9cd563db29b8f589d0ceae23b9fa83`

PR #131 remains open, draft and unmerged. PR #130 / `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f` remains frozen and scientifically untouched.

### Pixel cable

The current downstream cable is:

`existing full-resolution photo renderer -> exact display-oriented RGB24 sRGB sibling -> { Free Raster backing artifact | High-Fidelity JPEG encoder }`

The RGB24 sibling is written from the same native `coreRgb` values used before the legacy NV21 chroma reduction. Free Raster therefore no longer round-trips through JPEG compression.

The High-Fidelity JPEG encoder consumes that RGB24 sibling directly. The old NV21 / Android `YuvImage.compressToJpeg()` path is not the admitted High-Fidelity JPEG encoder path.

### Hard High-Fidelity JPEG contract

The admitted path requires all of the following:

- requested JPEG quality exactly `100`;
- native encoder rejects every requested quality other than `100`;
- baseline SOF0 JPEG;
- exact bound output width and height;
- exactly three components;
- SOF component sampling factors `0x11 / 0x11 / 0x11`, i.e. `1x1 / 1x1 / 1x1` = 4:4:4;
- quantization table selectors `0 / 1 / 1`;
- DQT tables 0 and 1 are 8-bit and every entry is `1` for this admitted Q100 encoder;
- any mismatch deletes the candidate and fails closed before destination commit;
- post-encode SHA-256 and existing destination-commit/current-output gates remain active.

JPEG has no standardized integer field that independently means “quality 100”. Therefore D.RAW does not treat the encoder request as proof. It verifies the concrete encoded structure and the Q100 quantization contract in the produced JPEG itself.

### Contract tests

`HighFidelityJpegContractV01Test` proves at unit-test level that the verifier:

- accepts exact Q100 + 4:4:4 + correct geometry;
- rejects 4:2:0-style `0x22` luma sampling;
- rejects non-Q100 quantization;
- rejects wrong raster geometry.

These are output-contract tests only and create no scientific authority.

### Exact-head build / CI proof

Workflow:

`D.RAW Free Raster v0.3 Finish APK`

Run:

`37681560680`

Exact runtime head checked out by the workflow:

`dd2ce23fae9cd563db29b8f589d0ceae23b9fa83`

The workflow completed successfully, including:

- full-resolution cable verification;
- Kotlin compilation;
- Android NDK / C++ compilation including the High-Fidelity JPEG encoder;
- `:app:testDebugUnitTest`;
- `:app:assembleDebug`;
- APK structure/native-library verification;
- artifact upload.

Gradle result:

- `BUILD SUCCESSFUL`;
- `44 actionable tasks: 44 executed`.

Candidate APK produced by this exact run:

- bytes: `8,815,899`;
- SHA-256: `76c1e4ef90f0745be7b1bad5a8dd709cb28e6699615af52ce8b107ee3d13a9a7`;
- workflow artifact ID: `11509366068`;
- uploaded artifact ZIP SHA-256: `1d22875457c7b6eebc2d5b60e682a9026cc553f4a110ba48d431312db156dd03`.

The workflow's historical runtime-wiring step reported `Runtime wiring already applied.` and did not create a newer runtime commit.

All returned exact-head workflows subsequently completed successfully, including Documentation Governance, Canonical Integrity, Research Live Status, Research Fresh Rerun, Scientific Master Bind Profile, DngCreator Compatibility, Universal Intake, Universal Physical Capture and the returned research/integrity checks.

CI/build evidence proves buildability and the static/tested output contract; it does not create scientific authority.

### Historical quality-96 / 4:2:0 evidence remains valid as history

The 2026-10-07 image-quality audit found the earlier supplied JPEG to be quality 96 with sampling `2x2,1x1,1x1` (4:2:0). That finding remains valid for that earlier file and is not rewritten.

The current implementation state supersedes that older encoder path for new High-Fidelity JPEG candidates, but does not retroactively change historical files or their audit results.

### Physical acceptance still pending

This v0.1 route is now **CODE/CI PASS** for the Q100 / 4:4:4 contract.

It is **not yet classified as a new REAL-DEVICE OUTPUT PASS**. The next physical acceptance must export a fresh JPEG from the new APK on the real device and independently inspect the actual saved file for at minimum:

- SOF0 dimensions;
- sampling factors `1x1 / 1x1 / 1x1`;
- Q100 DQT contract;
- SHA-256 / byte size;
- correct 0-degree geometry;
- correct non-zero rotation geometry, preferably +90 degrees;
- source/route stale-state fail-closed behavior.

Only that physical exported artifact may close the new-device-output acceptance. No visual similarity alone is sufficient.

### Scientific boundary unchanged

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`

The RGB24 sibling, Free Raster representation and High-Fidelity JPEG are all downstream derived presentation outputs. They cannot create MEASURED evidence, promote UNKNOWN, mutate sealed CFA/source bytes, alter Scientific Master, apply a scientific candidate or authorize scientific writeback.
