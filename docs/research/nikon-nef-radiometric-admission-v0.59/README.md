# Nikon NEF radiometric admission v0.59

Status: **RESEARCH / EXACT-SCOPE BLACK + SATURATION GATE / SCIENTIFIC MASTER STILL BLOCKED**

## Goal

v0.58 established that a strict subset of Nikon NEF can expose exact CFA sample codes through the common multi-vendor `IRawTileSource` ABI.

v0.59 adds the next authority layer:

`exact NEF sample decode`
→ `exact-scope radiometric binding`
→ black-level + saturation/white admission
→ **still stop before Scientific Master** until noise/uncertainty and source-bound color are separately admitted.

## Why this is separate

A 16-bit NEF storage word does not imply that `65535` is the physical sensor saturation value.

Likewise, a decoder that can expose exact stored sample codes does not automatically know:

- optical black / electronic offset;
- per-CFA-phase black;
- mode-specific saturation;
- gain/readout-state changes;
- noise model;
- physical color transform.

Therefore the v0.58 placeholder metadata:

- black = unknown;
- white = storage ceiling only;

must never be promoted into scientific radiometry merely because the container decoded.

## RawRadiometricBinding

The multi-vendor adapter ABI now has a versioned `RawRadiometricBinding`.

A vendor-specific radiometric pack must bind exactly to:

- format family;
- camera Make;
- camera Model;
- RAW width and height;
- CFA code;
- storage bits per sample;
- a non-empty binding ID;
- an explicit authority class.

Supported authority classes at this stage:

- `ContainerExplicit`;
- `CalibrationPackValidated`.

`Unknown` never admits radiometry.

The binding also carries:

- four CFA-phase black values;
- admitted white/saturation code.

## Fail-closed scope matching

When a radiometric binding is supplied to the Nikon adapter, **every scope field must match the decoded source**.

A Nikon Z8 pack cannot silently apply to a Nikon Z9 file.
A pack for a different geometry, CFA phase or storage bit depth cannot silently apply.
Invalid or non-finite black/white values fail closed.

When no binding is supplied, the NEF remains measurement-only.

## Descriptor result

A successfully matched radiometric pack sets:

- `radiometricBindingProvided = true`;
- `blackLevelAuthoritative = true`;
- `saturationLevelAuthoritative = true`.

It does **not** set:

- `scientificAdmissionReady`;
- `directSensorAdcClaimAllowed`;
- `scientificColorBindingProvided`.

Black/saturation authority is one gate, not the whole camera model.

## Current scientific-admission chain

`sealed NEF`
→ strict sample decode
→ exact camera/source scope identity
→ black/saturation admission
→ **noise/uncertainty admission pending**
→ **source-bound color admission pending**
→ held-out validation pending
→ Scientific Master eligibility.

## Tests

v0.59 host tests require:

1. NEF Make + Model identity extraction;
2. exact sample decode without a radiometric pack remains scientific-blocked;
3. exact-scope validated black/white pack is applied;
4. a mismatched camera-model pack fails closed;
5. radiometric success alone still does not enable Scientific Master;
6. compressed NEF remains unsupported/fail-closed.

## Nonclaims

v0.59 does not claim that Nikon MakerNote black/saturation semantics have been decoded.
It does not ship a real Nikon camera calibration pack yet.
The synthetic test pack validates scope enforcement only.

A real Nikon promotion requires a real NEF corpus and independently validated camera/mode calibration evidence.
