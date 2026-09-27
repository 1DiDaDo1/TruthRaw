# D.RAW next-chat handoff — 2026-09-27

## Start here

Official product name: **D.RAW**

Repository: `1DiDaDo1/TruthRaw`

Primary current repository line: `main`.

Latest substantive scientific/control-plane integration:

`1d6785b501f86f129dc81359ac205d6f734af9fd`

The real main camera is now **ADMITTED_SOURCE_LOCAL** as one physical
Observation. A new chat should not repeat the earlier “main awaits scientific
route” work.

## Mandatory reading order

1. `START_HERE_NEW_CHAT.md`
2. `state/CURRENT_PROJECT_STATE_2026-09-27.json`
3. `docs/DRAW_KNOWLEDGE_GROWTH_INTEGRATION_2026-09-27.md`
4. `docs/DOCUMENT_STATUS_INDEX_2026-09-27.md`
5. `docs/research/draw-observation-record-v0.4/README.md`
6. `docs/research/draw-source-capability-envelope-v0.2/README.md`
7. `docs/research/draw-source-admission-package-v0.2/README.md`
8. `docs/research/draw-scientific-ingress-lineage-v0.1/README.md`
9. `docs/research/free-world-observation-graph-v0.1/README.md`
10. `docs/research/drawnegative-v0.1/SEAL_MANIFEST_v0_1.json`

## Permanent laws

- **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
- **Measured where measured. Reconstructed where necessary. Never invented.**
- **One Free World. Many sealed observations. One evidence law.**
- **Evidence stays what it was. Knowledge can grow through admitted relations.**

## Current main-camera physical Observation

Immutable physical source:

`C2OBS_20260914_174825_895_4096x3072.dng`

Physical source SHA-256:

`a85cac58601d6cc8138bf8f9372e5161b85ab1e89afa70372f97c46461dcea79`

Physical Observation ID:

`DRAW_PHYSICAL_OBS_a85cac58601d6cc8138bf8f9372e5161b85ab1e89afa70372f97c46461dcea79`

Physical Free World graph-node SHA-256:

`029b845daf0bb20ebed4e7ce6d66eb61a172dd89e9977f996d20d604c2703353`

Final source-local admission state:

`c20104981a0373f0d2f7c03216272213ff614f0dc732b13247cf2a0c0c6a9b7a`

Status: **ADMITTED_SOURCE_LOCAL**

Counts remain one physical frame / one independent evidence item.

## Compatibility-ingress lineage

The immutable source contains invalid TIFF Orientation value 9. It remains
unchanged.

DNG Orientation Quarantine v0.1 creates a derived compatibility ingress by
changing only the Orientation storage byte `09 -> 01`. Meaning is strictly
`STORAGE_COORDINATE_NORMAL_ONLY`; presentation/world orientation remains
UNKNOWN.

Derived ingress SHA-256:

`7c8eb85c568f6bc0ec3ae007de1658ec86d38b1144059d0fe0bc294a3e17bf08`

Parent and derived serialized CFA payload are byte-identical:

`4818fd406cc528984ff57e036db49ac394ce0bb6b6d0830e2f9f1ad95ecc8c3c`

The compatibility container is **not** a second physical Observation.

## Existing common scientific pipeline result

The CI-built host wrapper calls the same common C++ `prepare()` route used by
the Android/JNI integration; it introduces no alternate reconstruction
algorithm.

The derived ingress passed the complete route twice with byte-identical JSON
results.

- Scientific Master:
  `c26939efe5e58a0d32846e905d156789e35b674bb2d1c03288bbabc53e243202`
- Authority field:
  `e18f0038b36ccca8502e5ec29108145408223084088ab883176d8390838d6b2a`
- TruthNegative Continuous v0.5:
  `cd3acbd90ce47efc2015f9676e7ef0ed25a16248acdb26bdb278ed1ca332a60c`
- D.RAWnegative v0.1:
  `87955cae86a3c9318b24990018208a4982379366d4bb4ae78e830c8d1cf0ccf7`

D.RAWnegative v0.1 remains byte/semantics sealed.

Its pipeline-local Observation ID is:

`DRAW_OBS_7c8eb85c568f6bc0ec3ae007de1658ec86d38b1144059d0fe0bc294a3e17bf08`

This ID is **computational lineage only**. It is intentionally different from
the stable physical Observation ID above.

Scientific Ingress Lineage Binding v0.1 state:

`ec33ec51a10e0c592fa63045f26b9a519698880a3c9d6000db409b3dd9d15ef6`

Validation run `36298041195`: **SUCCESS**.

## Observation Record v0.4

v0.4 is the current physical-identity Observation Record successor.

Record state:

`aaf2dcf0b81ebe5debcc2fa6a0b40057e5f1cf20e0d6e2df67a54fd825853fc5`

Merge:

`d18dfa23c023642b1d4055ee35c4c0aaaab7c7d8`

Validation run `36298333714`: **SUCCESS**.

It embeds a fully valid v0.3 pipeline record but adds the stable physical
Observation identity and physical graph node. Future cross-observation
relations must use the physical Observation ID, not a compatibility-container
pipeline ID.

## Source Capability Envelope v0.2

v0.2 is the current physical source knowledge map.

State:

`057c627bf172521d9336386d4f74ae1e26664c70c4a24ed7993b6c16c33a3986`

Merge:

`fe6275c3dd5f1f4b8f429950768378b44ab1a2c5`

Validation run `36298514477`: **SUCCESS**.

It keeps separate:

- physical Camera2 capture route;
- serialized DNG/CFA storage;
- compatibility ingress;
- capture sample domain;
- readout domain;
- sensor pixel mode.

For main the last three remain **UNKNOWN**.

## Final Source Admission v0.2

Merge:

`1d6785b501f86f129dc81359ac205d6f734af9fd`

Validation run `36298639318`: **SUCCESS**.

Main is now `ADMITTED_SOURCE_LOCAL`, but admission does not create stronger
source claims.

Still explicitly unresolved:

- capture sample domain;
- gain/readout-state identity;
- sensor pixel mode;
- stored-sample sensel semantics;
- direct sensor ADC provenance.

Calibration bindings = 0.  
Graph relations = 0.  
Fusion admissions = 0.

No tele calibration transfers to main.

## Validation boundary

The real main source ran through the unchanged common scientific C++ route on
the host. This is genuine execution against those source bytes, but it is not
a new Android/device runtime validation and it does not certify physical
sensor/readout semantics.

Latest fully host + Android validated D.RAWnegative checkpoint remains:

`3e150afeb36cbb68fd318b0143a315e64d5a637f`

Android/NDK/JNI run `36260305613`: **SUCCESS**.

## Current frontier

Do **not** redo main admission.

Next:

1. locate/seal a real ultra-wide physical-camera-4 source;
2. pre-admit it using only proven source facts;
3. run the unchanged common scientific route;
4. create physical identity + capability map;
5. admit it source-local only after all its own gates pass;
6. only then start independent main↔tele↔ultra-wide relation campaigns.

Cross-observation geometry, gauge, color/spectral, optics,
uncertainty/correlation, temporal and provenance remain independently gated.
