# Professional RAW Ingress / Decoder ABI v0.1 — Engineering Report

## Decision

`LOCAL_POLICY_PROTOTYPE_PASS_FORMAT_DECODE_TOPOLOGY_AUTHORITY_SEPARATED`

## Main result

TruthRaw can be widened toward professional camera RAW without changing the sealed-house principle. The correct integration boundary is before Measurement Lab reconstruction: original container bytes stay in Archivist custody, a decoder adapter exposes verified sample semantics plus provenance/resource metadata, and scientific admission independently decides whether those samples qualify for the current single-frame Direct-CFA master.

## Authority model

The implementation deliberately avoids one overloaded `SUPPORTED` flag. It separates:

- container family;
- measurement topology;
- compression semantics;
- decoder certification;
- downstream topology certification;
- scientific admission;
- evidence class;
- execution memory mode.

This prevents a file that is merely readable from gaining a stronger scientific status than its evidence supports.

## Resource integration

A decoder reports resident/scratch upper bounds and whether it is tile-random-access, storage-unit-bounded or full-frame-materialized. These values belong in Building Runtime / Room ABI admission.

A weak device may be unable to admit a full-frame decoder lease while a stronger device can. That is an execution-availability difference, not an evidence upgrade. Long-term parity requires tile/streaming professional RAW adapters where practical.

## Local validation

GCC Release: PASS

Clang Release: PASS

Clang ASan/UBSan: PASS

Observed deterministic output:

```
PROFESSIONAL_RAW_INGRESS_V0_1_TEST_PASS
extensionAloneCertifies=0
nativeDngDirectCfa=1
certifiedExternalLossless=1
xtransWithoutDownstreamTopology=research_only
computationalRawDirectCfa=0
multiShotDirectCfa=0
```

## Next steps

1. bind this contract into Room ABI v0.2 via a borrowed immutable decoder-provenance handle and decoder resource lease;
2. define adapter certification fixtures for real vendor RAW samples before claiming any family/model as supported;
3. keep Tile-Native DNG Source v0.1 as the native low-memory route;
4. evaluate broad compatibility backends only behind the adapter ABI, with explicit full-frame memory accounting;
5. add topology-specific scientific paths (for example X-Trans or layered sensors) only after measured-preserving reconstruction/uncertainty contracts are independently proven;
6. maintain a versioned compatibility registry at camera + RAW-mode/codec granularity rather than a permanent “all RAW supported” claim.
