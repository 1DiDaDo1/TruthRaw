# Android runtime policy — Room Capsule v0.1

The native core does not depend on Android APIs. The app adapter supplies only runtime capability values:

- `ActivityManager.getMemoryClass()` -> `appMemoryClassMiB`;
- `ActivityManager.isLowRamDevice()` -> `lowRamDevice`;
- an optional stricter app working-set ceiling -> `explicitMaxWorkingSetBytes`;
- GPU availability/capability -> advisory `gpuAvailable` only.

Rules:

1. CPU/C++ remains the correctness baseline on every supported device.
2. Vulkan/GPU code may accelerate a tile but may not change scientific or appearance semantics.
3. Room caches are reproducible and disposable; on memory pressure the Android layer should drop them before sealed/scientific state.
4. The native planner never allocates above an explicit ceiling; if too little memory is available it returns a failure plan rather than overcommitting.
5. The scheduler visits only tiles intersecting the room bounding domain and clips further against the vector room boundary.
6. Full-resolution source/master buffers are not duplicated into the Room Capsule.
