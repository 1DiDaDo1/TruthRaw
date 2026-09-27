# D.RAW Source Admission Package v0.2 — Physical Source-Local Admission

Status: **MAIN-CAMERA ADMITTED_SOURCE_LOCAL — HOST VALIDATED**

v0.2 is the physical-identity successor to Source Admission Package v0.1.

It admits a physical observation only after all of these already pass:

1. Source Pre-Admission v0.2;
2. Scientific Ingress Lineage Binding v0.1;
3. Observation Record v0.4;
4. Source Capability Envelope v0.2.

## Meaning of ADMITTED_SOURCE_LOCAL

`ADMITTED_SOURCE_LOCAL` means the following are coherently bound to one
physical observation:

- immutable physical source;
- compatibility-ingress lineage;
- current scientific pipeline result;
- Scientific Master;
- TruthNegative Continuous parent;
- sealed D.RAWnegative v0.1 computational lineage;
- stable physical Free World graph node;
- explicit capability map.

It does **not** mean:

- sensor/readout semantics are certified;
- capture sample domain is known;
- sensor pixel mode is known;
- calibration is shared with another lens;
- radiometry is absolute;
- cross-observation equality is proven;
- any fusion is allowed.

## Main admission

Physical observation ID:

`DRAW_PHYSICAL_OBS_a85cac58601d6cc8138bf8f9372e5161b85ab1e89afa70372f97c46461dcea79`

Physical graph node:

`029b845daf0bb20ebed4e7ce6d66eb61a172dd89e9977f996d20d604c2703353`

Observation Record v0.4:

`aaf2dcf0b81ebe5debcc2fa6a0b40057e5f1cf20e0d6e2df67a54fd825853fc5`

Capability Envelope v0.2:

`057c627bf172521d9336386d4f74ae1e26664c70c4a24ed7993b6c16c33a3986`

Final admission state:

`c20104981a0373f0d2f7c03216272213ff614f0dc732b13247cf2a0c0c6a9b7a`

Capture sample domain, readout domain and sensor pixel mode remain UNKNOWN after
admission.

Graph relations and fusion admissions are empty.
