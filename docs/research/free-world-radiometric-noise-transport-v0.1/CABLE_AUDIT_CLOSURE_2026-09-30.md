# D.RAW cable-audit closure — 2026-09-30

## Status

**IMPLEMENTED + ANDROID BUILD VERIFIED**

Build-proven source head:
`55755b795ec6e48e326732c8e5d2071270b81a90`

Workflow:
- name: `D.RAW Free World Research APK`
- run: `36744973469`
- conclusion: `SUCCESS`
- artifact: `DRAW-free-world-research-debug-arm64`
- artifact id: `11112237658`
- APK bytes: `8155983`
- APK SHA-256: `11df4341bc9414a15fdfdd1412b769c700eeb798298a9119fea83c108f3d5193`

This build result proves integration/compilation/package creation only.
It does not prove the new physical models, calibrations, world registration,
noise model or denoise result on a real device.

## Closed wiring findings

### Physical camera lineage

The physical capture chain is explicitly represented as:

`sealed RAW_SENSOR physical root -> acquisition evidence -> derived DNG processing root -> Universal Source Profile -> Scientific Master`

The processing DNG remains a useful sealed processing source but never becomes a
second physical frame merely because it has its own hash.

`FreeWorldEvidenceLineageManifestV01` now carries both processing roots and
physical-evidence roots plus explicit derivation records.

### Calibration Observation Record identity

`CalibrationObservationRecordIdentityV01` provides canonical SHA-256 record
identity:
- JSON object key order does not change identity;
- array order remains significant;
- source SHA-256 roots are normalized to lowercase;
- session-binding fields are excluded from the physical record identity;
- a changed scientific payload receives a changed candidate identity.

UI de-duplication uses this identity rather than `JSONObject.toString()`.

### Active-session binding

`CalibrationObservationSessionBindingV01` binds records to the currently
selected Universal Profiles before any numeric candidate solver sees them.

Fail-closed rules:
- every record root must belong to the active observation set;
- camera RAW_SENSOR roots and their proven processing-DNG aliases map to the same
  observation;
- listing both physical and derived aliases as independent observations is
  rejected;
- cross-session record use is forbidden.

### Record validation

The validator now checks:
- supported axis;
- valid/unique SHA-256 roots;
- one nonblank role per root;
- nonblank setup description;
- allowed relation evidence class;
- nonempty uncertainty object;
- finite uncertainty numbers;
- no negative sigma/variance/bound/RMSE values;
- explicit uncertainty status when no numeric uncertainty is present;
- recognized validation status;
- canonical identity consistency;
- forbidden camera/lens/vendor/format identity keys recursively.

Numeric candidate admission additionally requires:
- active-session binding;
- relation evidence class eligible for numeric research;
- a numeric-admission validation status.

`UNVALIDATED`, `CANDIDATE`, `REJECTED` and `FAILED` records cannot drive a
numeric solver.

### Sparse CFA and radiometric source aliases

Repeated sparse-grid noise and RAW-profile radiometric relations accept the
proven physical RAW_SENSOR or processing-DNG alias but normalize both to the
same session observation.

This closes case-sensitive SHA lookup and camera-derived-DNG ancestry gaps.

### Colour covariance

`ColourCovarianceTransportCandidateV01` connects the multi-illuminant 3x3
colour-relation candidate to `ScientificNoiseMathV01`.

Only explicitly supplied camera-RGB covariance is transported:

`C_out = J * C_in * J^T`

No missing covariance is synthesized.

### NPS / optics / inverse-optics

`NoiseOpticsJointCandidateV01` connects:
- controlled NPS measurements;
- controlled MTF/SFR optical-support measurements;
- explicit signal PSD;

to `NoiseAwareInverseOpticsCandidateV01`.

The bridge does not infer signal PSD from the image, does not infer transfer
from apparent sharpness, does not apply an image transform and does not
authorize deconvolution.

### Promotion state

`ScientificPromotionStateV01` is the only typed runtime path for future
scientific promotion.

It requires an internal promotion-decision schema with:
- internal-validator provenance;
- held-out validation pass;
- no measured-sample mutation;
- no writeback request;
- source roots bound to the active session.

Promotion consumers call `ScientificPromotionStateV01.decision(...)` rather
than trusting loose JSON booleans.

User-imported Calibration Observation Records cannot create this state.

### World-to-source bridge

`FreeWorldSourceLatticeBridgeContractV01` can only admit a future bridge when
all are present:
- promoted world-to-source state;
- exact validated mapping schema;
- held-out validation pass;
- explicit mapping uncertainty;
- non-appearance authority;
- mapping source roots bound to active source roots.

Default/current research state remains fail-closed.

### Geometry / temporal / world residual provenance

Nested geometry rays, temporal observations and world-space residual samples
must belong to source roots admitted by their session-bound record.

World-space residual decomposition additionally requires the actual admitted
world-to-source bridge and promoted radiometric relation; imported payload
booleans are not authority.

### Reconstruction / denoise

`ScientificReconstructionCandidateV01` can optionally restrict support to the
source roots carried by a promoted state.

`ScientificDenoiseOperatorV01.resolveWithPromotionState(...)` connects future
internal promotion state to the existing denoise admission and reconstruction
operator.

Research Foundation calls keep `allowPromotionProjection=false`.

Even after a future admitted route:
- output authority remains `RECONSTRUCTED`;
- measured anchors are not modified;
- Scientific Master is not modified;
- no new evidence is created;
- scientific writeback remains false.

### Promotion firewall

`ResearchPromotionFirewallV01` now also blocks true values for:
- `candidate_applied`;
- `scientific_master_modified`;
- `measured_anchor_modified`;
- `measured_anchors_modified`;
- `image_transform_applied`;

in addition to the existing promotion/correction/deconvolution/denoise/writeback
flags.

### Navigation and session state

Normal navigation now resets Research mode when a non-Research intent reuses
`MainActivity`.

Direct RAW/DNG actions reopen the picker even when a session already contains
RAWs.

Large Calibration Observation Records are not placed in the Activity Bundle.
They use `CalibrationObservationRecordSessionStoreV01`, a private cache scoped
by an opaque session id. The cache is not evidence and is cleared when the
activity is deliberately finished.

### Test status invariant

The modern Research UI keeps the shared green/red status-dot + timer model.

`TruthRawLegacyTestStatusV01` now applies the same convention to the historical
diagnostic/oracle activities exposed from Settings without changing their
measurement logic:
- green + live `Looptijd` while running;
- green + `Gereed in` on success;
- red + `Gestopt na` on error/fail-closed result;
- muted idle state.

The timer/dot remains UI-only and cannot alter authority.

## Verified invariants retained

- sealed source bytes are immutable;
- `MEASURED != RECONSTRUCTED != APPEARANCE`;
- BlackLevel != Zero-Line;
- F64 remains the branch-sensitive reconstruction/decision domain where
  required;
- controlled F32 remains the scientific storage/export domain where admitted;
- frontside appearance analysis cannot redefine backside CFA evidence;
- JPEG previews remain non-authority preview-only;
- Camera-5 / 4K-to-200MP output remains reconstructed dense support and never
  claims 200MP measured CFA detail;
- restoration remains downstream/retreatable;
- camera/lens/vendor/RAW identity is not a scientific model key;
- UNKNOWN residual is not automatically noise.

## Intentional boundaries that remain

These are not loose cables and must not be bypassed merely to make more features
appear active.

### Proprietary RAW decode coverage

DNG is the fully admitted native scientific-processing path.

NEF remains sample-decode/calibration-pending; CR3/CR2/ARW/RAF/RW2/ORF/PEF/
IIQ/X3F and other proprietary formats remain decoder-pending immutable handles
until their decoders and semantics are separately implemented and validated.

Architecture-level identity independence does not imply decoder availability.

### Generic compute router

The generic router remains `CPU_REFERENCE` unless an individual accelerated
kernel has its own equivalence/correctness/self-test.

Specific already-existing accelerated paths such as TruthNegative Vulkan and
restoration multicore remain separate.

Compute speed cannot increase truth authority.

### Scientific Master Streaming v0.3

v0.3 remains a research candidate. Production/native bridges are not silently
migrated away from the currently admitted streaming binding.

### Continuous Free World query

The runtime remains UNKNOWN until a world-to-source mapping and continuous
reconstruction are physically validated and promoted.

### Physical validation

The new multidisciplinary candidate wave still needs real-device and controlled
held-out validation before any axis-specific promotion.

## Build note

The Android build emits an existing C++ compiler warning in
`canonical/reconstruction/v4.7i/native/src/core.cpp` about misleading
indentation around a compact `}return Status::ok();` sequence.

The build succeeds. This audit deliberately did not change that canonical
reconstruction implementation merely to silence a formatting warning.
