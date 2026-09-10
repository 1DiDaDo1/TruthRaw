# CICM v1 fact check — 2026-09-10

## Findings implemented

- The light transport equation depends on incident illumination, BSDF/material scattering and geometry/global visibility. Uniform radiance scaling is therefore not equivalent to a full sun/moon relight.
- Single-image inverse rendering/relighting is ill-posed: geometry, material and illumination can be entangled. Inferred/generative relighting may be plausible but cannot be relabeled measured TruthRaw evidence.
- EMVA 1288 Release 4.0 Linear explicitly characterizes quantities including gain in DN/e-, dark noise and saturation capacity. TruthRaw currently lacks an independently measured, exact-bound HONOR BKQ-N49 BnCam tele electron/photon-transfer calibration, so physical camera predictions remain blocked for that real mode.
- CIE D65 represents average daylight for colorimetry, but real daylight varies with time/location/season; sky luminance also depends on weather and sun position. NASA describes moonlight as reflected sunlight. Therefore v1 has no hard-coded 'sun' or 'night' physical scalar preset.

## TruthRaw-specific correction

`canonical/ptc/v1.1` means **Pure Truth Certificate**. It is a fail-closed certificate engine. It is not itself the photon-transfer/electron calibration; its own FULL_PHYSICAL gap explicitly still requires electron/PTC calibration bound to sensor/lens mode.

## Claim boundary

CICM can now simulate relative worlds and can execute a calibrated temporal expected-value sensor model when calibration is supplied. It cannot yet claim physically correct Honor tele sun/night relighting, spectral color changes, new cast shadows, BRDF/specular changes, atmospheric scattering, motion blur, diffraction/DOF changes, spatial sensor non-uniformity, or independently calibrated ISO-dependent noise.

## External references

- EMVA 1288 Release 4.0 Linear: https://www.emva.org/wp-content/uploads/EMVA1288Linear_4.0Release.pdf
- PBRT 4e, Light Transport Equation: https://www.pbr-book.org/4ed/Light_Transport_I_Surface_Reflection/The_Light_Transport_Equation
- CIE standard illuminants: https://www.cie.co.at/publications/colorimetry-part-2-cie-standard-illuminants-0
- CIE General Sky: https://www.cie.co.at/publications/spatial-distribution-daylight-cie-standard-general-sky-0
- NASA Moonlight: https://science.nasa.gov/moon/moonlight/
- CVPR 2025 inverse-rendering literature: https://openaccess.thecvf.com/content/CVPR2025/html/Choi_Channel-wise_Noise_Scheduled_Diffusion_for_Inverse_Rendering_in_Indoor_Scenes_CVPR_2025_paper.html
