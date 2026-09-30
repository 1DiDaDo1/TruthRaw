# D.RAW Dark Chroma Stability v0.6 — exact sampled support distance

Date: 2026-09-29

## Why v0.6 exists

The device-validated v0.5 selective main/wide capture
`1790662450477` produced:

- 15 visible Dark-Chroma candidates;
- 15/15 fine-bound;
- candidate overlap structure fraction ≈ 0.1677;
- candidate interior structure fraction ≈ 0.1678;
- whole-observation N2 structure-protected sample fraction ≈ 0.16974;
- 558 / 0 counted 32×32 fine tiles with/without structure support;
- zero candidate regions with a structure-free sampled interior.

This proves that the next question is not "should the tile be 16×16 instead?"
The scientifically relevant question is where the exact protected N2 samples
lie relative to each candidate.

## v0.6 path

```text
one sealed DNG observation
 -> Dark Chroma v0.1/v0.2/v0.3
 -> v0.4 same-observation N2 binding
 -> v0.5 fine 32x32 structure density
 -> exact N2 sampled Structure/Censor/CensorBoundary coordinates
 -> candidate-center and candidate-rectangle support geometry
 -> Dark Chroma v0.6 diagnostic state

NO correction
NO distance threshold
NO private chroma A/B/Delta
NO Scientific-Master writeback
```

## Metrics

For every visible candidate v0.6 measures:

- exact mapped source rectangle;
- exact mapped source center;
- nearest Structure support from center;
- nearest Structure support to rectangle;
- nearest Censored support from center/to rectangle;
- nearest CensorBoundary support from center/to rectangle;
- sampled and structure counts/densities at center radii 8/16/32/64 px;
- sampled and structure counts/densities at rectangle margins 0/8/16/32 px.

The protected coordinate stream itself is recorded in the native sidecar using
compact base64 U32 x/y pairs and has an independent SHA-256.

## v0.5 parity

Where v0.5 fine support is available, v0.6 verifies aggregate parity for:

- sampled count;
- structure-protected count;
- censored-protected count;
- censor-boundary-protected count.

This is a protection-semantics parity check, not permission to replace v0.5.

## Selective execution

Like v0.5, the new distance audit runs only for:

- native-ready DNG;
- one or more visible Dark-Chroma candidates;
- source not globally `DARK_UNINFORMATIVE`.

Near-black tele and zero-candidate ultra-wide cases therefore remain cheap and
retain their already validated blockers.

## Scientific authority

v0.6 is explicitly diagnostic:

- exact sampled coordinates are evidence only for those sampled locations;
- unsampled pixels stay unknown;
- distances are not probabilities;
- no threshold is admitted;
- distance cannot reduce existing structure protection;
- distance cannot enable correction;
- `DARK_UNINFORMATIVE` cannot be overridden;
- `chroma_correction_supported=false`;
- `private_ab_delta_allowed=false`;
- `candidate_applied=false`;
- `scientific_writeback_allowed=false`.

## Device target

Primary device validation target remains a selective wide/main scene similar to
`DRAW_CAPTURE_1790662450477_wide_main_4096x3072.dng`.

The first device run should establish the actual nearest-support distance
distribution and radius densities. Only after that evidence exists may a later
version ask whether any distance/support threshold is scientifically justified.
