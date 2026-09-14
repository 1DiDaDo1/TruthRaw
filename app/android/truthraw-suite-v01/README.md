# TruthRaw Suite v0.1

This Android project produces one debug APK containing both:

- the normal TruthRaw reconstruction/output UI (`com.truthraw.adaptiveui`); and
- the FotoGraaf Camera2 acquisition activity (`com.truthraw.fotograafcapture`).

The canonical standalone apps remain unchanged. This suite is a convenience packaging layer only.

Because Android permissions are application-wide, the combined suite APK necessarily declares `android.permission.CAMERA`. Scientific authority is therefore separated at the contract/activity level rather than by package permission: normal TruthRaw processing still may not invent capture-time evidence, and FotoGraaf capture still grants no calibration authority by itself.

The suite launcher offers two explicit doors: **Open TruthRaw** and **Open FotoGraaf**. The existing C0/C1/C2, CalibrationPack, scene-admission, shadow-model and physical-promotion gates remain authoritative.
