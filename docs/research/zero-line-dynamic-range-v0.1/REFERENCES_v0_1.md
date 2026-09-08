# External fact-check references

This research note was checked against the following concepts:

1. OpenEXR scene-linear representation: scene-referred values are proportional to light and are not naturally bounded by display white.
2. darktable scene-referred filmic model: scene EV coordinates are described from negative infinity to positive infinity around a reference middle gray.
3. Debevec & Malik, SIGGRAPH 1997: recovered radiance maps are determined up to a multiplicative scale factor.
4. EMVA 1288 Release 4.0: physical camera dynamic range remains a finite ratio between saturation and sensitivity threshold.

These sources support the distinction made in this proposal:

- an unbounded representation coordinate is mathematically valid;
- the camera's evidence-supported dynamic range remains finite;
- choosing a gauge/zero reference does not magically recover information lost to saturation or noise.
