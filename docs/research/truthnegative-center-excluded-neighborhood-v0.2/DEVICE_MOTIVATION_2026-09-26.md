# N2 Reconstruction-Support Closure — Real-Device Follow-up 2026-09-26

Status: **USER-DEVICE OBSERVATION — SCREENSHOT/EXPORTED-SIDECAR BOUND**

## Purpose

This note records why the next N2 research step moved from output protection
to local-estimate quality.

It does not create camera evidence and does not alter Direct CFA, Scientific
Master, TruthNegative, authority or the existing N2 v0.1 candidate path.

## Closure result

The previously observed failure mode was:

    protected CFA source site
      -> nearby admitted N2 correction
      -> measured-preserving full-colour reconstruction support
      -> reconstructed RGB at protected output site changed

The reconstruction-support closure suppresses otherwise admitted N2 Stage-2
corrections inside the reconstruction backend's required support radius and
then requires every protected output RGB channel to remain Float32 bit-identical
to baseline.

The current F64 reconstruction backend reports:

    requiredHalo() = 3

A diagnostic crop fails closed if any protected core RGB channel changes.

## Real-device follow-up A

A Camera-5 admitted 4080x3072 DNG completed the full-colour diagnostic with
`baseline-mismatch=0` in all three selected crops while the support closure
was active.

The structure crop no longer placed its largest displayed delta directly on
Structure-protected support. The highlight crop likewise kept the largest
displayed delta away from the nearest censor/boundary support.

The quiet crop still showed a large amount of candidate activity and a
substantial display-gradient-energy decrease. This shifted the open question
from "does protected output change?" toward "is the surviving local estimate
actually separating sensor noise from fine scene variation?"

## Real-device follow-up B

A second, more structured Camera-5 scene provided a stronger closure stress
test.

### Quiet / mixed crop

Source crop x=768, y=384, 192x192:

- sampled: 36864
- v0.1 candidate: 27892
- structure-protected: 8969
- support-closed candidate Stage-2 sites: 14714
- changed display pixels: 11303
- baseline mismatch: 0
- display gradient-energy B/A: 0.978719

Thus 13178 otherwise admitted v0.1 candidate sites did not survive the
reconstruction-support closure in this mixed quiet/edge crop.

### Structure crop

Source crop x=3888, y=1216, 192x192:

- v0.1 candidate: 507
- structure-protected: 35610
- support-closed candidate Stage-2 sites: 0
- reconstructed RGB channels changed: 0
- changed display pixels: 0
- baseline mismatch: 0
- display gradient-energy B/A: 1.000000

The source-site gate still found 507 potential corrections, but none were
allowed into the private candidate Stage-2 after support closure.

### Censor / highlight crop

Source crop x=2176, y=896, 192x192:

- v0.1 candidate: 29
- CENSORED / boundary: 32003 / 2104
- support-closed candidate Stage-2 sites: 0
- reconstructed RGB channels changed: 0
- changed display pixels: 0
- baseline mismatch: 0
- display gradient-energy B/A: 1.000000

Again, candidate discovery did not imply candidate application.

## Consequence

The support-closure mechanism is retained unchanged.

The next research question is the quality of the local signal predictor at
candidate centers that remain eligible outside protected reconstruction
support.

The current v0.1 authority-aware neighborhood uses the observed center value
when deciding neighbor compatibility. The v0.2 research branch therefore
tests a stricter **center-excluded** predictor:

    neighbors only
      -> symmetric H/V/diagonal support
      -> directional agreement
      -> multiscale agreement
      -> local estimate

Only after that estimate exists may the observed center re-enter as a residual.

v0.2 remains audit-only. It does not replace v0.1, does not change B and does
not weaken the already proven support-closure or exact Scientific-Master
baseline gates.

## Permanent invariants

- Direct CFA immutable
- Scientific Master immutable
- TruthNegative immutable
- one physical frame / one independent evidence item
- createsNewEvidence=false
- scientificWritebackAllowed=false
- v0.2 candidate-applied=false
- encoded-display gradient energy remains diagnostic only, never an MTF claim
