# D.RAW Research Live Status v0.2

Validation-only lifecycle/UI repair on top of the v0.2.11 authority-template
candidate and Fresh Rerun v0.1.

## Real-device evidence

A clean v0.2.10 control install reproduced the apparent >2 minute
`PROFILE_ASSEMBLY` hang. The exported Foundation snapshot proved the actual
two-profile computation had already completed in about 41.9 seconds. Leaving
and re-entering the Activity immediately exposed the terminal state.

The root cause is that `onResume()` only arms Research polling when an
operation is already active. If the Research page is resumed first and the user
starts the batch afterward, the successful explicit start did not arm
`researchStatusPoll`. The foreground worker could therefore finish while the
Activity continued to show the last RUNNING frame indefinitely.

## Repair

- after a successful explicit Research batch start, immediately arm the
  existing 2-second status poll;
- reset the UI heartbeat observation before the new attempt starts polling;
- persist `attempt_finished_at_wall_ms` and `attempt_elapsed_ms` in the
  durable Research journal;
- render the Research operation timer from the journal's
  `attempt_started_at_wall_ms` / `attempt_finished_at_wall_ms`, falling back
  to the generic operation timestamps only when journal timing is unavailable;
- use per-operation Research-worker liveness when recovering the Research
  status view.

This makes Activity resume a display synchronization event only. It is no
longer required for a completed foreground Research worker to become visibly
terminal.

## Scientific boundary

No RAW bytes, Scientific Master values, canonical authority records, authority
digest semantics, reconstruction, calibration, correction, candidate
promotion, or scientific writeback are changed.

The profiler generation remains:

`D.RAW/UniversalSourceProfileCache/0.2.11-authority-template-reuse-v1`

The change is orchestration/telemetry only.
