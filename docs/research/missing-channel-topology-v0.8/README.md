# TruthRaw missing-channel topology v0.8

Status: `HELDOUT_CFA_TOPOLOGY_NECESSARY_CONDITION_PASS_CO_SITED_CERTIFICATION_OPEN`.

This research layer hides real measured CFA samples and tests whether local spatial/channel topology can recover their ordering and curvature without using the hidden value. It is a necessary-condition study only.

It **must not** set reconstructed missing-channel `topologyCertified=true`. A Bayer pixel lacks co-sited measurements for two colour components; held-out same-lattice measurements do not manufacture that absent ground truth.

The three dog captures were also tested as a possible phase-diversity calibration set and explicitly **FAILED** the registration gate, so they are not reused as co-sited ground truth.

Git stores the reproducible evaluator, aggregate summary and exact SHA-256 identity of the full result evidence. The full 67,250-byte result payload and 25-MB source DNGs remain external evidence rather than repository payloads.

See `REPORT_v0_8.md`, `TOPOLOGY_HELDOUT_SUMMARY_v0_8.json`, `DOG_3RAW_PHASE_DIVERSITY_QUALIFICATION_v0_8.json`, and the capture protocol.
