#pragma once

// This product includes DNG technology under license by Adobe.

#include "finalized_scientific_preview_release_v0_2.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "truthraw/core.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace truthraw::linear_dng_projection::v0_1 {

enum class StatusCode : std::uint8_t {
    Ok = 0,
    InvalidArgument,
    FinalizedLineageRequired,
    SourceIdentityMismatch,
    ScientificIdentityMismatch,
    UnauthorizedColorBinding,
    SingularColorMatrix,
    InvalidNeutral,
    SourceFailed,
    SinkFailed,
    OutputTooLarge,
    NonFiniteSample,
};

struct Status final {
    StatusCode code = StatusCode::Ok;
    std::string message;
    explicit operator bool() const noexcept { return code == StatusCode::Ok; }
    static Status ok() { return {}; }
    static Status error(StatusCode code, std::string message) {
        Status out;
        out.code = code;
        out.message = std::move(message);
        return out;
    }
};

enum class ProjectionRole : std::uint8_t {
    LinearDngXyzD50CompatibilityProjection = 1,
};

struct ProjectionAdmission final {
    ProjectionRole role = ProjectionRole::LinearDngXyzD50CompatibilityProjection;
    scientific_preview_binding_v0_1::SourceSeal source{};
    std::array<std::uint8_t, 32> scientificMasterHash{};
    // Exact source-bound transform already validated for the finalized lineage.
    // It is applied exactly once by the Scientific-Master export adapter before
    // the bounded DNG writer sees any samples.
    std::array<float, 9> cameraToXyzD50 = {1.f, 0.f, 0.f,
                                           0.f, 1.f, 0.f,
                                           0.f, 0.f, 1.f};
    scientific_preview_binding_v0_1::ColorBindingAuthority colorAuthority =
        scientific_preview_binding_v0_1::ColorBindingAuthority::Unverified;
    bool strongerPhysicalColorClaim = false;
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
};

// Builds a downstream projection admission from the already-finalized
// Scientific Preview lineage. The writer independently re-checks the admission
// invariants; constructing this struct manually cannot promote authority.
Status admit_linear_dng_projection(
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    const finalized_scientific_preview_release::v0_2::ReleaseResult& release,
    ProjectionAdmission& out) noexcept;

class ILinearRawTileSource {
public:
    virtual ~ILinearRawTileSource() = default;
    virtual int width() const noexcept = 0;
    virtual int height() const noexcept = 0;
    virtual Orientation orientation() const noexcept = 0;

    // Returns 3-channel scene-linear XYZ(D50) compatibility samples for exactly
    // the requested core rectangle. This is an output projection interface,
    // never measured CFA evidence and never the Scientific Master itself.
    virtual Status readLinearRawTile(
        int x0,
        int y0,
        int x1,
        int y1,
        float* xyzD50Out,
        std::size_t floatCount) = 0;
};

class ISequentialByteSink {
public:
    virtual ~ISequentialByteSink() = default;
    virtual Status append(const std::uint8_t* data, std::size_t size) = 0;
    virtual std::uint64_t bytesWritten() const noexcept = 0;
};

// Required by the finalized Scientific-Master-bound export path. Bytes may be
// staged while the export digest is being recomputed, but they become a valid
// output only after commit(). Any pre-commit failure calls abort(), which must
// leave no usable DNG payload behind (an empty destination is acceptable).
class ITransactionalByteSink : public ISequentialByteSink {
public:
    ~ITransactionalByteSink() override = default;
    virtual Status commit() = 0;
    virtual void abort() noexcept = 0;
};

struct Options final {
    int tileEdge = 64;
    // Explicit output-scale only. There is no auto exposure, tone curve or
    // appearance operation in this writer.
    double linearScale = 1.0;
    // The compatibility DNG is a bounded 16-bit projection. Values below 0 and
    // above 1 are clamped only when this explicit projection flag is enabled;
    // every clamp is counted in Audit. The Scientific Master remains untouched.
    bool clampToLinearReferenceRange = true;
    std::string uniqueCameraModel = "TruthRaw XYZ D50 Linear Projection";
    std::string software = "TruthRaw Linear DNG Projection v0.1";
};

struct Audit final {
    ProjectionRole role = ProjectionRole::LinearDngXyzD50CompatibilityProjection;
    std::uint64_t linearSamplesRead = 0;
    std::uint64_t negativeSamplesClamped = 0;
    std::uint64_t overrangeSamplesClamped = 0;
    std::uint64_t quantizedSamples = 0;
    std::uint64_t tilesWritten = 0;
    std::uint64_t outputBytes = 0;
    std::size_t logicalWorkspacePeakBytes = 0;

    bool boundedCompatibilityProjection = true;
    bool scientificMasterModified = false;
    bool createsEvidence = false;
    bool fullFrameMaterialized = false;
    bool outputEncodingXyzD50 = true;
    bool sourceMetadataColorOnly = true;
    bool strongerPhysicalColorClaim = false;
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
};

// Writes a classic little-endian TIFF/DNG raw IFD using:
// - PhotometricInterpretation = LinearRaw (34892)
// - 3 interleaved XYZ(D50) scene-linear compatibility channels
// - 16-bit unsigned integer bounded samples
// - uncompressed 64x64-style tiles by default
// - identity ColorMatrix1 (XYZ -> declared projection space)
// - CalibrationIlluminant1 = D50 and AsShotNeutral = D50 XYZ.
//
// The output is a compatibility projection. It is never the Scientific Master
// and never measured CFA evidence.
Status write_linear_dng(
    ILinearRawTileSource& source,
    ISequentialByteSink& sink,
    const ProjectionAdmission& admission,
    const Options& options,
    Audit& out) noexcept;

const char* status_name(StatusCode code) noexcept;

}  // namespace truthraw::linear_dng_projection::v0_1
