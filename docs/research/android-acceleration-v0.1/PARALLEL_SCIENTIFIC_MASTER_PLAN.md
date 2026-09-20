# Parallel Scientific Master execution plan

Status: research / not selected in production.

## Existing reference

The current Android Scientific Master path uses
scientific-master-streaming-binding-v0.3. It is the reference and remains
unchanged.

v0.3 performs:

1. Pass 1 over runtime stripes:
   - read Stage-2
   - reconstruct camera-native RGB
   - feed the partition-independent canonical 64x64 Scientific Master digest
   - accumulate high-16 Stage-2 median histogram
2. Resolve the high 16 bits of the exact median ranks.
3. Pass 2 over Stage-2-only stripes:
   - accumulate low-16 histograms for the selected high buckets
4. Resolve exact lower/upper median float bit patterns and L0.

The canonical Scientific Master digest already stores leaf hashes by canonical
cell index and explicitly allows runtime tile order and tile size to vary.
Therefore compute order is not part of Scientific Master identity.

## Parallel v0.1 design

Do not modify full-frame-streaming-v0.1 or scientific-master-streaming-binding-v0.3.

Create an Android acceleration wrapper with N independent read-only DNG tile
sources. Every worker owns:

- one TileNativeDngSource opened from the same sealed source bytes
- one v4.7i reconstruction instance
- one streaming Workspace
- private pass-1 Stage-2 high histogram
- private eligible count
- private reconstructed stripe result

### Pass 1

Runtime stripe indices are assigned dynamically.

A worker:
- fills Stage-2 for one stripe
- reconstructs camera-native RGB using the canonical v4.7i backend
- computes private Stage-2 histogram/count contribution
- returns exact Float32 camera-native stripe pixels

The commit thread:
- adds each returned stripe to ScientificMasterDigestAccumulator
- merges integer histogram counters
- records resource metrics

The digest accumulator is already location-addressed by canonical cell, so
commit order must not affect the final root. For first promotion, nevertheless
commit in canonical runtime-stripe order to minimize the proof surface.

### Pass 2

No reconstruction is consumed by v0.3 pass 2. Workers independently read
zero-halo Stage-2 stripes and return only two private uint64 histograms.
The commit thread sums counters exactly. Integer addition is exact and the
resolved median bit patterns must equal v0.3.

## Promotion gates

The parallel path is forbidden from production selection until all gates pass:

1. Same Scientific Master SHA-256 as v0.3 for every fixture.
2. Same exact IEEE-754 bit patterns for lower median and upper median.
3. Same L0 double bit pattern.
4. Same selfGaugeEligibleSamples.
5. Same reconstruction backend and scene binding.
6. Same physicalFrameCount=1 and independentEvidenceCount=1.
7. 1-worker wrapper result equals v0.3 reference.
8. N-worker wrapper result equals 1-worker wrapper.
9. Repeated N-worker runs are deterministic.
10. Memory upper bound includes N independent source/workspace states.
11. Failure of any worker fails closed; no partial result is admitted.

## Worker count

Worker count is provided by TruthRawCpuSchedulingPolicyV01. It is bounded by:
- online CPU count
- measured/declared bytes per worker
- app memory class
- thermal state and thermal headroom
- power-save state
- an initial maximum of 8 workers

No CPU affinity is part of the scientific algorithm.

## Authority

Acceleration changes execution provenance only.

It cannot:
- redefine Source Evidence
- change Scientific Master identity
- change v4.7i semantics
- upgrade reconstructed/censored/unknown authority
- introduce appearance data into the master
