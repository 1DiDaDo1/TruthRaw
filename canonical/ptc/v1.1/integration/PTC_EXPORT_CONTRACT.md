# TruthRaw exporter contract — PTC-1.1

Before an export can receive `PURE_TRUTH_CERTIFIED`:

1. Freeze source SHA-256 and source class.
2. Freeze admission decisions, including GainMap present/application count.
3. Bind the exact reconstruction backend binary/source hash to the output.
4. Freeze a scene-linear Scientific Scene Master hash before appearance/rendering.
5. Assert no generated scene content, semantic detail generation, invented clipped detail, or hidden extra ISP.
6. Compute the export media-payload digest before/independent of PTC metadata.
7. Evaluate PTC fail-closed.
8. Embed copyright + PTC XMP only after evaluation.
9. Recompute media-payload hash and require exact equality.
10. Optionally publish a complete final-container SHA-256 externally.

### DNG
Set TIFF tag 33432 to:

`Copyright © TruthRaw Project. Pure Truth Certificate PTC v1.1. Reconstruction provenance embedded.`

Write PTC XMP to TIFF/DNG tag 700 using the production DNG writer. Never rebuild a production DNG through a generic TIFF library just to add the certificate.

### JPEG
Insert standard XMP APP1. PTC's reference writer is metadata-only and verifies the JPEG media-payload hash is bit-identical before/after injection.
