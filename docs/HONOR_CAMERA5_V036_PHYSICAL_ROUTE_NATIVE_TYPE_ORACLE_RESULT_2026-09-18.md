# HONOR Camera-5 v0.36 physical-route native-type oracle result — 2026-09-18

Status: **DEVICE RESULT COMPLETE — 4/4 UNIQUE NATIVE TYPES RESOLVED**

## Scope

v0.36 is representation-only. It does not create a camera session, attach session parameters,
submit a capture request, access RAW pixels or mutate source evidence.

The oracle deliberately separates:

- metadata/tag lookup authority: physical Camera 5 characteristics;
- disposable request-template authority: logical Camera 0.

This preserves the rejected v0.15 direct-open physical-5 result while still testing native
camera_metadata representation for keys that v0.35 showed only on physical Camera 5
session/request surfaces.

## Device result

The uploaded device JSON reports:

- classification: `PHYSICAL_ROUTE_NATIVE_TYPE_ORACLE_COMPLETE_4_OF_4__REPRESENTATION_ONLY`
- logical camera: 0
- physical camera: 5
- control reference: TruthRaw v0.20 unchanged
- candidate count: 4
- resolved count: 4
- unresolved count: 0
- ambiguous count: 0

All four candidates preserve the physical-only availability topology used for selection:
logical session=false, physical session=true, logical request=false, physical request=true.

Resolved representations:

| Symbol | Key | Tag | Native type |
|---|---|---|---|
| H | `org.codeaurora.qcamera3.sessionParameters.inSensorZoomEnable` | `0x801F0029` | BYTE |
| I | `org.codeaurora.qcamera3.sessionParameters.EnableVSR` | `0x801F000B` | INT32 |
| J | `org.codeaurora.qcamera3.sessionParameters.ExtendedMaxZoom` | `0x801F000A` | INT32 |
| K | `org.codeaurora.qcamera3.sessionParameters.enableQLL` | `0x801F000D` | INT32 |

For each candidate exactly one tested metadata type was accepted and all other tested
types were rejected.

The device evidence also confirms that the oracle itself performed:

- no direct physical Camera-5 open;
- no camera session;
- no session-parameter attachment;
- no capture submit;
- no vendor-modified request submit to HAL;
- no RAW pixel access;
- no source mutation;
- no semantic promotion.

## Advertised RAW geometry continuity

The same v0.36 JSON reports:

- standard RAW output: 4080x3072
- maximum RAW output: 8160x6144
- maximum high-resolution RAW output: 16320x12288

This is capability context only and does not change v0.20 source/payload authority.

## Bounded conclusion

v0.36 resolves representation, not semantics or causal route effects.

The key names do not prove what the controls do. Numeric values are not decoded enums.
No candidate is promoted to a sensor, ADC, binning, remosaic, zoom or optical-resolution
claim.

The strongest next experiment is not another broad sweep. Candidate H is selected first
for an **isolated attachment-feasibility + conditional capture experiment** because it is:

1. physical-only in both session and request availability;
2. uniquely resolved as BYTE, unlike I/J/K which are all INT32;
3. therefore a structurally distinct member of the newly resolved physical-only set.

This selection does **not** use the vendor name as semantic evidence.

## v0.37 decision rule

Attempt only `inSensorZoomEnable = BYTE(1)` after Gate A.

Because the key is not advertised as a logical session key and is not an available physical
override key, v0.37 must fail closed:

- verify builder set/readback;
- verify built-request readback;
- attempt `SessionConfiguration.setSessionParameters`;
- if attachment is rejected, record the blocked result and perform no capture;
- only if attachment succeeds may the unchanged v0.20 logical0 -> physical5 MAX source-first
  capture, Stage 3.6 and Stage 3.7 proceed.

No second vendor key may be written.
