#define truthraw truthraw_v47j_reference
#include "../../../../canonical/detail/v4.7j/native/src/core.cpp"
#undef truthraw

extern "C" int truthraw_v47j_reference_apply(
    const float* input,
    int width,
    int height,
    int coreX0,
    int coreY0,
    int coreW,
    int coreH,
    float noiseSigmaAt2Pct,
    float* output) {
    truthraw_v47j_reference::AdaptiveDetailedCrispAppearance appearance;
    truthraw_v47j_reference::AppearanceContext context{};
    context.noiseSigmaAt2Pct = noiseSigmaAt2Pct;
    const auto status = appearance.applyTile(
        input,
        width,
        height,
        coreX0,
        coreY0,
        coreW,
        coreH,
        context,
        output);
    return status ? 0 : static_cast<int>(status.code) + 1;
}
