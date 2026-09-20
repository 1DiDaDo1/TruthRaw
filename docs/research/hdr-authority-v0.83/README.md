# TruthRaw v0.83 — HDR Authority Contract

Status: **research/integration candidate**

Schema:

`TruthRawHdrAuthorityContract/0.83`

v0.83 separates **presentation HDR** from **scientifically admitted HDR gain**.

It does not initially change the current gain-map pixels.

## Why this gate is needed

The current Advanced HDR path is a presentation/appearance transform. It is useful,
but a positive gain value is not automatically evidence of additional scene radiance.

Scientific HDR needs a separate authority gate.

## Required inputs

The v0.83 contract binds:
- sealed source evidence;
- Scientific Master;
- canonical Open Scene;
- v0.78 per-channel authority state;
- v0.79 uncertainty-admission decision;
- v0.82 illumination-state identity;
- current presentation HDR enable/gain count;
- censor suppression state;
- evidence count 1/1.

## Scientific admission rules

Scientific HDR gain is allowed only if all are true:

1. per-output-channel authority is available;
2. no gain-driving channel is UNKNOWN;
3. every RECONSTRUCTED gain-driving channel has admitted uncertainty;
4. CENSORED support is suppressed from exact-value gain;
5. source/master/open-scene/authority/uncertainty/illumination identities are all bound.

Current Advanced does **not** yet have a canonical per-output-channel authority map,
therefore current scientific HDR remains blocked with:

`NO_PER_OUTPUT_CHANNEL_AUTHORITY`

The existing HDR remains:

`APPEARANCE_ONLY`

## Illumination boundary

v0.82 illumination state is context only.

Even a known source-bound white point, CCT or Duv cannot create HDR headroom.

A future calibrated SPD also cannot bypass RGB/channel authority, censor bounds,
uncertainty or output mapping.

## Censor/unknown law

- CENSORED is a bound, never an exact recovered latent value.
- UNKNOWN never gains scientific HDR headroom.
- presentation gain may exist as appearance, but it cannot be relabeled scientific gain.
- no new physical frame/evidence item is created.

## Forward-compatible states

The contract can later admit:
- `DIRECT_EVIDENCE_BOUND` when per-output authority is present and no reconstructed channel is needed;
- `RECONSTRUCTION_UNCERTAINTY_BOUND` when reconstructed channels are locally mapped and their uncertainty is admitted.

Those states are reachable in tests but are not the current generic/camera-DNG runtime result.
