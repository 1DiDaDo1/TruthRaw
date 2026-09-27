# D.RAW Direct Physical Source Admission v0.1

Status: **MAIN-INTEGRATED HOST-VALIDATED REAL ULTRA-WIDE ADMISSION**

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

## Integrated ultra-wide checkpoint

- merge: `faea14c3fd9109907e4941156de020978b9bfd86`;
- validation run: `36302484185` — **SUCCESS**;
- physical source SHA-256: `14757aaac784b17421598121a232c22e044f217531571697a73e4c90bff28133`;
- Scientific Master: `949777edb5541064e190d148775ce27d303ce1e0d35f57d7a837fc092ad9f6d9`;
- D.RAWnegative v0.1: `b0ee1322ff6260eb30072533c2b3e294be46375778ac4622b908d6c746e172e0`;
- physical graph node: `85d8ef0b9b3e769f30c4d62a114e70c94369fc3250ae5c4f664c04223980a64e`;
- final source-local admission state: `629edde9697387e87c39c66846f21457954d258d831464140eda42ed912b655a`.

The source is admitted through `DIRECT_PHYSICAL_SOURCE_INGRESS`. No compatibility container is required. Runtime Camera2 active physical result 4 remains unproven for this exact file; capture-sample domain, readout domain and sensor pixel mode remain UNKNOWN.
