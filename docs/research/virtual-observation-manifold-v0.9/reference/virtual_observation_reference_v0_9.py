"""Independent stdlib-only numerical reference for TruthRaw Virtual Observation Manifold v0.9."""
from __future__ import annotations
import math

DEFAULT_EV_NODES = (-6, -4, -2, 0, 2, 4, 6, 8, 10)


def exposure_scale(ev: float) -> float:
    return 2.0 ** float(ev)


def project_truthrange_ev(t: float, exposure_ev: float) -> float:
    return float(t) + float(exposure_ev)


def project_signed_linear(values, exposure_ev: float, gain_ev: float = 0.0):
    s = exposure_scale(exposure_ev + gain_ev)
    return [float(x) * s for x in values]


def project_covariance(cov, exposure_ev: float, gain_ev: float = 0.0):
    s2 = exposure_scale(exposure_ev + gain_ev) ** 2
    return [[float(x) * s2 for x in row] for row in cov]


def normalized_ev_weights(signal: float, ev_nodes=DEFAULT_EV_NODES,
                          target: float = 0.18, width_stops: float = 2.0):
    if not (math.isfinite(signal) and signal > 0.0):
        raise ValueError("signal must be positive and finite")
    if not (math.isfinite(target) and target > 0.0):
        raise ValueError("target must be positive and finite")
    if not (math.isfinite(width_stops) and width_stops > 0.0):
        raise ValueError("width_stops must be positive and finite")
    raw = []
    for ev in ev_nodes:
        d = abs(math.log2(signal * exposure_scale(ev) / target))
        raw.append(math.exp(-0.5 * (d / width_stops) ** 2))
    total = math.fsum(raw)
    if not (math.isfinite(total) and total > 0.0):
        raise ValueError("normalization sum invalid")
    return [x / total for x in raw]


def self_test():
    assert exposure_scale(4) == 16.0
    assert project_truthrange_ev(-3.0, 4.0) == 1.0
    x = [-0.25, 0.0, 0.5]
    assert project_signed_linear(x, 2.0) == [-1.0, 0.0, 2.0]

    c = [[1.0, 0.2, 0.1], [0.2, 4.0, 0.3], [0.1, 0.3, 9.0]]
    got = project_covariance(c, 4.0)
    expected = [[v * 256.0 for v in row] for row in c]
    assert got == expected

    w = normalized_ev_weights(0.01)
    assert abs(math.fsum(w) - 1.0) < 1e-15
    assert len(w) == len(DEFAULT_EV_NODES)

    # Gain encoding must not change TruthRange scene coordinates.
    assert project_truthrange_ev(2.25, 0.0) == 2.25
    # View count is deliberately absent from every evidence equation above.
    print("TRUTHRAW_VIRTUAL_OBSERVATION_REFERENCE_V0_9 PASS")


if __name__ == "__main__":
    self_test()
