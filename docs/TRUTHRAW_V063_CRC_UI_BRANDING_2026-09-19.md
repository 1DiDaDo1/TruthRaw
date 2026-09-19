# TruthRaw v0.63 — corrected Backplane CRC + branded product UI

Date: 2026-09-19

Status: **IMPLEMENTED / HOST GCC + CLANG + ANDROID CI GREEN / APK VERIFIED**

Active branch:

`integration/truthraw-suite-v0-63-crc-ui-branding`

App version:

`0.28-v0.63-crc-ui-branding`

## Scientific correction first

v0.62 proved that the saved PURE Float32 DNG was using the new self-binding route, but inspection of a real artifact exposed that the additional text field `technical_backplane_crc32` was computed over all 180 serialized Backplane bytes. Because the last four bytes already contain the canonical internal CRC32, CRC32 over all 180 bytes produced the constant residue `0x2144df1c` rather than the record-specific Backplane CRC.

v0.63 corrects this before any interface work is treated as complete.

The canonical Technical Backplane v0.1 format remains frozen at 180 bytes. Its internal CRC is the CRC32 of bytes 0..175 and is stored little-endian at bytes 176..179.

The PURE private contract is versioned to:

`TRUTHRAW_PURE_SELF_BINDING_V0_63`

The DNG now records:

`technical_backplane_crc_scope=PREFIX_176_BYTES`

and `technical_backplane_crc32` is computed over exactly those 176 bytes.

## Content-level post-write verification

The Android post-write verifier no longer checks only the textual CRC shape. After reopening the exact saved destination DNG it now:

- decodes the full 180-byte Technical Backplane from `technical_backplane_serialized_hex`;
- verifies magic `TRBACK01`, version 1 and serialized length 180;
- verifies `forbiddenFlags=0`;
- verifies physical frame/evidence count 1/1;
- verifies reserved bytes 165..175 are zero;
- checks that source, Scientific Master, Zero-Line and scene-scale hashes inside the Backplane exactly match the textual DNG self-binding fields;
- recomputes CRC32 over bytes 0..175;
- compares that recomputed CRC with the internal little-endian CRC stored at 176..179;
- compares the same recomputed CRC with the DNG `technical_backplane_crc32` metadata field;
- fails closed and attempts to delete the destination if any comparison fails.

The old constant-residue behavior is explicitly rejected by writer regression tests.

## User interface and icon

Only after the CRC correction was implemented, v0.63 applies the intended product interface direction.

The launcher now presents two clear primary input routes:

- **Open RAW / DNG**
- **Gebruik camera**

Both still converge on the same existing sealed-source admission; this is a UI/navigation change, not a scientific split.

The output preference area presents:

- JPG — real finalized preview route;
- JPG XL — visible but disabled until actually admitted;
- TRUTHRAW PURE — real 32-bit Float DNG route and default;
- TRUTHRAW ADVANCED — visible but disabled while still research-only.

Selecting JPG or TRUTHRAW PURE changes only which real export action is shown first after processing. It does not alter reconstruction, color authority, TruthRange, Zero-Line, Backplane or evidence.

All historical research/test entry points that were previously visible on the launcher have moved behind the settings gear into a dedicated **Onderzoek & diagnostiek** screen.

The generated TruthRaw icon is also applied to the Android application and to the product header. Its TR monogram contains the requested camera-lens/open-world reflection motif.

## Scientific invariants

v0.63 does not change:

- sealed source evidence;
- Scientific Master pixel mathematics;
- Float32 PURE projection mathematics;
- Zero-Line/L0 semantics;
- scene-scale semantics;
- frozen 180-byte Technical Backplane layout;
- physicalFrameCount=1;
- independentEvidenceCount=1;
- the distinction between measured, reconstructed and appearance data.

## CI and APK validation

v0.63 workflow run `35469414682` passed after the UI receiver-shadowing compile fix:

- host GCC: **SUCCESS**;
- host Clang: **SUCCESS**;
- Android arm64: **SUCCESS**;
- artifact ID: `10592980623`;
- artifact name: `truthraw-suite-v0-63-crc-ui-branding-debug-arm64`;
- artifact ZIP SHA-256: `f8d24306b33d2141b4eb3982949637d1aa4ab84585d253a4efed70708d149f2d`;
- extracted APK bytes: `5874361`;
- extracted APK SHA-256: `82dc3103b4e945e129fa5ea38fe717a342d459d1f6b71e4895bab9e00282d173`.

APK inspection confirms the packaged TruthRaw icon and the native markers `TRUTHRAW_PURE_SELF_BINDING_V0_63`, `technical_backplane_crc_scope=PREFIX_176_BYTES`, `technical_backplane_crc32=` and the exact serialized Backplane field.

The remaining evidence gate is real-device output: export a new v0.63 PURE DNG and independently verify that its declared CRC equals the internal Backplane CRC recomputed over bytes 0..175.
