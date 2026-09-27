# D.RAW Host Scientific Route v0.1

Status: **HOST WRAPPER CANDIDATE — NO NEW SCIENTIFIC ALGORITHM**

This module exists only to execute the already-current D.RAW scientific route
on a POSIX host file descriptor.

It calls:

`truthraw::android_truthnegative_pipeline::v0_1::prepare()`

directly.

Despite the historical namespace name, that common pipeline function is ordinary
C++ and uses a POSIX file descriptor plus the same validated project modules
used by the Android/JNI route.

The host wrapper does not reimplement:

- DNG parsing;
- source sealing;
- DNG color binding;
- Stage-2;
- F64 reconstruction;
- Scientific Master hashing;
- Zero-Line self-gauge;
- authority-field construction;
- TruthNegative Continuous v0.5;
- D.RAWnegative v0.1.

It merely opens the input file, invokes the existing common pipeline and emits
its resulting identities/audit as JSON.

## Privacy boundary

GitHub CI builds the executable from public project source. A user's real DNG
does not need to be uploaded to the repository or to Actions.

The intended workflow is:

1. build and validate the wrapper in CI;
2. download the executable artifact;
3. execute it locally against the private sealed DNG;
4. commit only the resulting scientific identities/evidence report if admitted.

## Authority boundary

A successful host run proves deterministic execution of the current common
pipeline against those exact source bytes. It does not prove Android runtime
performance, physical sensor ADC provenance, capture-sample/readout identity,
or cross-observation calibration.

The adapter's own conservative fields remain visible in the JSON, including
`storedSampleSenselSemanticsCertified` and
`directSensorAdcClaimAllowed`.
