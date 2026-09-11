#pragma once

#include "scientific_preview_source_binding_v0_1.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace truthraw::dng_color_binding_producer_v0_2 {

enum class ProducerStatusCode : std::uint8_t {
    Ok = 0,
    InvalidArgument,
    SourceSealMismatch,
    SourceReadFailed,
    InvalidTiff,
    UnsupportedBigTiff,
    InvalidIfd,
    MissingColorMatrix,
    MissingAsShotNeutral,
    IncompleteDualCalibration,
    TripleCalibrationUnsupported,
    UnsupportedCalibrationIlluminant,
    InvalidTagType,
    InvalidTagCardinality,
    InvalidValue,
    SingularMatrix,
    NeutralSolveDidNotConverge,
};

struct ProducerStatus {
    ProducerStatusCode code = ProducerStatusCode::Ok;
    std::string message;
    explicit operator bool() const noexcept { return code == ProducerStatusCode::Ok; }
    static ProducerStatus ok() { return {}; }
    static ProducerStatus error(ProducerStatusCode code, std::string message) {
        ProducerStatus out;
        out.code = code;
        out.message = std::move(message);
        return out;
    }
};

struct ProducerAudit {
    std::uint64_t metadataBytesRead = 0;
    std::uint32_t ifdEntriesVisited = 0;
    std::size_t parserWorkspacePeakBytes = 0;
    bool sourceVerifiedBeforeParse = false;
    bool sourceVerifiedAfterParse = false;
    bool delegatedSingleIlluminantV01 = false;
    bool dualIlluminantUsed = false;
    bool thirdCalibrationSeen = false;
    bool usedForwardMatrix = false;
    bool usedSingleForwardMatrixAcrossTemperatures = false;
    bool cameraCalibrationPresent = false;
    bool cameraCalibrationSignatureMatched = false;
    bool cameraCalibrationApplied = false;
    std::uint16_t calibrationIlluminant1 = 0;
    std::uint16_t calibrationIlluminant2 = 0;
    double calibrationTemperatureLowK = 0.0;
    double calibrationTemperatureHighK = 0.0;
    double resolvedWhiteTemperatureK = 0.0;
    double interpolationWeightLow = 0.0;
    double resolvedWhiteX = 0.0;
    double resolvedWhiteY = 0.0;
    std::uint32_t neutralSolveIterations = 0;
};

struct ProducerResult {
    scientific_preview_binding_v0_1::ScientificColorBindingRecord color{};
    ProducerAudit audit{};
};

ProducerStatus produce_source_metadata_color_binding(
    tile_dng_v0_1::IRandomAccessByteSource& source,
    const scientific_preview_binding_v0_1::SourceSeal& sourceSeal,
    ProducerResult& out);

const char* status_name(ProducerStatusCode code) noexcept;

} // namespace truthraw::dng_color_binding_producer_v0_2
