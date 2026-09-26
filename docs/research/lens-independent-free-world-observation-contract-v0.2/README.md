# Lens-Independent Free World Observation Contract v0.2 — D.RAWnegative

Status: **CURRENT CANONICAL SUCCESSOR CONTRACT**

Parent v0.1 is sealed and remains unchanged.

v0.2 changes one architectural identity:

`TruthNegative per observation`

becomes the current public identity:

`D.RAWnegative per observation`

while requiring the historical TruthNegative Continuous state as explicit
legacy ancestry for v0.1 D.RAWnegative.

## Required chain

```text
Source Evidence
 -> Scientific Master
 -> authority field
 -> TruthNegative Continuous parent
 -> D.RAWnegative
 -> Free World Observation Graph
```

## Compatibility

Historical TruthNegative identifiers remain stable where changing them would
break reproducibility, ABI, schema, wire format, hashes or existing artifacts.

The old name is compatibility ancestry, not the current product-facing name.

## Gauge

Every D.RAWnegative must carry a scale gauge.

A source-local gauge cannot enable cross-observation radiometric equality or
fusion.

## Precision

Float64 branch-sensitive compute and validated Float32 canonical storage remain
the current precision contract. Neither changes evidence authority.

## Container boundary

Legacy `.tnc` does not automatically become a native D.RAWnegative container.
A future native D.RAWnegative format requires a new explicit schema/magic.
