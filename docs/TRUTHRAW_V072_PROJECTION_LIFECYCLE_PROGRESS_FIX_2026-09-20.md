# TruthRaw v0.72 — projection lifecycle + visible progress fix

Date: 2026-09-20

Branch:

`integration/truthraw-suite-v0-72-projection-lifecycle-progress-fix`

App version:

`0.37-v0.72-projection-lifecycle-progress-fix`

## Real-device symptom reproduced from user evidence

The v0.71 UI showed two contradictory states around TIFF projection:

1. a fresh projection could immediately be labelled:
   `Onderbroken TIFF-projectie opgeruimd; geen gedeeltelijk doelbestand blijft geldig.`
2. a second attempt could then enter:
   `TIFF: .trr lineage + derivative digest worden geverifieerd; full-resolution staging draait.`

The same screen gave no continuously visible elapsed-time/progress state, so a long-running full-resolution projection looked stalled. A subsequent OpenEXR create-document flow could also return to a recreated/other MainActivity state because the pending format existed only in RAM.

## Root cause

`MainActivity.onResume()` calls `RestorationProjectionJobStore.recoverInterruptedIfNeeded()`.

v0.71 persisted a non-terminal STAGING job before `startForegroundService()`, but the in-memory
`RestorationProjectionForegroundService.isRunning` flag only became true inside
`Service.onCreate()`.

There is therefore a valid scheduling window:

`JobStore.begin(STAGING)`
→ `startForegroundService()`
→ **MainActivity resumes**
→ `isRunning == false`
→ recovery incorrectly classifies the brand-new projection as stale
→ staging + destination cleanup.

This explains the first-attempt cleanup message without requiring a TIFF writer failure.

A second weakness was that `pendingProjectionFormat` was only an Activity field. If Android recreated MainActivity while the system document picker was open, the returned destination no longer had a reliable DNG/TIFF/EXR format identity.

A third weakness was that the UI allowed another projection picker to be opened while a previous DNG/TIFF/EXR foreground job was still non-terminal.

## v0.72 lifecycle correction

### STARTING phase + startup grace

Projection job phases now include:

`STARTING -> STAGING -> COMMITTING -> VERIFYING -> SUCCESS/FAILED`.

`begin()` uses synchronous preference commit for the initial transaction record and stores both:

- `startedAtMs`;
- `updatedAtMs`.

Recovery will not clean a non-terminal job during the first 30 seconds after its latest startup state unless the state has genuinely become stale.

### Foreground-service race closure

`RestorationProjectionForegroundService.start()` marks `isRunning=true` immediately before
`startForegroundService()`, rather than waiting for `Service.onCreate()`.

If Android rejects service startup, the flag is reset and the target/staging are safely cleaned.

This removes the MainActivity resume window that caused a fresh projection to be mistaken for an interrupted one.

### Exactly one projection at a time

A non-terminal projection now blocks a second DNG/TIFF/EXR launch regardless of the transient
`isRunning` flag.

MainActivity checks the persistent job first and refuses to open a second create-document picker until the current projection reaches a terminal phase.

All three projection buttons are disabled while a projection is active.

### Visible foreground state

While a projection is non-terminal MainActivity now renders:

- indeterminate progress indicator;
- selected format;
- continuously running elapsed-time chronometer;
- current transaction phase;
- explicit text that the foreground service continues processing.

Terminal SUCCESS/FAILED remains persisted and is rendered after completion.

The existing foreground notification remains the second completion signal.

### Picker/process recreation

The selected DNG/TIFF/EXR projection format is now persisted before opening
`ACTION_CREATE_DOCUMENT`.

On result it is consumed from RAM **or** persisted picker state.

If MainActivity was recreated while that picker was open, v0.72 can rebuild the selected source handle from the already successful full-resolution Restoration transaction and preserve its original TruthRaw job id before resuming the projection flow.

This specifically prevents the last-format flow from silently returning to an unrelated/empty screen state.

## User artifact inspection

The uploaded v0.71 full-resolution Restoration artifact was independently parsed in the development session:

TRR:

- bytes: `162,996,224`;
- SHA-256: `8f1cca5ba3792142ea0abfaab40c8a3f8d02e74769381c933f667a725a50f7bf`;
- raster: `4080×3072`;
- canonical tiles: `3072`;
- preserved: `12,517,905`;
- censored source pixels: `15,855`;
- restored role-1 pixels: `15,855`;
- unresolved role-2 pixels: `0`;
- changed components: `47,561`;
- exact role-mask bytes: `12,533,760`;
- declared/recomputed role-mask SHA-256:
  `8cf0f1bff562c20190f53e7cc6969d7d6d7cf7d855df999c3b479cb09f2211ae`;
- canonical Open Scene artifact SHA-256:
  `2b0f4f8c4587dac4dddf1dc2df636003b3432d48f3b12d12db4d5f9afd001bc9`.

The complete role raster was reread from all 3072 TRR records. Recomputed counts were exactly:

- role 0: `12,517,905`;
- role 1: `15,855`;
- role 2: `0`.

The uploaded v0.71 Restoration Float32 DNG was also parsed:

- bytes: `163,556,236`;
- SHA-256: `6e58e785872963da8025bac248039d9c0d581ecfda2d2a92eb531bc46075be2d`;
- classic TIFF magic valid;
- raster: `4080×3072×3`;
- 32-bit sample tags present;
- PhotometricInterpretation = LinearRaw;
- `3072` canonical 64×64 tiles;
- every tile byte count = `49,152`;
- final tile end offset equals exact file size;
- DNGPrivateData contains `TRUTHRAW_ROLE_MASK_BINARY_V1`;
- embedded role-mask payload length = `12,533,760`;
- embedded mask SHA-256 =
  `8cf0f1bff562c20190f53e7cc6969d7d6d7cf7d855df999c3b479cb09f2211ae`;
- embedded role counts exactly match the TRR.

Therefore the uploaded DNG itself **did finish** and its full role-mask binding matches the uploaded TRR. The observed problem is principally lifecycle/progress presentation, not evidence that this DNG write was truncated.

## Scientific status

The uploaded source exercises an effectful Restoration path:

`censored_source_pixels=15,855 > 0`.

All censored sites in this artifact received role 1 and none remained role 2. This is materially stronger than the earlier zero-censored real-device artifact.

However, without independently replaying the original sealed source DNG alongside the TRR, this artifact-only inspection does not by itself close every possible empirical rule about base-pixel equality. The project therefore records this as:

`EFFECTFUL_RESTORATION_ARTIFACT_VALIDATED__SOURCE_REPLAY_STILL_DESIRED`.

## Non-regression

v0.72 changes Android projection lifecycle and status presentation only.

It does not change:

- PURE v0.63 pixel mathematics or self-binding;
- canonical Open Scene v0.70 state semantics;
- v0.67 Restoration eligibility/support math;
- v0.71 role-mask embedding;
- evidence counts;
- scientific writeback rules.
