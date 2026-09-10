# TruthRaw Room ABI v0.1

Research integration layer that turns Building Runtime v0.1 resource policy into explicit native room memory/lifetime contracts without modifying canonical scientific algorithms.

## Contract

- truth authority and resource authority remain orthogonal;
- corridor authority preserves one physical frame, one independent evidence source, and an unmodified scientific master;
- enum values crossing the ABI are validated before use;
- Room Capsule leases require a Counterfactual-floor corridor token;
- memory leases are external, explicit, bounded, aligned, and owned by one room;
- the actually granted capacity may never exceed the effective runtime room budget, even when a host receives a larger preferred request;
- a forged `valid` resource policy with zero positive-lease budget fails closed;
- transient scratch and rebuildable cache can be released under pressure;
- rebuildable cache may never contain the sole copy of sealed evidence, immutable provenance, or the scientific master;
- lease failure is explicit and cannot silently select a weaker scientific method;
- current Manifold Conditioning adapter performs no allocations;
- current Room Capsule adapter converts its existing upstream memory plan into external leases and reserves its full conservative planner peak;
- the v4.7i bridge changes only tile-core size and thread count in a caller-supplied `ProcessOptions` copy.

The module does **not** yet own the newer Tile-Native DNG Source or Full-Frame Streaming handles. Those are the next ABI-extension target after v0.1 is proven against real upstream CI.

Real-upstream GitHub CI run `34537486512` passed GCC Release, Clang Release, and Clang ASan/UBSan against the sealed upstream bindings. Current status: `RESEARCH_CANDIDATE_REAL_UPSTREAM_CI_PASS`.
