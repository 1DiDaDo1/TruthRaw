# TruthRaw conservation/restoration authority model — v0.1

Status: **RESEARCH FOUNDATION / GOVERNANCE MODEL / NO SCIENTIFIC AUTHORITY UPGRADE**

Date: 2026-09-16

TruthRaw has repeatedly used the restoration of paintings and photographs as a conceptual test for how reconstruction should behave. This document makes that analogy explicit and turns it into a machine-oriented scientific rule set.

Permanent TruthRaw rule:

> **Measured where measured. Reconstructed where necessary. Never invented.**

Conservation/restoration adds a second operational rule:

> **Preserve the original, document the condition, separate intervention from original material, and make compensation for loss detectable and reversible where practicable.**

The purpose is not to claim that digital RAW reconstruction and physical art conservation are the same discipline. The purpose is to import the strongest provenance and intervention principles where they genuinely map.

## 1. Conservation sequence that maps to TruthRaw

Professional conservation practice generally separates:

`examination -> condition documentation -> scientific investigation -> stabilization -> treatment/restoration -> documentation of intervention`

TruthRaw maps this to:

`Source Evidence -> condition/evidence assessment -> measurement/scientific analysis -> stable Scientific Master -> bounded reconstruction/loss compensation -> appearance/presentation -> permanent provenance`

The order matters. Aesthetic completion must not silently overwrite the evidence record.

## 2. Original material is not a workspace

In paintings conservation, conservators avoid covering or removing original paint merely to obtain a more seamless cosmetic result. In photograph preservation, the original object/file is retained and reproductions/surrogates are preferred when intervention or access would risk the original.

TruthRaw equivalent:

- admitted source RAW/CFA bytes remain immutable;
- measured CFA samples retain their measured identity;
- reconstructed channels do not overwrite the fact that the corresponding source channels were absent;
- an exported DNG, HDR image, repaired image or appearance render is a derivative, not a rewritten source.

## 3. Condition report before restoration

A conservator first records what is present, damaged, missing, altered, uncertain or previously restored. Condition reporting commonly uses normal-light images plus detail images and, where useful, raking light, ultraviolet or infrared examination.

TruthRaw should therefore model a scene/data condition report before restoration-like reconstruction:

- exact source identity;
- CFA/sample support;
- saturation/censoring;
- dead/missing/corrupt samples when known;
- local uncertainty/support;
- blur/motion/focus support;
- shading/illumination evidence;
- colour/calibration authority;
- prior processing/provenance when known.

This belongs upstream of appearance.

## 4. Stabilization before aesthetic reintegration

Conservation distinguishes stabilization from retouching. A painting may be structurally stabilized before any inpainting is undertaken.

TruthRaw equivalent:

1. first establish a numerically and scientifically stable state;
2. then reconstruct missing quantities under explicit authority;
3. only then apply optional appearance/finishing.

Examples:

- exact source/CFA admission before demosaic/reconstruction;
- F64 branch-sensitive reconstruction before optional F32 storage;
- uncertainty/support binding before confidence-dependent enhancement;
- scientific HDR state before display tone/gain-map adaptation.

## 5. Compensation for loss is not original material

The American Institute for Conservation states that compensation for loss should be documented, detectable by common examination methods, reversible where practicable, and should not falsely modify known characteristics or obscure original material.

TruthRaw equivalent:

- `RECONSTRUCTED` remains reconstruction;
- `CENSORED` remains a bound rather than an invented exact value;
- `UNKNOWN` remains unknown when there is insufficient support;
- `COUNTERFACTUAL` remains hypothetical;
- appearance-only retouching cannot become `MEASURED`.

A visually seamless reconstruction is allowed. A provenance-seamless reconstruction is not.

## 6. Inpainting versus overpainting

Paintings conservation distinguishes inpainting of actual losses from overpainting that covers surviving original material. The latter can obscure evidence and is therefore much more problematic.

TruthRaw mapping:

- filling an unmeasured colour at a Bayer location is analogous to loss compensation and must remain `RECONSTRUCTED`;
- replacing a valid measured sample because a model predicts a prettier value is analogous to overpainting and is forbidden in the scientific master unless the original sample has separately failed an evidence/validity gate;
- measured-channel exactness tests are therefore the digital equivalent of protecting surviving original material.

## 7. Reversibility becomes computational retreatability

Physical conservation cannot always guarantee literal reversibility, but modern practice strongly values retreatability and clear identification of added materials.

Digital TruthRaw can be stricter:

Every reconstruction/appearance operation should be replayable or removable from provenance without changing the sealed source. A downstream product should retain enough identity information to recover:

- the source evidence identity;
- the Scientific Master identity;
- the authority map/version;
- the transformation/version that produced the derivative.

This is one role of the Technical Backplane.

## 8. Detectability becomes authority masks and hashes

Conservation additions should be detectable by examination. TruthRaw can make intervention detection exact rather than merely visual:

- per-site authority classes;
- reconstruction/support masks;
- censor/unknown masks;
- module/version identity;
- hashes of source/master/authority artifacts;
- explicit export role.

A viewer may hide these overlays for presentation, but the underlying provenance must remain recoverable.

## 9. Multi-modal examination does create additional evidence only when it is actually measured

Conservators may use raking light, UV fluorescence, infrared photography, XRF or other examinations to reveal facts not visible under normal illumination. These are genuinely additional measurements because new physical observations were made.

TruthRaw consequence:

- a second independent sensor modality or calibration measurement may legitimately add evidence;
- a virtual exposure, relight, generative fill, tone curve or re-render does not;
- `independentEvidenceCount` changes only when a genuinely independent physical observation is admitted by an explicit contract.

The current single-frame core remains `physicalFrameCount=1`, `independentEvidenceCount=1`.

## 10. Photograph restoration

For damaged historical photographs, preservation practice strongly prefers preventive care, safe digitization and retention of the original. A digital restoration is therefore best understood as a derivative interpretation/surrogate, not a rewrite of the historical artifact.

TruthRaw mapping for a future photo-restoration mode:

`Original scan/capture evidence -> condition mask -> defect classification -> evidence-bounded repair estimate -> restoration layer -> appearance export`

The repaired pixel must carry a reconstruction/restoration label even if it is visually indistinguishable from its neighbours.

## 11. Light, colour, detail and restoration meet in one scene state

Restoration is especially useful to TruthRaw because it exposes why light, colour and structure cannot be repaired independently.

A faded/abraded/discoloured region may involve:

- missing material;
- altered pigment/dye spectra;
- varnish/aging layer;
- changed surface roughness and therefore changed specular response;
- deformation/geometry change;
- altered illumination during capture.

Similarly in RAW reconstruction, an apparent colour/detail defect may be caused by illumination, optics, focus, CFA sampling, saturation, noise or calibration.

Therefore the Restorer must consume authority-bound scene state rather than blindly editing RGB.

## 12. Proposed TruthRaw restoration authority classes

These are restoration-layer roles, not replacements for the core Dynamic Authority classes:

- `ORIGINAL_MEASURED_SUPPORT` — surviving admitted measured support;
- `CONDITION_OBSERVED` — observed defect/condition information;
- `STABILIZED_DERIVED` — derived correction that preserves scientific provenance;
- `LOSS_COMPENSATION_RECONSTRUCTED` — bounded reconstruction of a missing/damaged quantity;
- `AESTHETIC_REINTEGRATION_ONLY` — presentation retouching, no scientific writeback;
- `UNRESOLVED_LOSS` — insufficient support; must remain open.

These map back onto core authority. In particular, loss compensation never becomes `MEASURED`.

## 13. Fail-closed requirements

A restoration operation must fail closed if it would:

- overwrite valid measured support without an explicit invalidation reason;
- remove the ability to distinguish reconstructed/restored content from source evidence;
- treat missing/unknown support as zero uncertainty;
- use appearance similarity as proof of physical correctness;
- turn historical/source metadata into independent calibration;
- increase evidence count without a new physical observation;
- make a derivative export the new source of truth.

## 14. Source references used for this research foundation

Primary professional conservation/preservation sources:

- American Institute for Conservation, Code of Ethics and Guidelines for Practice — compensation for loss and documentation: https://www.culturalheritage.org/conservation-at-work/uphold-professional-standards/code
- Smithsonian American Art Museum, Painting Conservation Studio — cleaning/inpainting, reversible materials, protecting original paint: https://americanart.si.edu/art/conservation/center/paintings-studio
- Canadian Conservation Institute, Caring for paintings — inpainting, reversibility, condition/treatment reporting: https://www.canada.ca/en/conservation-institute/services/preventive-conservation/guidelines-collections/paintings.html
- Canadian Conservation Institute, Condition Reporting — visible/raking/UV/IR documentation: https://www.canada.ca/en/conservation-institute/services/conservation-preservation-publications/canadian-conservation-institute-notes/condition-reporting-paintings-introduction.html
- Library of Congress, Photographs FAQ and digitization/preservation guidance — retain originals and use facsimiles/surrogates where appropriate: https://www.loc.gov/preservation/about/faqs/photographs.html
- Library of Congress, Preservation Facsimile: https://www.loc.gov/preservation/care/photocpy.html
- Getty, conservation case studies and reversible retouching barriers: https://www.getty.edu/news/edgar-degas-self-portrait-conservation/

## 15. Engineering consequence

TruthRaw's historical `Restorer` room should now be interpreted as a provenance-preserving scientific reconstruction service, not as a beauty filter.

It may:

- detect/characterize defects and missing support;
- stabilize a derived scene representation;
- reconstruct only within an explicit evidence/uncertainty contract;
- maintain a separate restoration mask and transformation identity;
- hand a later appearance layer optional aesthetic reintegration.

It may not:

- rewrite sealed source evidence;
- call visually plausible repair measured truth;
- use semantic hallucination as scientific microstructure;
- erase uncertainty or provenance.

This conservation/restoration model should be carried forward into the next Open Scene State / Dynamic Authority runtime iteration.
