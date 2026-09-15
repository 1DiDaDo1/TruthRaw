# TruthRaw open-world Structure Evidence runtime v0.4

Status: **RESEARCH IMPLEMENTATION — NOT MAIN PROMOTED**

## Purpose

v0.4 moves Structure Evidence from caller-supplied abstract support values toward the actual TruthRaw evidence chain. It binds structure authority to the exact reconstruction family, the prospective tele uncertainty result, the held-out CFA topology study, censoring, and (when it exists) independently validated optical MTF support.

The world remains open. Only source evidence and authority claims are constrained.

## Canonical evidence bound by this runtime

The runtime reads and cross-checks the repository's actual canonical evidence:

- `canonical/uncertainty/v5.0g/BACKEND_BINDING_v5_0g.json`
- `canonical/uncertainty/v5.0g/FROZEN_PROTOCOL_v5_0g.json`
- `canonical/uncertainty/v5.0g-p1/PTC_TELE_READINESS_v5_0g_p1.json`
- `canonical/uncertainty/v5.0g-p1/PROSPECTIVE_TELE_HOLDOUT_RESULT_v5_0g_p1.json`
- `docs/research/missing-channel-topology-v0.8/TOPOLOGY_HELDOUT_SUMMARY_v0_8.json`

It refuses to continue if those bindings disagree.

## Important domain correction discovered in v0.4

The prospective v5.0g-p1 uncertainty PASS is source-class bound. The actual prospective holdout metadata is **4080×3072 BGGR**, HONOR BKQ-N49, 22.48 mm tele, and the exact bound reconstruction backend.

Therefore v5.0g-p1 does **not** currently authorize uncertainty claims for the separate **16320×12288 Camera-5 maximum-resolution RAW** route.

This is not evidence against the 200 MP route. It is a calibration-domain boundary: native-200MP capture proof and 200MP reconstruction uncertainty are different questions. A genuine 16320×12288 uncertainty validation campaign is required before the v0.4 structure runtime may use the v5.0g-style uncertainty authority there.

## Runtime gates

### 1. Exact measured-channel reinjection

For every tile pixel, the CFA-measured component in reconstructed RGB must equal the Stage-2 CFA value exactly. Any mismatch blocks Structure Evidence.

This connects the gate directly to the central v4.7i rule: measured where measured.

### 2. Measured CFA structure

The runtime inspects only same-colour Bayer pairs separated by two pixels along cardinal axes. Censored pairs are excluded. A pair contributes measured structure only when its Stage-2 difference exceeds two times the combined source-noise sigma.

This is evidence from the measured mosaic, not from sharpened/display RGB.

### 3. Missing-channel topology remains a proxy

The v0.8 topology study remains explicitly non-co-sited. v0.4 therefore stores `topology_certified=false` and uses the conservative minimum of worst-fold ordering and worst-fold curvature only as a ceiling on reconstructed topology support.

No rendered texture can promote that proxy to measured truth.

### 4. Uncertainty confidence

For an admitted v5.0g-p1 source, missing-channel p95 bands are summarized relative to local measured CFA contrast. The dimensionless support is capped by the actual prospective p95 coverage. This is a detail-support quantity, not a physical probability or electron-domain claim.

### 5. Censoring

Censoring risk is the fraction of source CFA samples in the runtime region that are censored. Full censoring removes both measured and reconstructed structure admission.

### 6. Optical MTF fail-closed gate

Without an independently hold-out-validated optical MTF support record for the exact source domain, adaptive v4.7j detail and v4.7k output acutance stay **Neutral**.

The runtime still reports the pre-optics CFA/topology/uncertainty diagnostics so future calibration can be attached without recomputing or inventing evidence.

When an admissible optics record is supplied, Structure Evidence may authorize **appearance-domain** detail only. It can never write transformed pixels back into the Scientific Master or claim that sharpening created measured detail.

## Test coverage

`tests/test_open_world_structure_runtime_v04.py` verifies:

- the real canonical v5.0g/v5.0g-p1/v0.8 files load and cross-bind;
- topology stays uncertified;
- exact measured reinjection is a hard gate;
- measured CFA support is derived before appearance;
- absent optics forces Neutral detail;
- a domain-matched hold-out optics fixture can admit appearance only;
- full censoring removes structure authority;
- most importantly, 16320×12288 is rejected by the current 4080×3072 uncertainty certification.

## Next gates

1. instrument or export real per-tile v5.0g-p1 uncertainty bands from the current 4080×3072 tele processing path and run v0.4 on real captures;
2. perform independent Camera-5 optics/MTF calibration with hold-out evidence;
3. build a separate maximum-resolution 16320×12288 uncertainty campaign rather than reusing the 4080×3072 certification;
4. after those gates pass, route the v0.4 decision into actual v4.7j/v4.7k runtime selection while preserving appearance-only authority;
5. keep co-sited topology certification open until independent phase-diverse/reference evidence exists.

**Measured where measured. Reconstructed where necessary. Never invented.**
