# TruthRaw Building Runtime v0.1 — room-role map

This maps the current project into execution roles without silently renaming or changing the authority of existing modules.

| Room | Primary role | Truth floor | Default status | Heavy lease |
|---|---|---:|---|---|
| Archivist | sealed evidence, hashes, immutable provenance | Foundation | Available | no |
| Measurement Lab | source normalization / measured camera facts | Measurement | Available | no |
| Architect | canonical reconstruction / scene construction | Reconstruction | Available | yes |
| Restorer | missing-channel/detail restoration candidates | Reconstruction | ResearchOnly | yes |
| Scene Registry | Scene Master / TruthRange / zero-line identity | Scene | Available | no |
| Surveyor | uncertainty/covariance/topology knowledge | Scene | ResearchOnly | yes |
| Manifold Conditioning | evidence-neutral numerical conditioning | Scene | Available | yes |
| Lighting Studio / CICM | counterfactual illumination/capture worlds | Counterfactual | ResearchOnly | yes |
| Room Capsule | local sparse counterfactual lighting domain | Counterfactual | ResearchOnly | yes |
| Colorist | colorimetric/appearance transform consumer | Appearance | Available | yes |
| Finisher | tone/S-curve/acutance/appearance finishing | Appearance | Available | yes |
| Exporter | DNG/SDR/HDR/projection output | Projection | Available | yes |

## Flexibility rule

A future device-specific plan may split a role into multiple rooms, merge implementation workers, or place compatible rooms on different CPU/GPU lanes. What may **not** move is the typed corridor authority: a worker cannot gain access to a lower truth floor, cannot create independent evidence, and cannot turn an appearance/counterfactual result into measured fact.

Therefore the floor plan is flexible in **execution topology**, while TruthRaw's evidence/claim hierarchy remains fixed.
