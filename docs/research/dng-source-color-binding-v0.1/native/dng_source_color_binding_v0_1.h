#pragma once

#include "scientific_preview_source_binding_v0_1.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace truthraw::dng_source_color_v0_1 {

enum class StatusCode : std::uint8_t {
    Ok = 0,
    InvalidArgument,
    SourceSealRejected,
    SourceChanged,
    InvalidTiff,
    BigTiffUnsupported,
    RootIfdOutOfBounds,
    TooManyIfdEntries,
    DuplicateColorTag,
    MissingDngVersion,
    UnsupportedDngVersion,
    UnsupportedColorimetricReference,
    UnsupportedProfileTopology,
    MissingColorMatrix1,
    MissingForwardMatrix1,
    MissingWhitePoint,
    MissingCalibrationIlluminant1,
    UnsupportedTagType,
    InvalidTagCount,
    TagPayloadOutOfBounds,
    InvalidRational,
    InvalidCalibrationSignature,
    InvalidColorMatrix,
    InvalidForwardMatrix,
    InvalidWhitePoint,
    SingularMatrix,
    NonFiniteResult,
};

struct Status {
    StatusCode code = StatusCode::Ok;
    std::string message;
    explicit operator bool() const noexcept { return code == StatusCode::Ok; }
    static Status ok() { return {}; }
    static Status error(StatusCode code, std::string message) {
        Status out; out.code = code; out.message = std::move(message); return out;
    }
};

struct Metrics {
    std::uint32_t rootIfdEntries = 0;
    std::size_t metadataBytesRead = 0;
    std::size_t temporaryBytesUpperBound = 0;
    bool littleEndian = true;
    bool usedAsShotNeutral = false;
    bool usedAsShotWhiteXY = false;
    bool usedAnalogBalance = false;
    bool usedCameraCalibration1 = false;
    std::uint16_t calibrationIlluminant1 = 0;
};

struct Result {
    scientific_preview_binding_v0_1::ScientificColorBindingRecord binding{};
    Metrics metrics{};
};

// Strict v0.1 producer for the embedded, single-illuminant DNG profile in IFD0.
// It deliberately rejects dual/triple-illuminant profiles, alternate camera profiles,
// output-referred ColorimetricReference, and any CameraCalibration1 whose signature
// compatibility cannot be proved from the file.
Status produce_source_bound_color_binding(
    tile_dng_v0_1::IRandomAccessByteSource& source,
    const scientific_preview_binding_v0_1::SourceSeal& sourceSeal,
    Result& out);

const char* status_name(StatusCode code) noexcept;

} // namespace truthraw::dng_source_color_v0_1
