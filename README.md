# D.RAW / TruthRaw

> **CURRENT PROJECT RECOVERY — 2026-10-03 — continuation code `44489`**
>
> Canonical project/product name: **D.RAW**  
> Historical/repository name: `TruthRaw`  
> Repository: `1DiDaDo1/TruthRaw`  
> Active integration branch: `fix/research-fresh-rerun-v01`  
> Runtime/research checkpoint at the start of the 44489 documentation refresh: `dbf826b883d0e7bb40ff0a8e7654a0d87be78546` (`Remove superseded pre-atomic retained gauge core`)

D.RAW is a deterministic, provenance-bound scientific imaging system built around permanent rules:

> **Seal the evidence, not the thinking.**
>
> **MEASURED != RECONSTRUCTED != APPEARANCE.**
>
> **Representation may become richer than the source; the knowledge claim may never become richer than the evidence.**
>
> **One Free World. Many sealed observations. One evidence law.**

## Start here — code 44489

The single canonical recovery document is:

1. [`docs/handoff/DRAW_44489_RECOVERY_2026-10-03.md`](docs/handoff/DRAW_44489_RECOVERY_2026-10-03.md)

Then read:

2. [`state/DRAW_PROJECT_STATE_2026-10-03.json`](state/DRAW_PROJECT_STATE_2026-10-03.json)
3. [`START_HERE_NEW_CHAT.md`](START_HERE_NEW_CHAT.md)
4. [`docs/DOCUMENT_STATUS_INDEX_2026-10-03.md`](docs/DOCUMENT_STATUS_INDEX_2026-10-03.md)
5. [`docs/research/scientific-master-exact-gauge-retained-artifact-v0.3/README.md`](docs/research/scientific-master-exact-gauge-retained-artifact-v0.3/README.md)

A future chat receiving only **44489** should be able to restore the complete current scientific/architectural state from the repository without asking the user to reconstruct old chats manually. It must fetch the live branch head first; documentation-only commits after the recorded runtime checkpoint are allowed and must not be confused with new scientific/runtime code.

## Current scientific architecture in one line

`sealed physical Observation / RAW_SENSOR -> D.RAW Observation Contract -> Source Capability Envelope -> Float64 measurement/calibration/reconstruction -> Scientific Master -> validated Float32 scientific state -> Dynamic Authority + uncertainty -> TruthNegative Continuous per Observation -> Free World Observation Graph -> Deep Scene / Light Transport -> View / Appearance -> finite projection`

This means lens/sensor/CFA/readout/capture-route are properties of an Observation. TruthNegative belongs to that Observation lineage. Multiple sealed observations may meet in one Free World only through admitted evidence relations; a shared TruthRange coordinate does not by itself prove a shared radiometric gauge.

## Current engineering frontier

The newest implemented research candidate is **Scientific Master Exact Gauge Retained Artifact v0.3**.

Host-side status:

- configure/build: green;
- warnings-as-errors: green;
- ASan/UBSan: green;
- exact parity oracle: green;
- forced budget fallback: green;
- atomic/no-replay semantics present;
- superseded pre-atomic core removed;
- candidate **not promoted**;
- Android integration **not yet completed**.

The candidate retains exact eligible Float32 gauge bits in pass 1 so the exact low-16 self-gauge median can be resolved without the canonical second Stage-2 reread. Scientific output must remain bit-exact.

Exact Gauge must be one registered, versioned **PassArtifact**, not a hard-coded special exception. The atomic core defines safe behavior once an artifact touches Scientific Master; it does not define which future cable/artifact families D.RAW may contain.

## Android lineage

Actual runtime application: `suite_android`.

Current retained Android identity before Exact Gauge Android wiring:

- application ID `com.truthraw.adaptiveui`
- versionCode `26100124`
- versionName `0.53-v0.84.2-scientific-master-tile-read-attribution-v01`

Do not label an APK from this lineage as an Exact Gauge v0.3 device build until the native Foundation/observation integration, regression gates and APK build have actually completed.

## Permanent scientific boundaries

- Direct-CFA/source evidence is immutable and sealed.
- Source capability is not proof of measured sample domain.
- Scientific Master remains separate from presentation/export.
- `UNKNOWN`, `CENSORED`, `RECONSTRUCTED`, `MEASURED` and `APPEARANCE` must not collapse into one another.
- Unknown covariance is not zero.
- Cross-observation radiometric fusion requires an admitted common-gauge relation.
- Performance artifacts may remember computation; they cannot create evidence or authority.
- Camera/lens/vendor/RAW identity may route parsing but cannot select scientific truth.
- No AI/ML/neural/generative runtime exists in the scientific path.
- No diagnostic/performance route may create evidence, mutate measured anchors, promote a correction or perform scientific writeback.
- SOURCE/SENSOR SPACE, WORLD/SCENE SPACE and VIEW/OUTPUT SPACE remain distinct.
- Derived/virtual observations do not create additional physical captures.

## Historical provenance

Older handoffs, state files and architecture branches remain preserved for provenance. In particular, the validated lens-independent Free World Observation architecture from 2026-09-26 is carried into 44489 as scientific lineage, not as the active branch. The previous 44488 bootstrap is superseded as the current recovery entry but remains historical provenance.
