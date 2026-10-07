# D.RAW High-Fidelity JPEG v0.1

The JPEG output path is downstream presentation only. It consumes the exact full-resolution RGB24 sibling emitted before NV21/JPEG chroma reduction and encodes it at requested quality 100.

Acceptance is fail-closed. The resulting JPEG itself must prove:

- baseline SOF0;
- exact frozen output dimensions;
- three components with sampling factors 1x1 / 1x1 / 1x1 (4:4:4);
- 8-bit quantization tables 0 and 1 containing only value 1 for this admitted Q100 encoder.

No encoder request, UI label or visual appearance is sufficient proof. A candidate failing any check is deleted and cannot be committed.

This output path cannot create evidence, mutate sealed source data, modify Scientific Master or authorize scientific writeback.
