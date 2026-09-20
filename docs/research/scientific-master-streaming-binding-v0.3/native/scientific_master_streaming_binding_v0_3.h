#pragma once

#include "../scientific-master-streaming-binding-v0.2/native/scientific_master_streaming_binding_v0_2.h"

namespace truthraw::scientific_master_streaming_binding::v0_3 {

using Hash256 = v0_2::Hash256;
using StatusCode = v0_2::StatusCode;
using Status = v0_2::Status;
using Options = v0_2::Options;
using Result = v0_2::Result;

inline constexpr double kSelfGaugeQuantile = v0_2::kSelfGaugeQuantile;
inline constexpr double kSelfGaugeBorderFraction = v0_2::kSelfGaugeBorderFraction;
inline constexpr int kCanonicalCore = v0_2::kCanonicalCore;
inline constexpr std::size_t kRadix16BucketCount = v0_2::kRadix16BucketCount;
inline constexpr std::size_t kRadix16HistogramBytes = v0_2::kRadix16HistogramBytes;
inline constexpr std::size_t kMaximumRadixAuxiliaryBytes = v0_2::kMaximumRadixAuxiliaryBytes;

// Runtime-only stripe policy. Both widths/heights are multiples of the
// canonical 64x64 digest cell except at the image edge. These constants may
// reduce execution overhead but are forbidden from changing scientific identity.
inline constexpr int kDigestStripeCoreWidth = 1024;
inline constexpr int kDigestStripeCoreHeight = 64;
inline constexpr int kGaugeStripeCoreWidth = 1024;
inline constexpr int kGaugeStripeCoreHeight = 256;

Status bind_scientific_master_streaming(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const Options& options,
    Result& out) noexcept;

inline const char* status_name(StatusCode code) noexcept {
    return v0_2::status_name(code);
}

}  // namespace truthraw::scientific_master_streaming_binding::v0_3
