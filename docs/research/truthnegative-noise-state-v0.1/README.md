# TruthNegative Noise State v0.1

Executable N1 measurement-state primitive. It binds a three-channel shot/read variance model to one finalized TruthNegative identity without changing any scene value.

Authority is explicit: DNG/metadata NoiseProfile is not promoted to independent physical calibration. A separately calibrated model has a distinct authority.

CENSORED samples intentionally receive no Gaussian exact-value uncertainty. Their censor/lower-bound state remains authoritative.

The state is single-frame and creates no evidence or scientific writeback. A later adapter may populate this state from the already existing TruthRange Dense Uncertainty / DNG NoiseProfile path after scale/domain identity is proven exact.
