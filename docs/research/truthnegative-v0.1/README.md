# TruthNegative v0.1 — scientific-negative foundation

This module is the first implementation checkpoint for the TruthNegative research branch.

It intentionally performs **no image reconstruction yet**.

Its job is to make future reconstruction code fail closed if it tries to:

- create new evidence;
- change the physical frame/evidence count;
- claim target-lattice measurements without an admitted sampling model;
- mutate the sealed source identity;
- collapse reconstructed and measured authority.

## Core contract

A TruthNegative manifest binds three domains:

1. `sourceEvidenceGrid`
2. `reconstructionDomain`
3. `projectionGrid`

The source grid can contain measured evidence.

The reconstruction domain can represent richer continuous/latent structure.

The projection grid is only a finite sampling request.

When `samplingModelStatus=UNRESOLVED`, the target projection is forbidden from claiming any `MEASURED` samples.

## Current Camera-5 example

Source:

`4080 x 3072 = 12,533,760` source sample sites.

Research projection:

`16320 x 12288 = 200,540,160` target sample sites.

Ratio:

`16x` sample count, `4x` linear dimensions.

That ratio is representational. It is not evidence that 15 new physical sensor samples exist around every source sample.

## Run locally

`python tools/truthnegative_foundation_v01.py --self-test`

or create a manifest:

`python tools/truthnegative_foundation_v01.py --source-sha256 <64-hex> --source-width 4080 --source-height 3072 --target-width 16320 --target-height 12288`

## Next module

TN-1 will bind an actual sealed CFA identity and add an identity/no-op source-resolution reconstruction before any dense projection algorithm is admitted.
