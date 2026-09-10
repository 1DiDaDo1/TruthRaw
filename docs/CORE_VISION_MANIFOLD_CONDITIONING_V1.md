# TruthRaw core-vision extension — Manifold Conditioning v1

The Virtual Observation Manifold is an analysis coordinate space, not a stack of captures.

TruthRaw may inspect one sealed evidence root through arbitrarily many EV coordinates. The safe default is **exact reparameterization**: values and uncertainty are scaled into a numerically convenient coordinate, an evidence-equivalent computation is performed, and the result is mapped back to the same latent scene coordinate.

A manifold view may change an estimate only through **robust model selection** when a candidate was frozen before evaluation and enough independent held-out source scenes satisfy explicit scalar-error and topology gates. Development evidence does not count toward promotion. Duplicate views of one physical source do not count as independent scenes.

If those gates are not met, the canonical reconstruction remains authoritative.

Binding rule: **unlimited representations; finite evidence.**
