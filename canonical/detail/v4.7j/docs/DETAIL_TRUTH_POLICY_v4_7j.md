# Detail Truth Policy v4.7j

1. Scientific masters are immutable. Adaptive Detail is appearance-side only.
2. No semantic skin detector is used. Skin safety emerges from general edge/noise/activity guards.
3. NoiseProfile controls confidence; higher-noise captures receive less detail amplification.
4. Hard-edge acutance and fine-texture compensation are not the same operation. Hard steps are protected.
5. RGB channels are scaled by one luminance factor; sharpening must not deliberately change hue/chroma.
6. Local support clipping limits ringing.
7. A detailed profile may compensate known algorithmic attenuation but may not invent unsupported texture.
8. Output acutance after resizing/export is a separate stage and requires its own validation.
