#include "truthnegative_center_excluded_spatial_audit_v0_2_1.h"
#include "truthnegative_center_excluded_spatial_audit_v0_2_2.h"
#include "truthnegative_n2_cfa_audit_v0_1.h"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace oldce =
    truthraw::truthnegative_center_excluded_spatial_audit::v0_2_1;
namespace newce =
    truthraw::truthnegative_center_excluded_spatial_audit::v0_2_2;
namespace a = truthraw::truthnegative_n2_cfa_audit::v0_1;
namespace st = truthraw::streaming_v0_1;

#define R(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)

class FakeSource final : public st::IRawTileSource {
public:
    FakeSource() {
        md.width = 64;
        md.height = 64;
        md.cfa = truthraw::CfaPattern::BGGR;
        md.whiteLevel = 1023;
        md.blackPhase = {64,64,64,64};
        md.hasNoiseProfile = true;
        md.noiseProfile = {
            0.004f,0.00002f,
            0.004f,0.00002f,
            0.004f,0.00002f};
        raw.resize(64u * 64u, 300u);
        for (int y = 0; y < 64; ++y) {
            for (int x = 0; x < 64; ++x) {
                int v = 300 + ((x + y) % 3) - 1;
                if (x >= 32 && y >= 8 && y < 56) {
                    v += ((y / 4) % 2) == 0 ? 20 : -20;
                }
                raw[static_cast<std::size_t>(y) * 64u +
                    static_cast<std::size_t>(x)] =
                    static_cast<std::uint16_t>(v);
            }
        }
        for (int y = 24; y < 32; ++y) {
            for (int x = 40; x < 48; ++x) {
                raw[static_cast<std::size_t>(y) * 64u +
                    static_cast<std::size_t>(x)] = 1023u;
            }
        }
    }

    const truthraw::DngMetadata& metadata() const override { return md; }
    std::size_t residentBytesUpperBound() const override {
        return raw.size() * sizeof(std::uint16_t);
    }

    st::StreamStatus readRawTile(
        const truthraw::TileRect& t,
        std::uint16_t* out,
        std::size_t n,
        float*,
        std::size_t) override {
        const int w = t.hx1 - t.hx0;
        const int h = t.hy1 - t.hy0;
        if (w <= 0 || h <= 0 ||
            n != static_cast<std::size_t>(w) *
                static_cast<std::size_t>(h)) {
            return st::StreamStatus::error(
                st::StreamStatusCode::InvalidArgument, "size");
        }
        for (int yy = 0; yy < h; ++yy) {
            for (int xx = 0; xx < w; ++xx) {
                out[static_cast<std::size_t>(yy) *
                        static_cast<std::size_t>(w) +
                    static_cast<std::size_t>(xx)] =
                    raw[static_cast<std::size_t>(t.hy0 + yy) * 64u +
                        static_cast<std::size_t>(t.hx0 + xx)];
            }
        }
        return st::StreamStatus::ok();
    }

    st::StreamStatus readRowBias(
        int, int, float*, std::size_t) override {
        return st::StreamStatus::ok();
    }
    st::StreamStatus readColBias(
        int, int, float*, std::size_t) override {
        return st::StreamStatus::ok();
    }

    truthraw::DngMetadata md{};
    std::vector<std::uint16_t> raw{};
};

template <typename A, typename B>
bool same_metrics(const A& a0, const B& b0) {
    return
        a0.sampled == b0.sampled &&
        a0.v01CandidateCenters == b0.v01CandidateCenters &&
        a0.predictorValid == b0.predictorValid &&
        a0.predictorInvalid == b0.predictorInvalid &&
        a0.symmetricPairsConsidered == b0.symmetricPairsConsidered &&
        a0.symmetricPairsAccepted == b0.symmetricPairsAccepted &&
        a0.symmetricPairsRejected == b0.symmetricPairsRejected &&
        a0.scalesConsidered == b0.scalesConsidered &&
        a0.scalesAccepted == b0.scalesAccepted &&
        a0.scalesRejected == b0.scalesRejected &&
        a0.centerResidualWithin1Sigma == b0.centerResidualWithin1Sigma &&
        a0.centerResidualBetween1And2Sigma == b0.centerResidualBetween1And2Sigma &&
        a0.centerResidualAbove2Sigma == b0.centerResidualAbove2Sigma &&
        a0.combinedResidualWithin1Sigma == b0.combinedResidualWithin1Sigma &&
        a0.combinedResidualBetween1And2Sigma == b0.combinedResidualBetween1And2Sigma &&
        a0.combinedResidualAbove2Sigma == b0.combinedResidualAbove2Sigma &&
        a0.absResidualSum == b0.absResidualSum &&
        a0.maxAbsResidual == b0.maxAbsResidual &&
        a0.centerVarianceSum == b0.centerVarianceSum &&
        a0.estimateVarianceSum == b0.estimateVarianceSum &&
        a0.estimateToCenterVarianceRatioSum ==
            b0.estimateToCenterVarianceRatioSum &&
        a0.maxEstimateToCenterVarianceRatio ==
            b0.maxEstimateToCenterVarianceRatio &&
        a0.maxDirectionalDisagreementSigma ==
            b0.maxDirectionalDisagreementSigma &&
        a0.maxCrossScaleDisagreementSigma ==
            b0.maxCrossScaleDisagreementSigma &&
        a0.v01CandidateCfaPhase == b0.v01CandidateCfaPhase &&
        a0.predictorValidCfaPhase == b0.predictorValidCfaPhase;
}

int main() {
    FakeSource source;

    a::Binding v01Binding{};
    v01Binding.sourceEvidenceSha256[0] = 1u;
    v01Binding.truthNegativeStateSha256[0] = 2u;

    a::Options options{};
    options.tileEdge = 32u;
    options.samplingPeriod = 8u;

    a::Result reference{};
    R(a::run(source, v01Binding, options, reference));
    R(reference.audit.corrected > 0u);
    R(reference.correctedSampleCoordinates.size() ==
      reference.audit.corrected);
    std::uint64_t sparseTotal = 0u;
    for (const auto& tile : reference.tiles) {
        R(tile.correctedSampleCount == tile.audit.corrected);
        R(tile.correctedSampleOffset + tile.correctedSampleCount <=
          reference.correctedSampleCoordinates.size());
        sparseTotal += tile.correctedSampleCount;
    }
    R(sparseTotal == reference.audit.corrected);

    oldce::Binding ob{};
    ob.sourceEvidenceSha256 = v01Binding.sourceEvidenceSha256;
    ob.scientificMasterSha256[0] = 3u;
    ob.authorityFieldSha256[0] = 4u;
    ob.truthNegativeStateSha256 = v01Binding.truthNegativeStateSha256;
    ob.v01CandidateSha256 = reference.candidateSha256;
    ob.v01AuditSha256 = reference.auditSha256;
    ob.v01SpatialSha256 = reference.spatialSha256;

    newce::Binding nb{};
    nb.sourceEvidenceSha256 = ob.sourceEvidenceSha256;
    nb.scientificMasterSha256 = ob.scientificMasterSha256;
    nb.authorityFieldSha256 = ob.authorityFieldSha256;
    nb.truthNegativeStateSha256 = ob.truthNegativeStateSha256;
    nb.v01CandidateSha256 = ob.v01CandidateSha256;
    nb.v01AuditSha256 = ob.v01AuditSha256;
    nb.v01SpatialSha256 = ob.v01SpatialSha256;

    oldce::Result oldResult{};
    newce::Result newResult{};
    R(oldce::run(source, ob, reference, oldResult));
    R(newce::run(source, nb, reference, newResult));

    R(oldResult.v01TileParityVerified);
    R(newResult.v01TileParityVerified);
    R(newResult.v01SparseReferenceReuseVerified);
    R(!newResult.v01RerunPerformed);
    R(same_metrics(oldResult.metrics, newResult.metrics));
    R(oldResult.tiles.size() == newResult.tiles.size());

    for (std::size_t i = 0; i < oldResult.tiles.size(); ++i) {
        const auto& x = oldResult.tiles[i];
        const auto& y = newResult.tiles[i];
        R(x.x == y.x);
        R(x.y == y.y);
        R(x.width == y.width);
        R(x.height == y.height);
        R(same_metrics(x.metrics, y.metrics));
    }

    newce::Report report{};
    R(newce::encode(
        nb,
        static_cast<std::uint32_t>(source.md.width),
        static_cast<std::uint32_t>(source.md.height),
        newResult,
        report));
    R(report.json.find(
        "\"v01_sparse_reference_reuse_verified\":true") !=
      std::string::npos);
    R(report.json.find(
        "\"v01_rerun_performed\":false") !=
      std::string::npos);
    R(!report.candidateApplied);
    R(!report.createsNewEvidence);
    R(!report.scientificWritebackAllowed);

    auto badReference = reference;
    badReference.correctedSampleCoordinates.pop_back();
    newce::Result shouldFail{};
    R(!newce::run(source, nb, badReference, shouldFail));

    std::cout
        << "N2 sparse-reference v0.2.2 exact-metric parity PASS "
        << "sampled=" << newResult.metrics.sampled
        << " candidates=" << newResult.metrics.v01CandidateCenters
        << " valid=" << newResult.metrics.predictorValid
        << " tiles=" << newResult.tiles.size()
        << "\n";
}
