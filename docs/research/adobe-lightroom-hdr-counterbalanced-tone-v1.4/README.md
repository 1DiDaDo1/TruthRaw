# TruthRaw — Adobe Lightroom HDR counterbalanced tone v1.4

**Status: RESEARCH — NOT MAIN-PROMOTED**

## Finding

A new controlled experiment from the same single-frame HONOR tele RAW/DNG shows that Lightroom can redistribute HDR presentation range very aggressively when global light controls are lowered while the custom tone curve is raised.

This does **not** create new scene evidence and does **not** recover exact radiance for source-censored CFA samples.

## Same scientific source

Immutable source authority remains:

- `IMG_BNC_TRUTHRAW20260907_094449_565.dng`
- source SHA-256 `7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67`
- decoded CFA SHA-256 `883cbe13fd2a5afe7e9f3f4147df3a8ed3be681340a3c0422933f42d0f4d719c`
- 217 source-white samples remain censored.

## Control AVIF `(11)`

Transport:
- P3-D65
- SMPTE ST 2084 / PQ
- 10-bit YUV 4:4:4
- HDR Limit +8 EV

Lightroom state:
- Exposure `0.00`
- Highlights `0`
- Whites `+2`
- ordinary tone curve is identity
- extended HDR tone curve is identity `(0,0) -> (500,500)`

Observed render:
- file SHA-256 `38e1942429dcb1f6c561820379069e3dcb3dc860fb67de1ef261c3459d37220a`
- MaxCLL 1391 nits
- decoded luma median 495/1023
- p99 697
- p99.9 722
- p99.99 732
- max 810
- no decoded luma code at 1023.

## Counterbalanced AVIF `(12)` / `(13)`

Both use the same transport and the same Lightroom edit recipe:
- Exposure `-5.00`
- Highlights `-100`
- Whites `-100`
- curve starts `(0,11) -> (17,255)` and then remains at 255 through the normal curve
- extended HDR curve `(0,11) -> (25,500)`.

Observed decoded render:
- MaxCLL 10000 nits
- decoded luma median 483/1023
- p99 684
- p99.9 816
- p99.99 889
- max 1023
- fraction exactly at 1023: `2.0532088675433894e-05`
- fraction >=1000: `2.5121614379354414e-05`.

Thus the bulk of the image is slightly darker than the control while the extreme highlight tail is stretched much farther upward. This is an HDR **presentation redistribution**, not measured dynamic-range creation.

`(12)` and `(13)` have different whole-file SHA-256 values, but their decoded primary color stream hashes are identical and their decoded gain-map stream hashes are identical. Their pixel render is therefore the same; the byte-level container difference is provenance/metadata, not a different HDR image.

## DNG `(2)`

The new Adobe DNG stores the counterbalanced edit recipe above in XMP while its compressed RAW sub-IFD payload is byte-identical to the prior Adobe DNG:

- 192/192 raw tiles identical
- concatenated compressed raw tile payload SHA-256 in both files:
  `830f530916444f8a90826b5b74ea87c1dce525ba7f59cf2f1fd09d9b0082ded5`

The prior Adobe DNG was already proven to decode to the immutable source CFA with zero sample mismatches. Therefore the new DNG remains a derived recipe-bearing carrier of the same measured CFA, not new evidence.

## Scientific interpretation

The experiment demonstrates three distinct quantities that must not be collapsed:

1. **Scientific scene authority** — measured/calibrated/reconstructed state rooted in one physical CFA exposure.
2. **HDR presentation mapping** — exposure, highlights, whites and tone curve decide how that state is distributed over the output address space.
3. **Encoded/display ceiling** — PQ can reach 10,000 nits, but touching that ceiling is not proof that the source scene was measured at 10,000 nits.

The counterbalanced recipe is especially informative because it does not simply lift the whole image. It lowers the bulk while extending a sparse highlight tail. This is a presentation technique that TruthRaw may later emulate only after the Scientific Master and Dynamic Authority Field have decided what values are supportable.

## Permanent authority law

Adobe tone state, MaxCLL, gain maps, PQ code values and display limits never write authority back to the TruthRaw Scientific Master.

Censored source samples remain censored unless a separate TruthRaw reconstruction result provides explicit support and uncertainty. Presentation controls cannot turn a lower bound into an exact measured radiance.
