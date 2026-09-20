# TruthRaw v0.68 — transactional foreground Full-Resolution Restoration

Date: 2026-09-20

Branch:

`integration/truthraw-suite-v0-68-restoration-transactional-fgs`

App version:

`0.33-v0.68-restoration-transactional-fgs`

Status: **research integration / APK built / v0.67 lifecycle defect addressed / v0.68 real-device validation pending**.

## Why v0.68 exists

The first real-device v0.67 `.trr` test exposed a lifecycle/atomicity defect in the Android wrapper, not a failure of the scientific restoration rule.

Observed incomplete v0.67 artifact:

`IMG_BNC_TRUTHRAW20260907_094414_423_truthraw_fullres_restoration_v0_67.trr`

Measured facts:

- file bytes: `25,854,576`;
- required complete 4080×3072 v0.67 container size: `162,996,224` bytes;
- initial 8192-byte header: all zero;
- valid final marker/header: absent;
- complete raster records present: `487`;
- complete source pixels represented: `1,987,584`;
- final complete tile: `x=2432..2495, y=448..511`;
- all stored Float32 components in those complete records are finite;
- negative components: `382`;
- >1 components: `2`;
- role-mask counts in the written prefix:
  - `PRESERVE_SCIENTIFIC_MASTER = 1,987,584`;
  - `AESTHETIC_REINTEGRATION_ONLY = 0`;
  - `UNRESOLVED_LOSS = 0`.

The artifact ended exactly on a tile boundary and never reached final header commit/post-write verification. The v0.67 native error paths truncate their own output on an internal failure, so the surviving prefix is consistent with the process/export lifecycle being externally interrupted while writing directly to the user-visible SAF destination.

## Scientific route is unchanged

v0.68 does **not** change the v0.67 restoration science.

The native v0.67 container still requires:

- exact sealed source;
- same Scientific Master;
- exact full Scientific-Master replay;
- Technical Backplane phase 2;
- full source resolution;
- `SOURCE_CFA_SAMPLE_AT_OR_ABOVE_WHITELEVEL` eligibility;
- non-censored support only;
- radius ≤ 2;
- minimum support = 3;
- role 0/1/2 separation;
- no Scientific-Master writeback;
- no new evidence;
- no second scientific world;
- physical frame/evidence = 1/1.

The real-device validated PURE writer remains `TRUTHRAW_PURE_SELF_BINDING_V0_63`.

## New v0.68 transaction model

Full-resolution Restoration no longer writes its long-running reconstruction directly into the final SAF document.

New sequence:

`source`
→ `foreground dataSync service`
→ `app-private staging .part`
→ `native full-res v0.67 reconstruction`
→ `staging contract verify`
→ `whole-file staging SHA-256`
→ `destination zero-header + body copy`
→ `fsync`
→ `VALID 8192-byte header written LAST`
→ `fsync`
→ `reopen exact destination`
→ `contract + size + whole-file SHA-256 equality`
→ `SUCCESS`.

The final header is the validity boundary. Until the body is complete, the destination contains no valid TruthRaw restoration header. If copying is interrupted, the document therefore cannot masquerade as a valid artifact.

## Foreground lifecycle

The export now runs in:

`FullResRestorationForegroundService`

with Android foreground-service type:

`dataSync`.

The service:

- starts before reconstruction work;
- owns the transaction independently from the Activity lifecycle;
- uses a partial wake lock during the long export;
- is `START_NOT_STICKY`;
- stores transaction state in `FullResRestorationJobStore`;
- keeps the user-visible document empty until verified staging is ready;
- updates state through:
  - `STAGING`;
  - `STAGING_VERIFIED`;
  - `COMMITTING`;
  - `VERIFYING`;
  - `SUCCESS` / `FAILED`.

Leaving TruthRaw or switching to another app no longer cancels the export merely because the Activity is paused/stopped.

## Process-death recovery

A hard process kill is treated differently from ordinary backgrounding.

If the app later restarts and finds a non-terminal persisted transaction while no Restoration foreground service is alive:

- the private staging file is deleted;
- the user-visible destination is deleted where supported;
- otherwise it is truncated to zero bytes;
- the job becomes `STALE_CLEANED`;
- the incomplete file is never promoted into a valid artifact.

v0.68 intentionally does not try to resume an unknown partially executed native reconstruction after process death. It fails closed and requires a fresh export.

## Whole-file equality

v0.68 adds a second post-write guarantee beyond the v0.67 header/size checks.

The complete private staging container is SHA-256 hashed after native completion.

After transactional commit, the exact SAF destination is read from start to end and SHA-256 hashed again.

Success requires:

`SHA256(staging) == SHA256(saved destination)`.

Therefore the final artifact is not only structurally valid; it is byte-for-byte identical to the fully verified staging artifact.

## Android permissions

v0.68 declares:

- `FOREGROUND_SERVICE`;
- `FOREGROUND_SERVICE_DATA_SYNC`;
- `WAKE_LOCK`;
- existing `POST_NOTIFICATIONS`.

The user-selected destination write permission is persisted when the document provider offers a persistable write grant.

## CI

Successful run:

`35501207534`

Results:

- host GCC PURE writer: SUCCESS;
- host Clang PURE writer: SUCCESS;
- OpenWorld/Dynamic-Authority/Restoration contract tests: SUCCESS;
- Android arm64 debug APK: SUCCESS.

Artifact:

`truthraw-suite-v0-68-restoration-transactional-fgs-debug-arm64`

Artifact ID:

`10602377966`

Artifact archive digest:

`sha256:a9968ddfd513c342a7f593aad7fec2bece503d43e406896d1118f08aca8b2391`

Extracted APK:

- bytes: `6,121,453`;
- SHA-256: `82a4fad83bc112067be00faf59609698daf742f19b71fcd65608e19e3b8c0d59`.

## Next real-device gate

Install v0.68 and repeat the same known DNG export.

During the test:

1. start `FULL-RES Restoration`;
2. while it is processing, switch to ChatGPT or another app;
3. optionally lock/unlock the screen;
4. return to TruthRaw;
5. wait for the foreground completion notification/state;
6. upload the resulting `.trr`.

Required validation:

- exact full size for its source geometry;
- valid final header;
- all expected tiles;
- source SHA stable;
- Scientific-Master replay valid;
- destination whole-file SHA equals staging SHA reported by the app;
- role counts cover exactly all source pixels;
- only role-1 pixels may differ from base Scientific Master;
- no partial prefix remains after interruption/failure.

Only after this passes should the lifecycle defect be considered closed on real hardware.
