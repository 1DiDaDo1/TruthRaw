#include "truthnegative_n2_row_band_reuse_v0_1.h"
#include "truthnegative_n2_cfa_audit_v0_1.h"
#include "truthnegative_center_excluded_spatial_audit_v0_2_2.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace rb =
    truthraw::truthnegative_n2_row_band_reuse::v0_1;
namespace n2 =
    truthraw::truthnegative_n2_cfa_audit::v0_1;
namespace n2pipe =
    truthraw::truthnegative_n2_candidate_pipeline::v0_1;
namespace ce21 =
    truthraw::truthnegative_center_excluded_spatial_audit::v0_2_1;
namespace ce22 =
    truthraw::truthnegative_center_excluded_spatial_audit::v0_2_2;
namespace st = truthraw::streaming_v0_1;

#define R(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)

class FakeSource final : public st::IRawTileSource {
public:
    FakeSource() {
        md.width = 192;
        md.height = 96;
        md.cfa = truthraw::CfaPattern::BGGR;
        md.whiteLevel = 1023;
        md.blackPhase = {64, 65, 66, 67};
        md.hasNoiseProfile = true;
        md.noiseProfile = {
            0.004f, 0.00002f,
            0.0042f, 0.000021f,
            0.0044f, 0.000022f};
        md.hasGainField = true;
        md.hasResidualBlack = true;

        raw.resize(
            static_cast<std::size_t>(md.width) *
            static_cast<std::size_t>(md.height));
        for (int y = 0; y < md.height; ++y) {
            for (int x = 0; x < md.width; ++x) {
                int value =
                    280 +
                    ((x + 2 * y) % 5) -
                    2;
                if ((x / 32) % 2 == 1 &&
                    y >= 8 &&
                    y < 88) {
                    value += ((y / 4) % 2 == 0)
                        ? 18
                        : -18;
                }
                if (x >= 136 && x < 152 &&
                    y >= 40 && y < 56) {
                    value = 1023;
                }
                raw[
                    static_cast<std::size_t>(y) *
                        static_cast<std::size_t>(md.width) +
                    static_cast<std::size_t>(x)] =
                    static_cast<std::uint16_t>(
                        std::clamp(value, 0, 1023));
            }
        }
    }

    const truthraw::DngMetadata& metadata() const override {
        return md;
    }

    std::size_t residentBytesUpperBound() const override {
        return raw.size() * sizeof(std::uint16_t);
    }

    st::StreamStatus readRawTile(
        const truthraw::TileRect& t,
        std::uint16_t* out,
        std::size_t n,
        float* gainOut,
        std::size_t gainCount) override {
        const int width = t.hx1 - t.hx0;
        const int height = t.hy1 - t.hy0;
        if (width <= 0 || height <= 0 || !out) {
            return st::StreamStatus::error(
                st::StreamStatusCode::InvalidArgument,
                "invalid fake tile");
        }
        const auto need =
            static_cast<std::size_t>(width) *
            static_cast<std::size_t>(height);
        if (n < need ||
            (md.hasGainField &&
             (!gainOut || gainCount < need))) {
            return st::StreamStatus::error(
                st::StreamStatusCode::InvalidArgument,
                "fake tile output too small");
        }

        for (int yy = 0; yy < height; ++yy) {
            for (int xx = 0; xx < width; ++xx) {
                const int gx = t.hx0 + xx;
                const int gy = t.hy0 + yy;
                const auto dst =
                    static_cast<std::size_t>(yy) *
                        static_cast<std::size_t>(width) +
                    static_cast<std::size_t>(xx);
                const auto src =
                    static_cast<std::size_t>(gy) *
                        static_cast<std::size_t>(md.width) +
                    static_cast<std::size_t>(gx);
                out[dst] = raw[src];
                gainOut[dst] =
                    1.0f +
                    0.0005f *
                        static_cast<float>(
                            (gx % 7) + 2 * (gy % 5));
            }
        }
        ++readCalls;
        return st::StreamStatus::ok();
    }

    st::StreamStatus readRowBias(
        int y0,
        int y1,
        float* out,
        std::size_t count) override {
        if (y0 < 0 || y1 < y0 ||
            count < static_cast<std::size_t>(y1 - y0) ||
            (y1 > y0 && !out)) {
            return st::StreamStatus::error(
                st::StreamStatusCode::InvalidArgument,
                "row bias");
        }
        for (int y = y0; y < y1; ++y) {
            out[static_cast<std::size_t>(y - y0)] =
                0.05f * static_cast<float>(y % 3);
        }
        return st::StreamStatus::ok();
    }

    st::StreamStatus readColBias(
        int x0,
        int x1,
        float* out,
        std::size_t count) override {
        if (x0 < 0 || x1 < x0 ||
            count < static_cast<std::size_t>(x1 - x0) ||
            (x1 > x0 && !out)) {
            return st::StreamStatus::error(
                st::StreamStatusCode::InvalidArgument,
                "col bias");
        }
        for (int x = x0; x < x1; ++x) {
            out[static_cast<std::size_t>(x - x0)] =
                0.03f * static_cast<float>(x % 4);
        }
        return st::StreamStatus::ok();
    }

    truthraw::DngMetadata md{};
    std::vector<std::uint16_t> raw{};
    std::uint64_t readCalls = 0u;
};

bool same_n2_audit(
    const n2pipe::Audit& a,
    const n2pipe::Audit& b) {
    return
        a.total == b.total &&
        a.eligible == b.eligible &&
        a.corrected == b.corrected &&
        a.preserved == b.preserved &&
        a.censoredProtected == b.censoredProtected &&
        a.censorBoundaryProtected == b.censorBoundaryProtected &&
        a.structureProtected == b.structureProtected &&
        a.unknownNoiseProtected == b.unknownNoiseProtected &&
        a.noNeighborhoodProtected == b.noNeighborhoodProtected &&
        a.residualOutlierProtected == b.residualOutlierProtected &&
        a.totalResidualEnergy == b.totalResidualEnergy &&
        a.removedResidualEnergy == b.removedResidualEnergy &&
        a.maxAbsCorrection == b.maxAbsCorrection;
}

bool same_v01(
    const n2::Result& a,
    const n2::Result& b) {
    if (!same_n2_audit(a.audit, b.audit) ||
        a.candidateSha256 != b.candidateSha256 ||
        a.auditSha256 != b.auditSha256 ||
        a.spatialSha256 != b.spatialSha256 ||
        a.appearanceGridSha256 != b.appearanceGridSha256 ||
        a.cfaPhaseSamples != b.cfaPhaseSamples ||
        a.borderProtected != b.borderProtected ||
        a.sampled != b.sampled ||
        a.samplingPeriod != b.samplingPeriod ||
        a.tileEdge != b.tileEdge ||
        a.correctedSampleCoordinatesComplete !=
            b.correctedSampleCoordinatesComplete ||
        a.correctedSampleCoordinates.size() !=
            b.correctedSampleCoordinates.size() ||
        a.tiles.size() != b.tiles.size() ||
        a.appearanceGrid.size() != b.appearanceGrid.size()) {
        return false;
    }

    for (std::size_t i = 0;
         i < a.correctedSampleCoordinates.size();
         ++i) {
        if (a.correctedSampleCoordinates[i].x !=
                b.correctedSampleCoordinates[i].x ||
            a.correctedSampleCoordinates[i].y !=
                b.correctedSampleCoordinates[i].y) {
            return false;
        }
    }

    for (std::size_t i = 0; i < a.tiles.size(); ++i) {
        const auto& x = a.tiles[i];
        const auto& y = b.tiles[i];
        if (x.x != y.x ||
            x.y != y.y ||
            x.width != y.width ||
            x.height != y.height ||
            x.sampled != y.sampled ||
            x.borderProtected != y.borderProtected ||
            x.cfaPhaseSamples != y.cfaPhaseSamples ||
            x.correctedSampleOffset != y.correctedSampleOffset ||
            x.correctedSampleCount != y.correctedSampleCount ||
            !same_n2_audit(x.audit, y.audit)) {
            return false;
        }
    }
    return true;
}

template <typename A, typename B>
bool same_ce_metrics(
    const A& a,
    const B& b) {
    return
        a.sampled == b.sampled &&
        a.v01CandidateCenters == b.v01CandidateCenters &&
        a.predictorValid == b.predictorValid &&
        a.predictorInvalid == b.predictorInvalid &&
        a.symmetricPairsConsidered == b.symmetricPairsConsidered &&
        a.symmetricPairsAccepted == b.symmetricPairsAccepted &&
        a.symmetricPairsRejected == b.symmetricPairsRejected &&
        a.scalesConsidered == b.scalesConsidered &&
        a.scalesAccepted == b.scalesAccepted &&
        a.scalesRejected == b.scalesRejected &&
        a.centerResidualWithin1Sigma == b.centerResidualWithin1Sigma &&
        a.centerResidualBetween1And2Sigma == b.centerResidualBetween1And2Sigma &&
        a.centerResidualAbove2Sigma == b.centerResidualAbove2Sigma &&
        a.combinedResidualWithin1Sigma == b.combinedResidualWithin1Sigma &&
        a.combinedResidualBetween1And2Sigma == b.combinedResidualBetween1And2Sigma &&
        a.combinedResidualAbove2Sigma == b.combinedResidualAbove2Sigma &&
        a.absResidualSum == b.absResidualSum &&
        a.maxAbsResidual == b.maxAbsResidual &&
        a.centerVarianceSum == b.centerVarianceSum &&
        a.estimateVarianceSum == b.estimateVarianceSum &&
        a.estimateToCenterVarianceRatioSum ==
            b.estimateToCenterVarianceRatioSum &&
        a.maxEstimateToCenterVarianceRatio ==
            b.maxEstimateToCenterVarianceRatio &&
        a.maxDirectionalDisagreementSigma ==
            b.maxDirectionalDisagreementSigma &&
        a.maxCrossScaleDisagreementSigma ==
            b.maxCrossScaleDisagreementSigma &&
        a.v01CandidateCfaPhase == b.v01CandidateCfaPhase &&
        a.predictorValidCfaPhase == b.predictorValidCfaPhase;
}

bool same_ce_result(
    const ce21::Result& a,
    const ce21::Result& b) {
    if (a.auditSha256 != b.auditSha256 ||
        a.tileEdge != b.tileEdge ||
        a.samplingPeriod != b.samplingPeriod ||
        !same_ce_metrics(a.metrics, b.metrics) ||
        a.tiles.size() != b.tiles.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.tiles.size(); ++i) {
        const auto& x = a.tiles[i];
        const auto& y = b.tiles[i];
        if (x.x != y.x ||
            x.y != y.y ||
            x.width != y.width ||
            x.height != y.height ||
            !same_ce_metrics(x.metrics, y.metrics)) {
            return false;
        }
    }
    return true;
}

int main() {
    n2::Binding binding{};
    binding.sourceEvidenceSha256[0] = 1u;
    binding.truthNegativeStateSha256[0] = 2u;

    n2::Options options{};
    options.tileEdge = 32u;
    options.samplingPeriod = 8u;
    options.appearanceGridWidth = 12u;
    options.appearanceGridHeight = 6u;

    FakeSource directSource;
    n2::Result directV01{};
    R(n2::run(
        directSource,
        binding,
        options,
        directV01));
    R(directV01.audit.corrected > 0u);

    FakeSource cachedSource;
    rb::RowBandReuseTileSource rowBand(
        cachedSource,
        8u * 1024u * 1024u);
    n2::Result cachedV01{};
    R(n2::run(
        rowBand,
        binding,
        options,
        cachedV01));

    R(same_v01(directV01, cachedV01));
    R(rowBand.bandFillCount() > 0u);
    R(rowBand.bandCacheHitRequestCount() > 0u);
    R(rowBand.fallbackRequestCount() == 0u);
    R(cachedSource.readCalls < directSource.readCalls);
    R(!rowBand.scientificValuesModified());
    R(!rowBand.createsNewEvidence());
    R(!rowBand.scientificWritebackAllowed());

    ce21::Binding ceBinding{};
    ceBinding.sourceEvidenceSha256 =
        binding.sourceEvidenceSha256;
    ceBinding.scientificMasterSha256[0] = 3u;
    ceBinding.authorityFieldSha256[0] = 4u;
    ceBinding.truthNegativeStateSha256 =
        binding.truthNegativeStateSha256;
    ceBinding.v01CandidateSha256 =
        directV01.candidateSha256;
    ceBinding.v01AuditSha256 =
        directV01.auditSha256;
    ceBinding.v01SpatialSha256 =
        directV01.spatialSha256;

    FakeSource ceDirectSource;
    ce21::Result directCe{};
    R(ce22::runSparseReference(
        ceDirectSource,
        ceBinding,
        directV01,
        directCe));

    FakeSource ceCachedSource;
    rb::RowBandReuseTileSource ceRowBand(
        ceCachedSource,
        8u * 1024u * 1024u);
    ce21::Result cachedCe{};
    R(ce22::runSparseReference(
        ceRowBand,
        ceBinding,
        cachedV01,
        cachedCe));

    R(same_ce_result(directCe, cachedCe));
    R(ceRowBand.bandFillCount() > 0u);
    R(ceRowBand.fallbackRequestCount() == 0u);
    R(ceCachedSource.readCalls <= ceDirectSource.readCalls);

    FakeSource fallbackSource;
    rb::RowBandReuseTileSource tinyCache(
        fallbackSource,
        64u);
    n2::Result fallbackV01{};
    R(n2::run(
        tinyCache,
        binding,
        options,
        fallbackV01));
    R(same_v01(directV01, fallbackV01));
    R(tinyCache.bandFillCount() == 0u);
    R(tinyCache.fallbackRequestCount() > 0u);

    std::cout
        << "N2 row-band reuse v0.1 exact parity PASS "
        << "direct_reads=" << directSource.readCalls
        << " cached_underlying_reads=" << cachedSource.readCalls
        << " band_fills=" << rowBand.bandFillCount()
        << " served=" << rowBand.bandServedRequestCount()
        << " cache_hits=" << rowBand.bandCacheHitRequestCount()
        << " peak_cache_bytes=" << rowBand.peakCacheBytes()
        << "\n";
}
