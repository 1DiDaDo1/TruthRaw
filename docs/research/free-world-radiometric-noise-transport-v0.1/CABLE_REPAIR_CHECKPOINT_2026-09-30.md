# D.RAW Cable Repair Checkpoint — 2026-09-30

## Build identity

- Branch: `research/free-world-radiometric-noise-transport-v01-2026-09-30`
- Green code head: `f36f750afda57f4672c264e59ac53db489cd5e19`
- Workflow run: `36744717288`
- Artifact: `11111543510` · `DRAW-free-world-research-debug-arm64`
- APK bytes: `8,155,983`
- APK SHA-256: `d78811f1535075bda0ca1beec553d5cf99eb3533b4bf12ad088ec7dad9f36036`
- Android build: SUCCESS
- New multidisciplinary physical/device validation: NOT RUN
- Scientific promotion: NONE

## Closed cables

### Provenance

Camera capture now carries both identities without conflating them:

`sealed RAW_SENSOR physical root -> acquisition evidence -> derived processing DNG -> Universal Source Profile -> Free World lineage`

The processing DNG remains the processing source. The upstream RAW_SENSOR remains
the physical evidence root. Derived containers do not increment physical frame
count or replace physical evidence.

### Calibration Observation Records

The optional relation-record layer now has:

- canonical JSON identity independent of object key order;
- normalized SHA-256 roots;
- semantic validation of validation status, roles and uncertainty object;
- canonical deduplication;
- active-session source-root binding before numeric candidate use;
- physical-root / processing-DNG aliases mapped to the same admitted observation;
- rejection of cross-session candidate records;
- private session cache persistence rather than large Bundle payloads;
- revalidation when restored/exported.

### Promotion and writeback firewall

A typed internal `ScientificPromotionStateV01` is now the only path by which
future held-out validation may project promotion authority.

Promotion consumers reject arbitrary/imported promotion booleans. The research
Foundation itself remains non-promoting.

The recursive research firewall now blocks accidental activation of, among
other fields:

- candidate application;
- Scientific Master modification;
- measured-anchor modification;
- image transform application;
- correction/deconvolution/denoise authority;
- calibration/geometry/world-space promotion;
- new evidence creation;
- Scientific Master writeback.

### Colour uncertainty

`ColourRelationCandidateSolverV01` is now connected to
`ScientificNoiseMathV01` through
`ColourCovarianceTransportCandidateV01`.

Only explicitly supplied valid camera-RGB covariance is transported:

`C_out = J * C_in * J^T`

Missing covariance remains UNKNOWN and is never synthesized.

### Noise + optics

A real fail-closed bridge now exists:

`controlled NPS + measured MTF/SFR + explicit signal PSD -> NoiseAwareInverseOpticsCandidateV01`

No image transform is applied. Missing signal PSD, NPS or transfer support keeps
the route UNKNOWN.

### Geometry and world-space

Geometry observations are bound to admitted relation roots. World-space
residual candidates no longer obtain authority from imported booleans.

Future world-space authority requires:

- typed internal promotion state;
- held-out validation;
- active-root binding;
- validated world-to-source mapping with uncertainty;
- promoted radiometric state.

### UI/session wiring

- normal File intent resets Research mode unless Research was explicitly requested;
- RAW picker can open even when a session already exists;
- Research mode survives activity recreation;
- calibration records survive configuration/activity recreation in private session cache;
- Research/JSON remains optional for normal photography.

### Test status

Modern tests and historical diagnostic/oracle screens use the same principle:

- green dot + live `Looptijd` while running;
- green dot + `Gereed in` on success;
- red dot + `Gestopt na` on error;
- muted idle/cancelled state where appropriate.

The status display has no scientific authority.

## Intentionally not “fixed” into unsafe behavior

The following remain deliberately fail-closed:

- proprietary RAW decoder-pending formats do not magically become DNG-native;
- generic compute routing remains CPU_REFERENCE unless a specific kernel has its
  own correctness/self-test route;
- candidate availability does not mean scientific promotion;
- denoise/deconvolution/world-space correction remains blocked without the
  preserved physical validation campaign;
- UNKNOWN residual remains valid;
- measured CFA anchors remain immutable;
- the special 4K -> 200MP route remains separate and reconstructed where not
  directly measured.

## Next phase

Implementation wiring is complete at this checkpoint.

The next scientifically meaningful work is the already-defined bundled physical
validation campaign and axis-specific promotion. Compile success is not physical
truth.
