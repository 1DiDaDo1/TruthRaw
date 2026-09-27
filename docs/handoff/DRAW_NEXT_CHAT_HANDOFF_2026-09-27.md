# D.RAW next-chat handoff — 2026-09-27

## Start here

Official product name: **D.RAW**

Repository: `1DiDaDo1/TruthRaw`

Primary current repository line: `main`.

Latest substantive scientific/control-plane integration:

`faea14c3fd9109907e4941156de020978b9bfd86`

The real main camera and the user-captured ultra-wide are now both **ADMITTED_SOURCE_LOCAL** as separate physical Observations. Camera-5/tele remains the existing source-local anchor. A new chat should not repeat main or ultra-wide source admission.

## Mandatory reading order

1. `START_HERE_NEW_CHAT.md`
2. `state/CURRENT_PROJECT_STATE_2026-09-27.json`
3. `docs/DRAW_KNOWLEDGE_GROWTH_INTEGRATION_2026-09-27.md`
4. `docs/DOCUMENT_STATUS_INDEX_2026-09-27.md`
5. `docs/research/draw-direct-physical-source-admission-v0.1/README.md`
6. `docs/research/draw-observation-record-v0.4/README.md`
7. `docs/research/draw-source-capability-envelope-v0.2/README.md`
8. `docs/research/draw-source-admission-package-v0.2/README.md`
9. `docs/research/draw-scientific-ingress-lineage-v0.1/README.md`
10. `docs/research/free-world-observation-graph-v0.1/README.md`
11. `docs/research/drawnegative-v0.1/SEAL_MANIFEST_v0_1.json`

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

## Current ultra-wide physical Observation

Private physical source:

`IMG_20260927_084938.dng`

The user explicitly attests that this is a self-captured **ultra-wide RAW**. That attestation establishes self-capture and lens role only; it does not certify runtime Camera2 active physical result, readout semantics, sensor-pixel mode or calibration.

Physical source SHA-256:

`14757aaac784b17421598121a232c22e044f217531571697a73e4c90bff28133`

Serialized CFA payload SHA-256:

`87e56296cd51a076d921c5a529dfd64784dfc8b9789869b64644b672767a89a5`

Physical Observation ID:

`DRAW_PHYSICAL_OBS_14757aaac784b17421598121a232c22e044f217531571697a73e4c90bff28133`

The original DNG enters the common scientific pipeline directly; no orientation quarantine or compatibility container is needed.

Scientific identities:

- Scientific Master: `949777edb5541064e190d148775ce27d303ce1e0d35f57d7a837fc092ad9f6d9`;
- D.RAWnegative v0.1: `b0ee1322ff6260eb30072533c2b3e294be46375778ac4622b908d6c746e172e0`;
- physical graph node: `85d8ef0b9b3e769f30c4d62a114e70c94369fc3250ae5c4f664c04223980a64e`;
- final admission: `629edde9697387e87c39c66846f21457954d258d831464140eda42ed912b655a`.

Direct-source validation run `36302484185`: **SUCCESS**.

Status: **ADMITTED_SOURCE_LOCAL**.

Project mapping associates ultra-wide with physical camera 4, but runtime active physical Camera2 result 4 is not claimed for this exact DNG. Capture-sample domain, readout domain and sensor-pixel mode remain UNKNOWN. Calibration bindings = 0, graph relations = 0, fusion admissions = 0.

## Main ↔ Ultra-Wide Geometry Relation Campaign v0.1

Status: **main-integrated; PENDING_MATCHED_SCENE_CAPTURE**.

- merge: `753a8048e5fbb97cd6bbaca95403664aeb756d31`
- campaign validation: `36304289379` — **SUCCESS**
- documentation governance: `36304289344` — **SUCCESS**
- campaign state SHA-256: `77d490b0d190b39736aa124add6f1fae9548c132c26d11d1da6b3e8c5aaccd5b`

Current relation is GEOMETRY / UNKNOWN. There is no certificate, no coordinate
transform, no equality, no calibration transfer and no fusion.

The existing MAIN observation (2026-09-14) and ULTRA_WIDE observation
(2026-09-27) are different scenes and are explicitly forbidden as a geometry
fit pair.

The frozen promotion route is a rigid-camera metric-target campaign: at least
12 paired training poses plus 4 disjoint holdout poses. The phone remains
fixed; the indexed metric target moves between poses and remains static between
the MAIN and ULTRA_WIDE exposure of each pair. Every capture must be admitted
source-local before fitting. Model family and thresholds freeze before final
holdout scoring. Exact endpoint applicability is required before a GEOMETRY
certificate may become ADMITTED.

A geometry success changes no other relation axis and cannot directly grant
fusion.

## Open-World Evolution Law v0.1

Current permanent law:

**Seal the evidence, not the thinking.**

Immutable evidence may include original source bytes, SHA-256 identities,
evidence counts and historical provenance/certificates. Scientific
interpretation is not frozen by that seal.

Projection models, distortion models, relation hypotheses, calibration
strategies, thresholds, feature detectors, target families, relation axes and
future source types may receive versioned successors without rewriting the
historical evidence they were built from.

The current schema is therefore a minimum semantic contract, not a claim that
the scientific universe is closed.

## Geometry Direct RAW Pair Intake v0.1

Status: **main-integrated; Camera2 optional**.

- merge: `ddcce0f8490bcbba2030548230f18bfb62c485d2`
- validation run: `36312425248` — **SUCCESS**
- documentation governance: `36312425129` — **SUCCESS**

A MAIN + ULTRA_WIDE geometry pair can now begin directly from two original
RAW/DNG files.

Phase A:

`SEALED_PAIR_AWAITING_SOURCE_LOCAL_ADMISSION`

Phase B:

`ADMISSION_BOUND_PAIR_READY_FOR_FEATURE_EXTRACTION`

The bind step deliberately accepts future
`D.RAW/SourceAdmissionPackage/*` successors when the required semantic
invariants still hold. It is not hard-coded to only the current source
admission schema versions.

Even after binding:

- geometry relation remains UNKNOWN;
- geometry fit is not yet authorized by the intake layer;
- coordinate transform remains false;
- fusion remains false;
- calibration transfer remains false.

Camera2 remains optional acquisition/runtime provenance. It is not required to
create D.RAWnegative or to enter a direct RAW geometry-pair campaign.

## Geometry Capture-Set Gate v0.1

Status: **main-integrated, host validated**.

- merge: `ad24587eca1f018cffaac86ac4e4273cac10e9ef`
- dedicated validation run: `36312992699` — **SUCCESS**
- initial workflow-registration run `36312895715` failed only because of YAML heredoc indentation before any job started; scientific semantics were unchanged.

The validated matrix builds 12 TRAINING + 4 HOLDOUT admission-bound pairs and proves:

- deterministic capture-set identity independent of CLI input order;
- insufficient holdout fails closed;
- HOLDOUT cannot participate in model selection;
- final holdout scoring remains closed until a later model/threshold freeze artifact exists;
- geometry relation remains UNKNOWN;
- coordinate transform, fusion and calibration transfer remain false;
- the capture-set gate does **not** select or seal a projection/distortion model.

Open-world law at this layer:

**Seal the evidence split, not the model hypothesis.**

## Geometry Model Freeze Gate v0.1

Status: **main-integrated, host validated; no real geometry model promoted**.

- merge: `0bca835d58deda5c6a3c6f378d55c352f9f0e8ba`
- dedicated validation run: `36313383767` — **SUCCESS**
- documentation governance: `36313383748` — **SUCCESS**

Core rule:

**Freeze one test candidate, not scientific thought.**

The gate consumes a complete capture set plus a TRAINING-only model-selection
record. It requires predeclared HOLDOUT scoring and acceptance policies before
final HOLDOUT scoring may open.

The selected model family is an extensible string rather than a closed enum.
The integration test explicitly accepts a future unknown model-family name
when the exact model specification, parameters, training result and validity
domain are content-addressed.

After freeze, for that candidate only:

- final HOLDOUT scoring = allowed;
- model refit = forbidden;
- model-family change = forbidden;
- hyperparameter change = forbidden;
- selected-candidate change = forbidden;
- acceptance-threshold change = forbidden.

Changing the idea creates a new versioned candidate; it does not rewrite the
old candidate.

Even after freeze:

- GEOMETRY relation = UNKNOWN;
- coordinate-transform authority = false;
- fusion = false;
- calibration transfer = false.

No real MAIN ↔ ULTRA_WIDE model is currently frozen because the real 12+4
matched capture set does not yet exist.

## Validation boundary

The real main source ran through the unchanged common scientific C++ route on
the host. This is genuine execution against those source bytes, but it is not
a new Android/device runtime validation and it does not certify physical
sensor/readout semantics.

Latest fully host + Android validated D.RAWnegative checkpoint remains:

`3e150afeb36cbb68fd318b0143a315e64d5a637f`

Android/NDK/JNI run `36260305613`: **SUCCESS**.

## Current frontier

Do **not** redo main or ultra-wide source admission.

Current source-local physical anchors are:

- Camera-5 / telephoto;
- physical Camera-2 / main;
- user-captured ultra-wide / project Camera-4 lens route.

The next scientific frontier is independent relation admission between these physical Observations:

1. geometry / pose;
2. radiometric gauge;
3. colorimetric and spectral;
4. optical support;
5. uncertainty / correlation;
6. temporal compatibility;
7. provenance.

Each axis must be measured and certified independently. No relation on one axis grants authority on another. Composite fusion remains forbidden until every relation required for that fusion kind is separately ADMITTED.

Optional device work: the preserved FotoGraaf ultra-wide probe may be used later to upgrade project-map Camera 4 into a runtime active-physical-result proof for a future capture. That would strengthen route provenance only; it would not grant calibration or fusion authority.
