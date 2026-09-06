#pragma once
#include <array>
namespace truthraw {
struct UncertaintyBands { float p50, p95; };
UncertaintyBands predict_uncertainty_v5_0g(const std::array<float,18>& x,int role,float snr);
}
