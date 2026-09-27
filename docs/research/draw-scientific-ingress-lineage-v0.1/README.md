# D.RAW Scientific Ingress Lineage Binding v0.1

Status: **REAL-MAIN LINEAGE CANDIDATE**

This layer binds an immutable physical source to a derived compatibility
ingress and the scientific identities produced from that ingress.

It solves a specific identity problem exposed by the real main-camera DNG:

```text
immutable physical source
  -> metadata-only quarantine container
  -> unchanged common scientific pipeline
```

The quarantine container has different file bytes, so D.RAWnegative v0.1
naturally receives a different pipeline-local observation/gauge identity.
That computational identity must **not** create a second physical observation.

## Two identities, two roles

v0.1 defines:

- `physical_observation_id` — stable identity derived from the immutable
  parent physical-source SHA-256;
- `pipeline_observation_id` — existing D.RAWnegative v0.1 computational
  lineage identity derived from the actual ingress bytes consumed by the
  pipeline.

For the recovered main source:

`DRAW_PHYSICAL_OBS_a85cac…dcea79`

is the physical identity.

`DRAW_OBS_7c8eb85c…7bf08`

is the current computational pipeline identity of the orientation-quarantined
ingress.

They are intentionally not equal.

## Permanent identity law

> **Compatibility representation may change pipeline identity. It may not
> create a new physical observation.**

The physical identity is anchored to the immutable parent source, not to a
derived container.

## Required chain

Admission requires the following exact cross-bindings:

1. real Source Pre-Admission v0.2 parent SHA;
2. DNG Orientation Quarantine v0.1 parent SHA;
3. byte-identical serialized CFA payload across parent and derived ingress;
4. derived ingress SHA as the Host Scientific Route input SHA;
5. source re-verification by the common pipeline;
6. one physical frame / one independent evidence item;
7. source-local D.RAWnegative gauge;
8. no common gauge, equality or fusion;
9. no sensel/ADC authority upgrade;
10. no evidence or scientific writeback created by the lineage binding.

## Observation Record consequence

D.RAW Observation Record v0.3 predates this distinction and requires its
top-level `observation_id` to equal D.RAWnegative v0.1's internal
`observation_id`.

That is acceptable for the historical Camera-5 route, which has no derived
compatibility ingress, but it is not sufficient as the final physical identity
model for this main-camera route.

A versioned Observation Record successor should therefore preserve:

- physical observation identity;
- pipeline/D.RAWnegative lineage identity;
- explicit Scientific Ingress Lineage Binding identity.

D.RAWnegative v0.1 itself remains sealed and unchanged.

## Real main-camera binding

Physical source SHA-256:

`a85cac58601d6cc8138bf8f9372e5161b85ab1e89afa70372f97c46461dcea79`

Derived ingress SHA-256:

`7c8eb85c568f6bc0ec3ae007de1658ec86d38b1144059d0fe0bc294a3e17bf08`

Scientific Master:

`c26939efe5e58a0d32846e905d156789e35b674bb2d1c03288bbabc53e243202`

D.RAWnegative v0.1:

`87955cae86a3c9318b24990018208a4982379366d4bb4ae78e830c8d1cf0ccf7`

Binding state SHA-256:

`ec33ec51a10e0c592fa63045f26b9a519698880a3c9d6000db409b3dd9d15ef6`
