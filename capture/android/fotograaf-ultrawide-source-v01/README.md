# D.RAW FotoGraaf Ultra-Wide Source Probe v0.1

Status: **DEVICE-CAPTURE SUCCESSOR CANDIDATE**

Purpose: obtain the first real source-bound observation for HONOR BKQ-N49
physical Camera 4 / project ultra-wide 0.6x.

The validated FotoGraaf Acquisition Domain v0.8 tele module remains unchanged.

Static Camera2 capability evidence exposed a candidate logical-Camera-0
multi-resolution mapping `4032×3024 -> physical ID 4`. This probe treats that
only as a candidate. Runtime must independently show that physical Camera 4
advertises exact standard RAW_SENSOR 4032×3024.

A route-proof capture requires exact size, forced physical-4 output binding
when needed, matching physical TotalCaptureResult Camera 4, and image/sensor
timestamp identity. Focal length is recorded but is not an identity gate.

The raw Image.Plane bytes and DNG convenience container are separately hashed.
One capture remains one physical frame / one independent evidence item.

A successful run remains `CAMERA2_ACQUISITION_OBSERVATION_ONLY`. It grants no
calibration, ADC/sensel provenance, graph relation, fusion or Source Admission.
