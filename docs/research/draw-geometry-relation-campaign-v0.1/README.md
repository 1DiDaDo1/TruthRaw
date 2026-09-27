# D.RAW Geometry Relation Campaign v0.1

Status: **PENDING MATCHED-SCENE CAPTURE**

This is the first explicit inter-lens relation campaign in the current D.RAW Free World.
It targets only the GEOMETRY axis between the independently admitted MAIN and ULTRA_WIDE physical observations.

The existing main observation (2026-09-14) and ultra-wide observation (2026-09-27) are different capture sessions and different scenes. They are therefore reference endpoints only and are forbidden as a geometry-fit pair. The relation remains UNKNOWN, with no certificate, no coordinate-transform capability and no fusion.

The capture protocol keeps the phone/camera system rigid and moves an indexed planar metric target. Each pose is captured with MAIN and ULTRA_WIDE without moving the device or target between the pair. D.RAW requires at least 12 training poses and 4 disjoint held-out poses; these counts are project protocol choices rather than external standards. Coverage includes centre, edges, corners, near/mid/far distances and tilted target poses.

Every capture must first be admitted source-local. Geometry features must remain tied to source-raster coordinates; crop, rotate or rescale operations may not define the measurement.

Accepted target families include ChArUco, AprilGrid and an equivalently unambiguous indexed checkerboard. Physical spacing and target geometry must be measured and cryptographically identified.

Projection/distortion is not inferred from the lens label. Candidate model families are pinhole/Brown-Conrady, fisheye/Kannala-Brandt and omnidirectional/Mei. MAIN and ULTRA_WIDE need not share a model family. Model selection uses training data only; the selected model and thresholds are frozen before final hold-out scoring.

A final geometry certificate must bind target geometry, training set, holdout set, per-lens intrinsics and distortion, MAIN-to-ULTRA_WIDE extrinsics, held-out residual report and an explicit validity domain. Focus state must be recorded or explicitly UNKNOWN; silent transfer across focus regimes is forbidden.

Even after a future GEOMETRY relation becomes ADMITTED, relation-level fusion remains false, calibration transfer remains false, equality remains false for this axis, and all other relation axes remain independent.

Method anchors:
- OpenCV fisheye/stereo calibration: https://docs.opencv.org/doc/doxygen/html/db/d58/group__calib3d__fisheye.html
- Kalibr multi-camera calibration: https://github.com/ethz-asl/kalibr/wiki/Multiple-camera-calibration
