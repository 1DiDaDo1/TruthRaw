# D.RAW DNG Orientation Quarantine v0.1

Status: **REAL-MAIN EVIDENCE CANDIDATE**

This module handles exactly one observed invalid DNG/TIFF metadata condition:
the recovered main-camera source contains TIFF tag 274 (Orientation) with value
9.

TIFF Orientation is defined by values 1 through 8. The source value 9 is
therefore not interpreted as a valid presentation orientation.

## Scientific policy

The original source remains immutable.

The quarantine creates a **derived ingress container** whose only allowed byte
change is the invalid Orientation SHORT value:

`9 -> 1`

Value 1 means only:

`STORAGE_COORDINATE_NORMAL_ONLY`

It does **not** claim that the scene, camera, display or world orientation was
normal.

Presentation/world orientation remains:

`UNKNOWN`

## CFA invariant

The module hashes the serialized strip payload before and after derivation.

Admission requires:

- same byte length;
- exactly one changed byte for the observed little-endian source;
- Orientation storage must not overlap CFA strip payload;
- parent and derived CFA payload SHA-256 must be equal;
- parent source remains immutable;
- same physical observation;
- physical-frame increment = 0;
- independent-evidence increment = 0;
- createsNewEvidence = false;
- scientificWritebackAllowed = false.

The derived container is therefore an ingress compatibility representation of
the same sealed observation, not a second observation.

## Narrow scope

v0.1 intentionally only quarantines the exact observed invalid value 9 in a
classic TIFF/DNG with one CFA IFD and strip storage.

It does **not** rewrite valid TIFF Orientation values. For example, Orientation
2 is valid TIFF metadata even though TileNativeDngSource v0.1 does not support
mirrored orientations; v0.1 quarantine must reject it rather than normalize it.

Any other malformed orientation condition requires a separately versioned
policy.

## Real main-camera result

Original physical-source DNG SHA-256:

`a85cac58601d6cc8138bf8f9372e5161b85ab1e89afa70372f97c46461dcea79`

Derived ingress SHA-256:

`7c8eb85c568f6bc0ec3ae007de1658ec86d38b1144059d0fe0bc294a3e17bf08`

Serialized CFA payload SHA-256, both parent and derived:

`4818fd406cc528984ff57e036db49ac394ce0bb6b6d0830e2f9f1ad95ecc8c3c`

Manifest identity:

`a0f7abda03b7877f3c8709ba0dad9f24c28ed64be8bde5818a63a9fc17cb8361`

The current Host Scientific Route v0.1 rejected the immutable original at DNG
ingress with `unsupported Orientation`. The derived quarantine container then
passed the unchanged common scientific pipeline twice with bit-identical JSON
results.

That run is evidence that this metadata defect was the ingress blocker. It is
not permission to hide the parent/derived lineage.
