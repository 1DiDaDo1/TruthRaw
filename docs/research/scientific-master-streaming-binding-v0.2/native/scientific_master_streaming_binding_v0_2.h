#pragma once

#include "scientific_master_streaming_binding_v0_1.h"

namespace truthraw::scientific_master_streaming_binding::v0_2 {

// v0.2 changes only the exact-median selection schedule. Scientific types,
// authority, digest format, gauge semantics and error vocabulary remain the
// validated v0.1 contract.
using Hash256 = v0_1::Hash256;
using StatusCode = v0_1::StatusCode;
using Status = v0_1::Status;
using Options = v0_1::Options;
using Result = v0_1::Result;

inline constexpr double kSelfGaugeQuantile = v0_1::kSelfGaugeQuantile;
inline constexpr double kSelfGaugeBorderFraction = v0_1::kSelfGaugeBorderFraction;
inline constexpr int kCanonicalCore = v0_1::kCanonicalCore;
inline constexpr std::size_t kRadix16BucketCount = 65536u;
inline constexpr std::size_t kRadix16HistogramBytes =
    kRadix16BucketCount * sizeof(std::uint64_t);
inline constexpr std::size_t kMaximumRadixAuxiliaryBytes =
    2u * kRadix16HistogramBytes;

// Builds exactly the same Scientific Master identity and TruthRange v0.2
// self-gauge as v0.1, but resolves the positive-float median in two 16-bit
// radix scans rather than four 8-bit scans. No source, reconstruction,
// zero-line, scene-scale, evidence or appearance semantics are changed.
Status bind_scientific_master_streaming(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const Options& options,
    Result& out) noexcept;

inline const char* status_name(StatusCode code) noexcept {
    return v0_1::status_name(code);
}

}  // namespace truthraw::scientific_master_streaming_binding::v0_2
