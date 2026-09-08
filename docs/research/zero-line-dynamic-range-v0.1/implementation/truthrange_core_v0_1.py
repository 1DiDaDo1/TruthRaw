from dataclasses import dataclass
from math import inf, isfinite, log, log2, sqrt
from typing import Optional

LN2 = log(2.0)

MEASURED_UNCENSORED = "MEASURED_UNCENSORED"
MEASURED_CENSORED_LOWER_BOUND = "MEASURED_CENSORED_LOWER_BOUND"
MEASURED_DARK_UPPER_BOUND = "MEASURED_DARK_UPPER_BOUND"
RECONSTRUCTED_STRONG = "RECONSTRUCTED_STRONG"
RECONSTRUCTED_WEAK = "RECONSTRUCTED_WEAK"
UNKNOWN = "UNKNOWN"
APPEARANCE_ONLY = "APPEARANCE_ONLY"

@dataclass(frozen=True)
class TruthRangeGauge:
    gauge_id: str
    L0: float
    unit: str
    absolute_binding: Optional[str] = None

    def __post_init__(self):
        if not (self.L0 > 0.0 and isfinite(self.L0)):
            raise ValueError("L0 must be finite and > 0")

@dataclass(frozen=True)
class TruthRangeSample:
    linear_signed: float
    estimate_ev: Optional[float]
    lower_ev: float
    upper_ev: float
    sigma_ev: Optional[float]
    support_class: str
    censor_state: str
    gauge_id: str

def truthrange_ev(L: float, gauge: TruthRangeGauge) -> float:
    if L <= 0.0:
        return -inf
    return log2(L / gauge.L0)

def inverse_truthrange(ev: float, gauge: TruthRangeGauge) -> float:
    if ev == -inf:
        return 0.0
    if ev == inf:
        return inf
    return gauge.L0 * (2.0 ** ev)

def scale_invariant_ev(L: float, L0: float, c: float) -> tuple[float, float]:
    if L <= 0 or L0 <= 0 or c <= 0:
        raise ValueError("L, L0 and c must be > 0")
    return log2(L / L0), log2((c * L) / (c * L0))

def from_sensor_signal(
    *,
    signal_normalized: float,
    exposure_gain_coordinate: float,
    gauge: TruthRangeGauge,
    noise_S: float,
    noise_O: float,
    clipped: bool = False,
    k_dark: float = 3.0,
    interval_sigma: float = 1.96,
) -> TruthRangeSample:
    """
    Convert a positive-light sensor-domain evidence sample to TruthRange.

    signal_normalized:
        Black-subtracted, source-white-normalized sensor signal.
        It may be negative as an estimator; negative numerical values are not
        interpreted as negative physical light.

    exposure_gain_coordinate:
        Positive coordinate that maps source response back toward a common
        scene-light gauge. In the v0.1 BnCam experiment this is ISO * exposure_s.
        That is a provisional empirical coordinate, not physical radiometric calibration.

    NoiseProfile:
        Var(signal_normalized) = S * max(signal_normalized,0) + O

    Clipping:
        A clipped sample is a lower-bound measurement. upper_ev is +inf.
    """
    if exposure_gain_coordinate <= 0 or not isfinite(exposure_gain_coordinate):
        raise ValueError("exposure_gain_coordinate must be finite and > 0")
    if noise_S < 0 or noise_O < 0:
        raise ValueError("noise coefficients must be >= 0")

    s = float(signal_normalized)
    positive_s = max(s, 0.0)
    L_est = positive_s / exposure_gain_coordinate
    sigma_s = sqrt(max(noise_S * positive_s + noise_O, 0.0))
    sigma_L = sigma_s / exposure_gain_coordinate

    if clipped:
        # Source white corresponds to signal_normalized >= 1 in this contract.
        lower_L = max(1.0, positive_s) / exposure_gain_coordinate
        return TruthRangeSample(
            linear_signed=s / exposure_gain_coordinate,
            estimate_ev=truthrange_ev(L_est, gauge) if L_est > 0 else None,
            lower_ev=truthrange_ev(lower_L, gauge),
            upper_ev=inf,
            sigma_ev=None,
            support_class=MEASURED_CENSORED_LOWER_BOUND,
            censor_state="HIGH_CLIPPED",
            gauge_id=gauge.gauge_id,
        )

    dark_upper_signal = k_dark * sqrt(noise_O)
    if s <= dark_upper_signal:
        dark_upper_L = max(dark_upper_signal, 0.0) / exposure_gain_coordinate
        return TruthRangeSample(
            linear_signed=s / exposure_gain_coordinate,
            estimate_ev=truthrange_ev(L_est, gauge) if L_est > 0 else None,
            lower_ev=-inf,
            upper_ev=truthrange_ev(dark_upper_L, gauge) if dark_upper_L > 0 else -inf,
            sigma_ev=None,
            support_class=MEASURED_DARK_UPPER_BOUND,
            censor_state="DARK_NOISE_LIMITED",
            gauge_id=gauge.gauge_id,
        )

    lo_signal = s - interval_sigma * sigma_s
    hi_signal = s + interval_sigma * sigma_s
    lo_L = max(lo_signal, 0.0) / exposure_gain_coordinate
    hi_L = max(hi_signal, 0.0) / exposure_gain_coordinate
    estimate = truthrange_ev(L_est, gauge)
    lower = truthrange_ev(lo_L, gauge) if lo_L > 0 else -inf
    upper = truthrange_ev(hi_L, gauge) if hi_L > 0 else -inf
    sigma_ev = sigma_s / (LN2 * s) if s > 0 and sigma_s > 0 else 0.0

    return TruthRangeSample(
        linear_signed=s / exposure_gain_coordinate,
        estimate_ev=estimate,
        lower_ev=lower,
        upper_ev=upper,
        sigma_ev=sigma_ev,
        support_class=MEASURED_UNCENSORED,
        censor_state="NONE",
        gauge_id=gauge.gauge_id,
    )
