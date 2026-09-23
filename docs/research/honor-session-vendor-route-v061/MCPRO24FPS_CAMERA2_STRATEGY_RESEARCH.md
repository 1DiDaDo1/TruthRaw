# TruthRaw v0.61 — mcpro24fps external Camera2 strategy research

Date: 2026-09-23

Status: EXTERNAL SOFTWARE RESEARCH / TARGET-SELECTION ONLY

## Why mcpro24fps matters

mcpro24fps is a third-party Android camera application that explicitly targets Camera2/hal3 and is known for exposing or experimentally activating camera capabilities beyond conservative app defaults. Its behavior is relevant as an independent implementation pattern, but it does not grant TruthRaw any extra evidence authority.

## Public documentation findings

Official mcpro24fps documentation states:

- the app uses Camera2 API / hal3 rather than the legacy Camera API;
- it scans available camera modules and stores per-camera capability/settings state;
- its available-sensor list is based on logical cameras registered by the manufacturer;
- it can sometimes expose more cameras than stock applications make obvious, but acknowledges manufacturer restrictions;
- it uses device-specific workarounds/hidden-key style techniques for some functions when standard Camera2 controls are insufficient;
- some frame rates and sizes can be force-added experimentally, while non-GPU direct codec paths generally remain constrained to Camera2-advertised sizes;
- its developers explicitly distinguish manufacturer-native privileged access from third-party Camera2 access;
- it offers a dedicated camera2info diagnostic utility via its support channel to inspect camera-module capabilities.

These patterns strongly support TruthRaw's existing strategy of separating:
1. public enumerated capability;
2. experimentally accepted configuration;
3. delivered buffer/result topology;
4. OEM/private route evidence.

## RAW-video relevance

The current mcpro24fps public specification advertises a Laboratory RAW-video mode with selectable output depths including 8/10/12/14/16-bit.

This is interesting for Android 17 / Camera-5 research, but **must not be interpreted as proof that the sensor or Camera2 source natively delivers each advertised bit depth**. The advertised recording representation may involve repacking, conversion, scaling, or an internal application container/codec representation.

For TruthRaw, the correct question is:

- which Camera2 image format or producer surface feeds that mode;
- what width/height/stride/sample population the source carries;
- whether the source is RAW_SENSOR, RAW10, RAW12, RAW14 or another buffer type;
- whether advertised recording depth is source depth or output/container depth.

## Relevance to the HONOR Magic 8 Pro path

mcpro24fps documentation is compatible with the device evidence already seen by TruthRaw:

- third-party apps can see logical camera modules and sometimes secondary lenses;
- vendor/manufacturer restrictions remain decisive;
- successful key/session attachment does not imply that the underlying source topology changes;
- undocumented controls can have prerequisites or side effects;
- forcing a size or mode is not equivalent to proving a new sensor readout.

Therefore mcpro24fps should be used as an **implementation/diagnostic reference**, not as evidence that the 200MP tele RAW path is already accessible.

## Additional v0.61 targets inspired by mcpro24fps

Extend the v0.61 read-only oracle to record:

- every public logical camera ID;
- physical IDs disclosed by each logical camera;
- full availableSessionKeys;
- full availableCaptureRequestKeys;
- stream configuration maps for RAW_SENSOR / RAW10 / RAW12 / RAW14 where the platform exposes them;
- high-resolution stream maps;
- dynamic-range/color-space profiles where applicable;
- logical-vs-physical availability topology;
- native vendor tag IDs/types for selected MotionCam-discovered session keys;
- no vendor writes in this stage.

Also add a separate **representation audit** concept for any future mcpro24fps-produced RAW-video sample:

`MCPro output bit depth != source sensor/Camera2 bit depth unless byte topology proves it`.

## Safety / authority boundary

No package-signature modification, identity spoofing, privilege bypass, or proprietary-app patching is part of this research.

Permanent law:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
