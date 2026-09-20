# TruthRaw Open Scene Channel Authority v0.78

Status: **integration candidate — richer authority representation, fail-closed by default**

v0.78 does not replace Open Scene v0.70. It is a child authority sidecar bound to the exact v0.70 artifact identity.

## Key correction

Authority and uncertainty knowledge are separate axes.

A direct CFA sample may remain `CALIBRATED_ESTIMATE` while its uncertainty state is explicitly `UNRESOLVED`. This is more precise than either pretending zero uncertainty or discarding the known source relationship.

A missing channel may become `RECONSTRUCTED` only when:
- an uncertainty binding SHA-256 is present;
- the binding explicitly permits reconstructed authority;
- finite nonnegative p95 is present;
- support is finite and positive;
- the reconstruction backend id is bound.

Without those conditions it stays `UNKNOWN`.

A censored direct CFA channel carries an explicit lower bound. The generic source-only adapter records the source RAW WhiteLevel bound in `SOURCE_RAW_CODE` domain rather than inventing an exact latent scene-linear value.

## Current generic Android meaning

For arbitrary admitted DNG:
- uncensored direct CFA channel: CALIBRATED_ESTIMATE / uncertainty UNRESOLVED / support=1;
- clipped direct CFA channel: CENSORED / source-code lower bound;
- two missing channels: UNKNOWN;
- RECONSTRUCTED count: zero.

Therefore v0.78 is a representational improvement, not an authority promotion.

## Exact tele v5.0g

The repository contains a prospective PASS for the exact historical HONOR BKQ-N49 4080x3072 vendor-DNG tele source class + v4.7i backend. It must not be transferred automatically to the v0.73 camera-derived DNG container.

The recovered exact v5.0g feature semantics have passed replay v0.2, but the current F64 reconstruction trace binding remains an explicit open gate. v0.78 therefore provides the admission slot without bypassing that gate.
