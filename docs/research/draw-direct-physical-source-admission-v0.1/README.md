# D.RAW Direct Physical Source Admission v0.1

Status: **REAL ULTRA-WIDE INTEGRATION CANDIDATE**

This family adds direct-ingress successors for sources that already enter the
common scientific pipeline without a compatibility container.

Versions:

- SourcePreAdmissionManifest v0.3
- ScientificIngressLineageBinding v0.2
- DRAWObservationRecord v0.5
- SourceCapabilityEnvelope v0.3
- SourceAdmissionPackage v0.3

The new mode is `DIRECT_PHYSICAL_SOURCE_INGRESS`.

The physical source and pipeline input may be byte-identical while their
identity roles remain separate:

- `DRAW_PHYSICAL_OBS_<source>` = physical Free World observation;
- `DRAW_OBS_<source>` = D.RAWnegative v0.1 computational lineage.

The user attests the supplied source as a self-captured ultra-wide RAW.
Project camera 4 mapping is retained as project-map authority only; runtime
Camera2 active physical result is not claimed for this file.

No calibration, relation or fusion is introduced. Unknown capture sample,
readout and sensor-pixel-mode domains remain UNKNOWN.
