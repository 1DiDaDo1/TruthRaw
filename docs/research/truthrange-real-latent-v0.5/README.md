# TruthRange real-DNG latent bridge v0.5

Status: **REAL_DNG_LATENT_DENSE_TRUTHRANGE_RESEARCH_PASS**

This module connects the closed v0.4 real-DNG Stage-2 bridge to the v0.2 Latent Camera Scene and the v0.3 uncertainty/TruthRange contracts using the exact frozen v5.0g feature/runtime contract.

## Pipeline

`sealed HONOR DNG -> v0.4 Stage-2 -> v4.7i/v0.2 Latent Camera Scene -> self-gauge -> exact v5.0g measured-role anchors -> tiled dense uncertainty transport -> TruthRange`

### Closed bindings

- v4.7i `core.cpp` SHA-256: `68f52c4896d47c4605adc1cf670e55f95d42a687e18c06a167028a64ce37b94c`
- v4.7i `core.h` SHA-256: `b7f6e2189d6ecccfc5fda80084041990bd12757145c5ff2c40f2ae0d0046b167`
- v5.0g model SHA-256: `8831bee921999e823466cfc462812e40620b2834080c0f7d64c6f11d7ead626f`
- v5.0g feature schema SHA-256: `8c8e56b762b83a5a846c5201ab3ba43c9a2549d896873cc0ceb66574e74a83e3`

## Frozen-role quirk

The historical training extractor iterated `B,G1,G2,R` while the final feature labels read `role_R,role_G1,role_G2,role_B`. v0.5 reproduces that training behavior exactly. Runtime role codes remain `R=0,G1=1,G2=2,B=3`. This is compatibility with the frozen model, not a semantic reinterpretation.

## Real evidence

Eight 4080x3072 BnCam/HONOR tele DNGs, ISO 100 through 12800:

- Stage-2 parity to v0.4: **8/8 exact (`max_abs=0`)**
- measured CFA reinjection: **8/8 exact (`max_abs=0`)**
- independent probe parity: **64 probes**, exactly two per R/G1/G2/B per capture
- max feature abs error: `4.76837158203125e-07`
- max scalar abs error: `1.9073486328125e-06`
- tile128 vs tile256: **8/8 exact**
- ISO12800 full-frame single tile (`8192`) vs tile256: **exact**

The dense contract represents 37,601,280 pixel-channel entries per full frame without materializing multi-GB per-entry structs.

## Scientific boundary

The reconstructed-channel v5.0g field is still a **transport proxy** from measured-role anchors. It is not a certified missing-channel topology model and it does not create co-sited measured RGB evidence. RGB covariance remains unresolved.
