# D.RAW 44489 — Source Capability Physical Occurrence Ledger v0.1 — 2026-10-11

Status: **IMPLEMENTED / CI-PROVEN PASSIVE SOURCE INTROSPECTION — NOT SCIENTIFICALLY PROMOTED**  
Scope: classic TIFF/DNG source-container evidence only.  
PR: #131 (`feat/draw-workspace-free-raster-v01`) remains **open, draft, unmerged**.

This document is a new exact checkpoint. It does not rewrite or invalidate older 44489 provenance. The permanent D.RAW evidence law and the canonical 44489 knowledge capsule remain authoritative.

---

## 1. Exact checkpoints

Clean pre-implementation reference used for this work:

`5ae93afb6eb20f834bfc800df7b9263f6d2e317d`

The previously established 38/38 exact-head workflow state belongs to that checkpoint and was **not rerun as a 38-workflow campaign here**.

Exact generated runtime implementation commit:

`807cd93b14cf3ddadf438aadbfa746c4eb5abf8d`

Exact expanded regression-test checkpoint:

`9d40553ca3bb9b5405b281bff4373ad55e7af46c`

Successful dedicated workflow:

`D.RAW Source Capability Occurrence Ledger v0.1`

Successful runs:

- run `38098094679` — first complete green validation; only after the full gate succeeded did CI commit the runtime implementation as `807cd93b...`;
- run `38098350169` — committed-state replay, idempotency and expanded endian/cross-IFD regression coverage; fully SUCCESS.

Historical failed setup rounds are retained as provenance rather than hidden:

- run `38097806329` — failed only because one CI grep assumed a single-line Kotlin call; no runtime source was committed;
- run `38097883552` — production Kotlin compiled, but JVM tests hit Android's throwing local `org.json` SDK stubs; no runtime source was committed.

The latter was corrected with a strictly test-only dependency:

`testImplementation("org.json:json:20260814")`

This does not enter the production APK/runtime; Android continues to provide its framework `org.json` implementation on-device.

---

## 2. Implemented evidence cable

The implemented cable is deliberately narrow:

`existing sealed source SHA-256 + byte length`

`-> existing DngContainerMetadataParser`

`-> ordered physical TIFF IFD entry occurrences`

`-> SourceCapabilityEnvelopeV02 passive ledger`

`-> UniversalSourceProfiler shadow output`

There is:

- no second TIFF/DNG parser;
- no second source hash route;
- no pixel decode introduced by this ledger;
- no reconstruction change;
- no Scientific Master mutation or writeback;
- no Appearance change;
- no admission/promotion caused merely by metadata presence.

---

## 3. Exact physical occurrence identity

For classic TIFF/DNG magic 42, each 12-byte IFD entry is now retained as its own evidence occurrence.

Each occurrence exports, where structurally available:

- `occurrenceIndex`;
- exact `entryOffset`;
- `entryByteLength = 12`;
- exact `valueFieldOffset = entryOffset + 8`;
- `valueFieldByteLength = 4`;
- exact original four source bytes as `originalValueOrOffsetBytesHex`;
- TIFF tag/type/count;
- overflow-safe `valueByteCount`;
- `storageKind` = `INLINE`, `EXTERNAL` or `UNKNOWN`;
- exact external value offset and end-exclusive byte range when provably in-bounds;
- `locatorStatus`;
- `parseStatus`;
- `decodeStatus`.

The Source Capability ledger binds those fields to the already existing:

- source SHA-256;
- source byte length;
- IFD path;
- IFD offset;
- parser identity.

Its declared physical identity is:

`IFD_PATH_PLUS_EXACT_ENTRY_OFFSET`

and:

`duplicate_tags_preserved_as_distinct_occurrences = true`.

---

## 4. Duplicate-tag law

A tag number is not an occurrence identity.

Two `BlackLevel` entries in one IFD remain two physically distinct ledger records. The same `BlackLevel` tag in two different IFDs also remains two physically distinct ledger records.

This matters because source evidence must preserve what was actually encoded, including contradiction or redundancy. A map-like `tag -> value` representation is insufficient as original evidence.

The pre-existing convenience fields remain compatibility projections. For example, per-IFD `blackLevel` or `UniversalSourceProfiler.findTagValue(...)` may expose a simplified selected value. They are **not** the physical occurrence ledger and may not replace it as provenance.

---

## 5. Independent structural/decode status

The implementation deliberately separates physical location from semantic decoding.

Examples covered by the current implementation/tests:

- ordinary supported entry: `LOCATED` + `DECODED`;
- unsupported TIFF field type: physical occurrence remains present as `LOCATED_UNSUPPORTED_TYPE` + `UNSUPPORTED_TYPE`;
- unsafe/invalid external range: physical occurrence remains present as `LOCATED_EXTERNAL_RANGE_OUT_OF_BOUNDS` + `EXTERNAL_RANGE_OUT_OF_BOUNDS`;
- metadata value too large for bounded decode: location remains evidence while decode is `NOT_DECODED_TOO_LARGE`;
- byte-count overflow path is fail-closed in the generic size logic.

A decode failure therefore does not automatically erase evidence that the physical entry existed.

---

## 6. Container-layout boundary

This implementation intentionally supports the existing classic TIFF/DNG parser contract only.

Classic TIFF:

- magic `42`;
- 12-byte IFD entries;
- 32-bit count;
- four-byte inline-value/value-offset field.

BigTIFF magic `43` is explicitly rejected. D.RAW does **not** reinterpret BigTIFF using classic TIFF offsets or silently pretend support exists.

Future BigTIFF support, if ever added, requires an explicit separate container-layout contract and tests; it must never be obtained by changing the meaning of these classic-TIFF offsets.

---

## 7. Proven regression coverage

The dedicated gate now proves at least:

- little-endian classic TIFF physical offsets and raw value-field bytes;
- big-endian classic TIFF preserves source byte order exactly;
- duplicate same tag in one IFD remains multiple occurrences;
- same tag across two IFDs remains multiple occurrences with distinct IFD paths/entry offsets;
- inline and external storage distinction;
- exact in-bounds external range;
- external out-of-bounds fail-closed retention;
- unsupported TIFF field type remains locatable evidence;
- BigTIFF magic 43 fails closed rather than using the classic layout;
- source SHA-256 and byte length propagate into the passive ledger;
- no capability is admitted or used merely because it was parsed;
- no evidence creation or scientific writeback is enabled;
- sealed Full-Frame Streaming integrity remains PASS;
- the complete Android unit-test suite passes.

The committed-source replay also proves the patcher is idempotent against the implemented state: rerunning the gate does not fabricate a second runtime patch.

---

## 8. Authority/firewall state

Unchanged permanent law:

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`

For this occurrence ledger:

- syntactic presence does not prove semantic correctness;
- successful decode does not prove scientific validity;
- standardized tag naming does not prove that a particular file's value is physically correct;
- parser/source provenance does not create calibration authority;
- `creates_new_evidence = false`;
- `scientific_writeback_allowed = false`;
- `pixel_route_effect = false`;
- `scientific_master_pixel_effect = false`;
- `appearance_pixel_effect = false`;
- admission remains closed unless a later, separate validation contract explicitly proves more.

The ledger is therefore richer knowledge *about the bytes and their structure*, not richer scientific authority over the photographed world.

---

## 9. Important limits — do not overclaim

This checkpoint does **not** prove:

- real-device behavior of the new ledger;
- a broad real-DNG corpus;
- semantic correctness of BlackLevel, WhiteLevel, ColorMatrix, NoiseProfile or any other parsed tag;
- scientific calibration promotion;
- BigTIFF support;
- recovery of malformed/truncated containers as complete ledgers;
- explicit anomaly records for every invalid IFD traversal condition.

In particular, the existing recursive IFD walker currently stops traversal on invalid/non-positive/out-of-file offsets, recursion-limit excess or a revisited offset. Hard structural truncation can also terminate parsing through existing `require(...)` checks. Those behaviors remain fail-closed, but they do not yet produce a complete passive **container traversal anomaly ledger**.

Do not describe that future anomaly layer as already implemented.

---

## 10. Next safe implementation seam

The original Source Capability Evidence Ledger v0.2 design requires richer source introspection, contradiction/fail-closed testing, missing-field handling and deterministic evidence records before any new datum may control scientific reconstruction.

Now that exact physical entries are preserved, the next narrow zero-pixel-change seam is:

`physical entry ledger`

`-> passive container traversal/completeness status`

`-> source-bound anomaly records for cycle / invalid offset / recursion limit / truncated IFD structure`

`-> existing Source Capability Envelope`

This next layer should report what could and could not be traversed. It must **not** repair the container, invent missing entries, promote metadata, modify source bytes or affect pixels/Scientific Master/Appearance/admission.

After that, semantic validation of specific standardized tags can be considered one contract at a time, with public-specification semantics plus observation-specific consistency tests. Presence alone is never sufficient.

---

## 11. Relationship to older active project lines

This checkpoint does not reopen or modify:

- accepted Censored Chroma Fallback / purple-highlight behavior;
- Natural Light Local Field Tone candidate status;
- emissive-display/yellow-subtitle research;
- Room Capsule/T5;
- Exact Gauge Retained Artifact v0.3;
- reconstruction promotion state;
- Free Raster/JPEG output contracts.

Those remain governed by their own exact historical/runtime evidence and the canonical 44489 capsule.

**One Free World. Many sealed observations. One evidence law.**
