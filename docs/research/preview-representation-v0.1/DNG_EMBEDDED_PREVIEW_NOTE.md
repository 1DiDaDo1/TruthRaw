# DNG embedded preview note — Preview Representation v0.1

Date: 2026-09-11

Adobe's current Camera Raw documentation states that a DNG can embed a JPEG preview specifically so other applications can view the contents without parsing the camera RAW data.

TruthRaw therefore adopts the following research policy for future production DNG export:

- keep the scientific/raw payload independent from the human-facing preview;
- when a DNG compatibility preview is embedded, use the same **sRGB JPEG compatibility representation** chosen by Preview Representation v0.1;
- never use the embedded JPEG as evidence, calibration, Scientific Master, CFA measurement, uncertainty source, or numeric validation reference;
- changing/removing/re-encoding the embedded preview must not alter the scientific/raw payload or its evidence identity;
- an embedded JPEG preview may represent an explicitly named appearance/export view, but it must not silently relabel that appearance as measurement truth;
- the external standalone JPEG preview remains useful even when a DNG embeds one, because not every chat/file/gallery surface exposes an embedded DNG preview consistently.

This supports the architectural rule:

```text
one scientific/raw result
        |
        +--> DNG/raw payload
        |
        +--> embedded JPEG preview (compatibility only)
        |
        +--> standalone JPEG preview (visibility/share only)
```

Reference reviewed 2026-09-11:

- Adobe Camera Raw — Open, process, and save images: the DNG `JPEG Preview` option embeds a JPEG preview so other applications can view the DNG contents without parsing camera RAW data.
  https://helpx.adobe.com/camera-raw/desktop/get-started/overview-and-setup/navigate-open-save-images-camera.html

No claim is made here that every Android or third-party viewer will choose the same embedded preview IFD or render every DNG identically. That remains a physical-device/viewer compatibility test.