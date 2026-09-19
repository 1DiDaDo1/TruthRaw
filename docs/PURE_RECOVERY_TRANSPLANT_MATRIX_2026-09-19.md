# TruthRaw PURE recovery transplant matrix — 2026-09-19

Status: **RECOVERY CLASSIFICATION / NO PORT YET / NO PIXEL CHANGE**

This matrix classifies the historically proven 2026-09-14 PURE/RGB/output implementation against the current TruthNegative/Open-World base.

Classification vocabulary:

- **PORT_READY_HOST** — dependencies required by the host module are still present and observed byte-identical where checked; port candidate without changing equations.
- **PORT_POLICY_ONLY** — preserve policy/behavior, but current Android/product wiring must be re-integrated rather than copied blindly.
- **LEGACY_VERIFY_ONLY** — preserve for historical artifact verification, not as the active schema for new artifacts.
- **REFERENCE_COMPAT** — useful downstream compatibility implementation/history, but not TRUTHRAW PURE authority.
- **SUPERSEDED_CURRENT** — a newer current module covers the active role.
- **BLOCKED_NEW_BINDING** — concept is valid but requires a new versioned binding to current authority before new artifacts use it.

## 1. Scientific Master Float32 PURE writer

Historical module:

`docs/research/scientific-master-linear-dng-projection-v0.1/`

Classification: **PORT_READY_HOST**

Why:

- it consumes camera-native scene-linear Scientific Master tiles;
- it prohibits appearance/tone/gamut/display transforms in the source contract;
- it transforms only through the admitted `cameraToXyzD50`;
- it writes 3-channel IEEE float32 LinearRaw;
- it counts but does not clip finite negative and >1 output values;
- it recomputes the Scientific Master digest while exporting;
- transaction commit is blocked unless the recomputed master exactly matches the admitted Scientific Master hash.

Observed dependency identity between the historical PURE branch and the current TruthNegative base:

| Dependency | Historical SHA | Current SHA | Result |
|---|---|---|---|
| Scientific Master Digest header | `6bb779d43270e99bb876727ce1ab7cb7b04ba200` | same | byte-identical |
| Scientific Master Digest cpp | `669122bcae82dd5475b2a1dfa413bc3e8dff6c22` | same | byte-identical |
| Scientific Master Streaming Binding v0.2 header | `d81e49227d4bdef8a889165e9a297cda343559ff` | same | byte-identical |
| Scientific Master Streaming Binding v0.2 cpp | `b005170b420dd6e4142d836110abb9dbd41f634a` | same | byte-identical |
| Scientific Master Streaming Binding v0.1 header | `ece9a9740ace45321161fc10843948cecacd5ccb` | same | byte-identical |
| Scientific Master Streaming Binding v0.1 cpp | `c4cf30a803ba934a51f0c2b550f6d70de73e2deb` | same | byte-identical |
| Full Frame Streaming header | `79bd42c18918ea524220d94646d46bbe4e4040b1` | same | byte-identical |
| Full Frame Streaming cpp | `3335eb85a48c930e5a7dcb08c73975c1ed0ae50e` | same | byte-identical |
| TruthRange latent v0.2 header | `90b15bb5e4f1f9daee6babe39730f88882ecce0e` | same | byte-identical |
| TruthRange latent v0.2 cpp | `2bf0aecd05c0b36440d14300d9ea973ffa58c62a` | same | byte-identical |
| TileNative DNG source public header | `0afb0e6072f3b88783709a01d637073e333c2267` | same | byte-identical |
| DNG color producer v0.2 header | `3144a25e040cb4c849d5a13d92ae6d2a38c08df5` | same | byte-identical |
| DNG color producer v0.2 cpp | `4fbd39edb7d9f688fc5e3f5698d6224ca6472738` | same | byte-identical |
| Technical Backplane phase2 header | `525d324bed20b976ae744f822ae115f0270e12c8` | same | byte-identical |
| Technical Backplane phase2 cpp | `ed1d1cbc99ee2fae98f826748820ff5c41454634` | same | byte-identical |
| frozen v4.7i core.cpp | `f79b951ba54cff08db400023e528e4eb91909ba6` | same | byte-identical |

This strongly reduces the risk that the historical host PURE writer depends on an obsolete Scientific-Master equation. It does **not** by itself authorize a port; the historical module must still be built/tested on the recovery branch and the current frozen real-source Scientific Master hash must remain unchanged.

## 2. Historical PURE workflow

Historical workflow:

`.github/workflows/scientific-master-linear-dng-projection-v0-1.yml`

Classification: **PORT_READY_HOST**

The workflow already contains GCC Release, Clang Release and Clang ASan/UBSan jobs. On recovery it should be restored as a branch-local regression workflow first. Do not make a production claim until it executes green on the recovered current tree.

## 3. Historical 16-bit RGB LinearRaw restore

Historical module:

`docs/research/rgb-linearraw-output-restore-v0.2/`

Classification: **REFERENCE_COMPAT**

It represents the preferred finite 16-bit RGB/LinearRaw compatibility path with headroom/BaselineExposure logic and embedded preview behavior.

It must not redefine TRUTHRAW PURE because:

- it is finite unsigned 16-bit;
- negative values may clamp at the representation boundary;
- it uses a finite compatibility window.

Keep it as compatibility history / future interoperability option.

## 4. Current 16-bit Linear DNG

Current module:

`docs/research/linear-dng-projection-v0.1/`

Classification: **SUPERSEDED_CURRENT for the active bounded 16-bit LinearRaw role**

It is a current 3-channel unsigned-16 camera-native RGB LinearRaw compatibility projection and explicitly excludes floating-point preservation outside [0,1].

Therefore it is not a replacement for historical PURE Float32.

## 5. Historical raw projection exporter

Historical module:

`docs/research/raw-projection-export-v0.1/`

Classification: **REFERENCE_COMPAT**

It records three explicit downstream roles:

- reconstructed CFA `.rawsensor`;
- reconstructed CFA DNG;
- 16-bit LinearRaw compatibility.

The current architecture should retain the role distinctions but not treat this historical module as the PURE implementation.

TruthNegative now owns the explicit scientific-negative/sensor-negative research direction; reconstructed CFA compatibility remains downstream.

## 6. Output mode policy

Historical policy/code:

- `OutputModePolicy.kt`;
- four-mode UI;
- fail-safe unknown mode -> `TRUTHRAW_PURE`;
- PURE appearance sanitization to neutral.

Classification: **PORT_POLICY_ONLY**

Preserve these laws:

- PURE has no Colourful/Detailed/Soft/HDR treatment;
- appearance cannot modify Scientific Master/TruthRange/Backplane/evidence;
- unknown mode cannot silently become an appearance mode.

Do not blindly copy the old Android Activity structure into the newer TruthRaw Suite. Re-integrate the policy into the current app only after host PURE recovery is green.

## 7. Historical `TRCERT01` certificate v0.1

Historical module:

`docs/research/truthraw-certificate-v0.1/`

Classification: **LEGACY_VERIFY_ONLY**

Reason:

- real historical PURE artifacts exist with this schema;
- those artifacts must remain verifiable under the schema that created them;
- current `canonical/ptc/v1.1` is a different, newer certification system;
- silently reinterpreting old bytes under PTC v1.1 would destroy provenance.

New PURE artifacts should not use `TRCERT01` as if it were the current global certificate authority.

## 8. Historical DNG certificate embed v0.1

Historical module:

`docs/research/truthraw-dng-certificate-embed-v0.1/`

Classification: **LEGACY_VERIFY_ONLY**

Preserve enough code/tests to verify the historical artifact contract. New DNG certification must use a new versioned integration compatible with current PTC/Dynamic Authority policy.

## 9. Historical PURE artifact verifier

Historical tool:

`tools/verify_truthraw_pure_dng_artifact_v0_1.py`

Classification: **LEGACY_VERIFY_ONLY + REGRESSION FIXTURE**

Use it to prove that known historical PURE artifacts retain:

- exact source SHA binding;
- master SHA agreement;
- Float32 LinearRaw structure;
- finite negative/>1 component preservation;
- 1/1 evidence counts;
- legacy certificate integrity.

Do not silently relax its hard-coded historical build/schema expectations to accept a new-generation PURE file. A new verifier version is required for new artifacts.

## 10. Current PTC v1.1

Current:

`canonical/ptc/v1.1/`

Classification: **CURRENT CERTIFICATION AUTHORITY / BLOCKED_NEW_BINDING for PURE DNG integration**

PTC v1.1 is the newer fail-closed certification framework. Its own documentation still lists production DNG exporter integration as unfinished.

Therefore the new PURE generation should eventually carry a versioned PTC-compatible binding, but host pixel-recovery work must not wait for the certification schema to mutate the proven writer equations.

## 11. Dynamic Authority binding

Current Dynamic Authority/Open Scene sidecars did not exist in the historical PURE writer.

Classification: **BLOCKED_NEW_BINDING**

Rule for recovery:

- bind current Dynamic Authority identity to the PURE artifact lineage;
- do not feed Dynamic Authority into the pixel transform;
- do not alter the Scientific Master digest;
- do not mutate the frozen 180-byte Backplane;
- use a new versioned sidecar/certificate field/schema.

## 12. Host-first recovery order

1. Restore the historical Float32 PURE module **unchanged** on this recovery branch.
2. Restore its host workflow.
3. Run the historical synthetic writer tests.
4. Run the streaming Scientific-Master replay test.
5. Add one current-line regression that uses the current Scientific Master binding and requires exact digest equality before/after PURE export.
6. Only after those pass, design the new Dynamic Authority/PTC lineage binding.
7. Android/product mode reintegration comes last.

This order recovers proven mathematics before adding modern metadata.
