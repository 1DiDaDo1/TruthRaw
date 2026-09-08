# GPU / Performance note v4.7j

Adaptive Detail uses 3×3, 5×5 and 11×11 local statistics plus edge/noise guards. The final CPU reference appearance pass costs ~346.5 ms at 12.53 MP on this host with 8 threads.

Recommended Vulkan decomposition:
1. luminance + local-statistics pass;
2. micro/fine/texture + hard-edge confidence pass;
3. RGB scale/apply pass.

Keep FP32 for the scientific reconstruction input and reference parity. FP16/mixed precision may only be enabled after measured CPU↔GPU tolerance validation. No SPIR-V validation or Adreno device benchmark is claimed in v4.7j.
