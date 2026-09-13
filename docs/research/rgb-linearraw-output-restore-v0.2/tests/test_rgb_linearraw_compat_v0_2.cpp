#include "rgb_linearraw_compat_v0_2.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(2);
    }
}

bool near(double a, double b, double tolerance) {
    return std::abs(a - b) <= tolerance;
}

}  // namespace

int main() {
    using namespace truthraw::rgb_linearraw_restore::v0_2;

    CompatibilityWindow w{};
    require(choose_compatibility_window(0.95, w) == WindowStatus::Ok, "0.95 must be representable");
    require(w.scale == 1.0, "0.95 must select 1x window");
    require(w.baselineExposureEv == 0.0, "1x BaselineExposure must be 0 EV");

    require(choose_compatibility_window(1.013005137, w) == WindowStatus::Ok, "historical 1.013 scene max must be representable");
    require(w.scale == 1.25, "1.013 scene max must select historical 1.25x window");
    require(near(w.baselineExposureEv, 0.32192809488736235, 1.0e-12), "1.25x BaselineExposure mismatch");

    require(choose_compatibility_window(1.730840802, w) == WindowStatus::Ok, "historical 1.73084 scene max must be representable");
    require(w.scale == 2.0, "1.73084 scene max must select historical 2x window");
    require(w.baselineExposureEv == 1.0, "2x BaselineExposure must be +1 EV");

    require(choose_compatibility_window(2.0000001, w) == WindowStatus::SceneExceedsValidatedWindow,
            "unvalidated >2x scene range must fail closed");

    CompatibilityWindow h{};
    require(choose_compatibility_window(1.730840802, h) == WindowStatus::Ok, "headroom setup failed");

    bool low = false;
    bool high = false;
    bool valid = false;

    const auto qOverOne = encode_u16(1.5, h, low, high, valid);
    require(valid, "1.5 encode must be valid");
    require(!low && !high, "1.5 must not clip inside 2x window");
    require(qOverOne < 65535u, "1.5 must preserve positive headroom below white code");
    require(near(decode_u16(qOverOne, h), 1.5, h.scale / 65535.0), "1.5 roundtrip exceeds one-code tolerance");

    const auto qNegative = encode_u16(-0.01, h, low, high, valid);
    require(valid && low && !high && qNegative == 0u, "negative finite value must clip low only");

    const auto qHigh = encode_u16(2.2, h, low, high, valid);
    require(valid && !low && high && qHigh == 65535u, "value above finite window must clip high only");

    const auto qNan = encode_u16(std::nan(""), h, low, high, valid);
    require(!valid && qNan == 0u, "non-finite value must fail encoding");

    std::cout << "RGB_LINEARRAW_COMPAT_V0_2_PASS\n";
    std::cout << "historical_window_1_25=1\n";
    std::cout << "historical_window_2_0=1\n";
    std::cout << "scene_over_one_preserved=1\n";
    std::cout << "unvalidated_over_2_fails_closed=1\n";
    return 0;
}
