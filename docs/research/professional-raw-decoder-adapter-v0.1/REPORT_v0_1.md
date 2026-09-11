# Professional RAW Decoder Adapter v0.1 — Engineering Report

## Decision target

`FORMAT_NEUTRAL_DECODER_ADAPTER_WITH_FAIL_CLOSED_EVIDENCE_AND_RESOURCE_ADMISSION`

## Architecture

`sealed original -> format router -> decoder adapter -> measurement/topology descriptor -> Professional RAW Ingress classification -> scientific admission`

The decoder adapter does not own the scientific master. It normalizes decoder claims and resource requirements.

## Scientific classes

- native strict DNG may reach `DirectNativeCertified`;
- verified external lossless Bayer may reach `LosslessDecodedCertified`;
- unverified decoder/sample equivalence remains blocked or research-only;
- computational and multi-shot output stays derived;
- non-Bayer topology is preserved rather than coerced.

## Resource behavior

`residentUpperBoundBytes + scratchUpperBoundBytes <= available decoder budget`

Resource failure never retunes or weakens scientific classification. A job waits/fails resource admission instead.

## Next implementation

Add a first external compatibility backend behind this ABI, initially as a capability/provenance adapter with explicit full-frame memory accounting. Do not make it canonical until real-file corpus tests prove byte/sample semantics for named codec variants.
