import math
from truthrange_core_v0_1 import (
    TruthRangeGauge,
    truthrange_ev,
    inverse_truthrange,
    scale_invariant_ev,
    from_sensor_signal,
    MEASURED_UNCENSORED,
    MEASURED_CENSORED_LOWER_BOUND,
    MEASURED_DARK_UPPER_BOUND,
)

g = TruthRangeGauge("TEST", 2.0**-10, "relative_light")

# Exact zero line.
assert abs(truthrange_ev(g.L0, g)) < 1e-15

# Infinite dark side and invertibility for finite samples.
assert truthrange_ev(0.0, g) == -math.inf
for ev in [-20.0, -3.25, 0.0, 4.5, 40.0]:
    L = inverse_truthrange(ev, g)
    assert abs(truthrange_ev(L, g) - ev) < 1e-12

# Gauge-scale invariance.
a, b = scale_invariant_ev(0.0037, 0.0011, 123456.0)
assert abs(a - b) < 1e-12

# Uncensored measured sample.
s = from_sensor_signal(
    signal_normalized=0.2,
    exposure_gain_coordinate=40.0,
    gauge=g,
    noise_S=3e-4,
    noise_O=2e-7,
)
assert s.support_class == MEASURED_UNCENSORED
assert math.isfinite(s.estimate_ev)
assert s.lower_ev < s.estimate_ev < s.upper_ev

# Highlight clip becomes finite lower bound + infinite upper bound.
c = from_sensor_signal(
    signal_normalized=1.0,
    exposure_gain_coordinate=40.0,
    gauge=g,
    noise_S=3e-4,
    noise_O=2e-7,
    clipped=True,
)
assert c.support_class == MEASURED_CENSORED_LOWER_BOUND
assert math.isfinite(c.lower_ev)
assert c.upper_ev == math.inf

# Noise-limited darkness gets -inf lower tail.
d = from_sensor_signal(
    signal_normalized=0.0,
    exposure_gain_coordinate=40.0,
    gauge=g,
    noise_S=3e-4,
    noise_O=2e-7,
)
assert d.support_class == MEASURED_DARK_UPPER_BOUND
assert d.lower_ev == -math.inf
assert math.isfinite(d.upper_ev)

# ISO-neutrality algebra: equal signal/(ISO*t) -> equal TruthRange.
x = from_sensor_signal(
    signal_normalized=0.04,
    exposure_gain_coordinate=40.0,
    gauge=g,
    noise_S=3e-4,
    noise_O=2e-7,
)
y = from_sensor_signal(
    signal_normalized=0.08,
    exposure_gain_coordinate=80.0,
    gauge=g,
    noise_S=3e-4,
    noise_O=2e-7,
)
assert abs(x.estimate_ev - y.estimate_ev) < 1e-12

print("TruthRange core v0.1: PASS")
