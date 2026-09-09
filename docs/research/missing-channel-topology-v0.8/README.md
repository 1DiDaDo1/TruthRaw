# TruthRaw missing-channel topology v0.8

Status: `HELDOUT_CFA_TOPOLOGY_NECESSARY_CONDITION_PASS_CO_SITED_CERTIFICATION_OPEN`.

This research layer hides real measured CFA samples and tests whether local spatial/channel topology can recover their ordering and curvature without using the hidden value. It is a necessary-condition study only.

It **must not** set reconstructed missing-channel `topologyCertified=true`. A Bayer pixel lacks co-sited measurements for two colour components; held-out same-lattice measurements do not manufacture that absent ground truth.

The three dog captures were additionally tested as a possible phase-diversity calibration set and explicitly **FAILED** the registration gate, so they are not reused as co-sited ground truth.

See `REPORT_v0_8.md`, `DOG_3RAW_PHASE_DIVERSITY_QUALIFICATION_v0_8.json`, and the compressed `TOPOLOGY_HELDOUT_RESULTS_v0_8.json.zlib`.
