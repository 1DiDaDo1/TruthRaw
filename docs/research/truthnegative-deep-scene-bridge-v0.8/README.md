# TruthNegative Deep Scene Bridge v0.8

Status: **EXECUTABLE AUTHORITY-PRESERVING BRIDGE**

v0.8 connects the raster-independent TruthNegative Continuous scientific negative directly to the existing Free-World Deep Scene and Light Transport architecture.

## Camera-plane object binding

A TruthNegative target query can now become one evidence-constrained deep-scene contribution while preserving:

- TruthNegative state SHA-256;
- exact query SHA-256;
- per-channel radiometric authority;
- one physical frame;
- one independent evidence item;
- zero measured target claims.

Region/object identity and geometry authority are then attached separately.

This is the key separation:

```text
TruthNegative radiometry
    !=
3D geometry claim
```

A camera-plane colour/radiometry result can remain bound to TruthNegative while its assigned depth is only INFERRED.

## Deep packet ancestry

The v0.8 scene packet binds:

- TruthNegative state identity;
- TruthNegative query identity;
- v0.4 DeepPixelPacket identity;
- v0.5 BoundDeepPacket identity;
- object/region/provenance identity;
- geometry authority.

Changing object identity changes the scene-packet hash without changing the underlying scientific-negative state.

## Physically based continuation

v0.8 also provides a bounded helper that creates an **INFERRED Lambertian light-transport seed** from a bound scene object.

The seed can carry:

- incoming/outgoing directions;
- inferred surface normal;
- inferred material identity and diffuse RGB;
- inferred illumination identity;
- inferred spectral-hypothesis identities;
- visibility.

It explicitly does not inherit TruthNegative radiometric authority as a measurement of hidden material, illumination or geometry.

The result keeps:

- `inheritedScientificRadiometryAsMeasurement = false`;
- `createsNewEvidence = false`;
- `scientificWritebackAllowed = false`.

## Why this matters

This is the first direct bridge from the scientific-negative interface into the physically based Free-World scene.

TruthNegative remains the boundary object for what the image scientifically supports. Deep geometry and light transport can become richer above it without rewriting that evidence.

## Next gates

- device-side PRO diagnostics can expose object/deep-scene ancestry;
- Camera-5 colour/highlight oracle can bind clipping and colour metadata to the same TruthNegative state;
- inverse optics remains blocked until admitted PSF/MTF calibration exists;
- native TruthNegative container/export can serialize the state + authority + provenance without reducing it to a display raster.
