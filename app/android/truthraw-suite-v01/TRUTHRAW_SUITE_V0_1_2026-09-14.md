# TruthRaw Suite v0.1 — 2026-09-14

This Android project produces one debug APK containing both the normal TruthRaw reconstruction/output UI (`com.truthraw.adaptiveui`) and the FotoGraaf Camera2 research activities (`com.truthraw.fotograafcapture`).

The canonical standalone apps remain unchanged. This suite is a convenience packaging layer only.

Because Android permissions are application-wide, the combined suite APK necessarily declares `android.permission.CAMERA`. Scientific authority is therefore separated at the contract/activity level rather than by package permission: normal TruthRaw processing still may not invent capture-time evidence, and FotoGraaf capture still grants no calibration authority by itself.

Current suite doors include:

- **Open TruthRaw** — the normal four-mode TruthRaw output/reconstruction UI.
- **FotoGraaf capability probe** — read-only inventory for physical main/wide/tele routes, standard RAW, maximum-resolution RAW, close-focus/macro controls and runtime RAW14 advertisement.
- **FotoGraaf generic Camera2** — generic RAW_SENSOR acquisition observation.
- **FotoGraaf HONOR tele physical ID 5** — fail-closed physical-output route that requests ID 5 through the logical multi-camera and requires a matching physical result.

The existing C0/C1/C2, CalibrationPack, scene-admission, shadow-model and physical-promotion gates remain authoritative. A capability is not capture proof. A capture observation is not calibration authority. Physical capture proof requires the requested physical route, a matching physical `TotalCaptureResult`, timestamp identity and sealed finalized source bytes before C0 can advance.
