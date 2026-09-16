# Authority digest binary specification v1.9

`TruthRawDynamicAuthorityRGBFieldDigest/1.9` is a deterministic scientific identity over three camera-native RGB authority streams. It is not an image container.

The common little-endian header begins with magic `TRDAF019`, version `1`, the exact header byte count, width `4080`, height `3072`, channel count `3`, record size `16`, camera-native RGB order, BGGR source CFA, direct Stage-2 authority `CALIBRATED_ESTIMATE`, and zero flags. It then contains the raw 32-byte SHA-256 values for the source DNG, decoded CFA, Scientific Master, v5.0g uncertainty model, and v5.0g uncertainty binding; the float64 little-endian bits of `L0`; and a 32-byte SHA-256 of the fixed authority-policy sentence frozen by the audit implementation.

Each channel stream hashes `header || channel_id || records_in_global_raster_order`. Every record is exactly 16 bytes: byte 0 authority (`1=CALIBRATED_ESTIMATE`, `2=RECONSTRUCTED`, `3=CENSORED`, `4=UNKNOWN`), byte 1 flags (`bit0=SCENE_VALUE_GE_BOUND` for a censored highlight), bytes 2..3 zero, bytes 4..7 float32 little-endian scene estimate bits or zero, bytes 8..11 float32 little-endian p95 uncertainty bits or zero, and bytes 12..15 float32 little-endian censor-bound bits or zero.

For an uncensored physical CFA channel, the estimate is exact Stage-2 and p95 is the DNG NoiseProfile Gaussian-equivalent Stage-2 uncertainty. For each uncensored missing colour, the estimate is the exact float Scientific-Master channel and p95 is the v5.0g measured-role-anchor local-max-transport proxy with radius 40. At a source-white-censored CFA site, the measured channel is `CENSORED` with the Stage-2 source-white lower bound and both missing colours are `UNKNOWN`.

The combined field digest hashes `header || R_digest || G_digest || B_digest || four uint64 little-endian global authority counts` in the order calibrated, reconstructed, censored, unknown. Compute band/tile boundaries are excluded from all hashed scientific content.
