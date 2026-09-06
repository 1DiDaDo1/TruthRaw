# Current TruthRaw -> PURE_TRUTH_CERTIFIED FULL_PHYSICAL gap

The PTC implementation is active and fail-closed. The current real Nature Truth example evaluates as **PURE_TRUTH_DERIVED**, not certified.

Remaining FULL_PHYSICAL blockers detected automatically:

1. backend-bound calibrated uncertainty for the exact reconstruction backend;
2. real per-lens color calibration;
3. real illuminant calibration bound to the capture/source class;
4. electron/PTC calibration bound to the sensor/lens mode;
5. optical calibration (PSF/MTF/CA/flare/color shading) bound to the lens/mode.

Operational production items still required:

- publish/configure the permanent TruthRaw Ed25519 public signing identity;
- connect the DNG exporter/DNG SDK so TIFF Copyright tag 33432 and XMP tag 700 are written without a generic TIFF rewrite;
- add final-device Magic8 Pro/Vulkan and DNG-reader interoperability certification.

No missing blocker is waived by the certificate engine.
