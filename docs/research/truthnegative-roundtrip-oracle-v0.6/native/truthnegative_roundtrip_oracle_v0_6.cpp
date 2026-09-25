#include "truthnegative_roundtrip_oracle_v0_6.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>

namespace truthraw::truthnegative_roundtrip_oracle::v0_6 {
namespace {

bool nonzero(const Digest& d) noexcept {
    return std::any_of(
        d.begin(), d.end(), [](std::uint8_t v) { return v != 0u; });
}

void hash_u32(
    truthraw::sha256_v0_69::Hasher& h,
    std::uint32_t v) noexcept {
    const std::array<std::uint8_t, 4u> b{
        static_cast<std::uint8_t>(v),
        static_cast<std::uint8_t>(v >> 8u),
        static_cast<std::uint8_t>(v >> 16u),
        static_cast<std::uint8_t>(v >> 24u),
    };
    h.update(b);
}

void hash_u64(
    truthraw::sha256_v0_69::Hasher& h,
    std::uint64_t v) noexcept {
    std::array<std::uint8_t, 8u> b{};
    for (unsigned i = 0u; i < 8u; ++i) {
        b[i] = static_cast<std::uint8_t>(v >> (8u * i));
    }
    h.update(b);
}

void hash_f64(
    truthraw::sha256_v0_69::Hasher& h,
    double v) noexcept {
    hash_u64(h, std::bit_cast<std::uint64_t>(v));
}

std::size_t authority_index(
    free_world::ResolvedAuthority authority) noexcept {
    switch (authority) {
        case free_world::ResolvedAuthority::Reconstructed: return 0u;
        case free_world::ResolvedAuthority::Censored: return 1u;
        case free_world::ResolvedAuthority::Unknown: return 2u;
    }
    return 2u;
}

bool normalized_footprint(
    const free_world::ResolvedPixel& pixel) noexcept {
    if (pixel.footprint.empty()) return false;
    double sum = 0.0;
    for (const auto& f : pixel.footprint) {
        if (!(f.weight > 0.0) || !std::isfinite(f.weight)) return false;
        sum += f.weight;
    }
    return std::isfinite(sum) && std::abs(sum - 1.0) <= 1e-12;
}

}  // namespace

bool run(
    const free_world::IScenePlaneSource& scene,
    const tn::State& state,
    const std::vector<RasterSpec>& rasters,
    double tolerance,
    Report& out) noexcept {
    out = Report{};
    try {
        if (!state.finalized ||
            !nonzero(state.stateSha256) ||
            state.createsNewEvidence ||
            state.scientificWritebackAllowed ||
            scene.width() != state.input.width ||
            scene.height() != state.input.height ||
            rasters.empty() ||
            !std::isfinite(tolerance) ||
            tolerance <= 0.0) {
            return false;
        }

        tn::QueryResult canonical{};
        if (!tn::resolvePixel(scene, state, 1u, 1u, 0u, 0u, canonical) ||
            canonical.stateSha256 != state.stateSha256 ||
            canonical.stateIdentityChangedByTargetRaster ||
            canonical.createsNewEvidence ||
            canonical.scientificWritebackAllowed ||
            !normalized_footprint(canonical.pixel)) {
            return false;
        }

        out.truthNegativeStateSha256 = state.stateSha256;
        out.canonicalGlobalMean = canonical.pixel.sceneLinear;
        out.tolerance = tolerance;
        out.rasters.reserve(rasters.size());

        bool allConserved = true;
        bool allState = true;
        bool allFootprints = true;
        bool allNoPromotion = true;

        truthraw::sha256_v0_69::Hasher oracleHasher;
        constexpr char domain[] =
            "D_RAW_TRUTHNEGATIVE_ROUNDTRIP_ORACLE_V0_6";
        oracleHasher.update(
            reinterpret_cast<const std::uint8_t*>(domain),
            sizeof(domain) - 1u);
        oracleHasher.update(state.stateSha256);
        hash_f64(oracleHasher, tolerance);
        for (double v : out.canonicalGlobalMean) {
            hash_f64(oracleHasher, v);
        }

        for (const auto& spec : rasters) {
            if (spec.width == 0u || spec.height == 0u) return false;
            const std::uint64_t count =
                static_cast<std::uint64_t>(spec.width) *
                static_cast<std::uint64_t>(spec.height);
            if (count == 0u ||
                count >
                    static_cast<std::uint64_t>(
                        std::numeric_limits<std::size_t>::max())) {
                return false;
            }

            tn::RasterResolver resolver(
                scene, state, spec.width, spec.height);
            if (!resolver.valid()) return false;

            RasterReport rr{};
            rr.raster = spec;
            rr.pixelCount = count;
            rr.stateIdentityPreserved = true;
            rr.footprintsNormalized = true;
            rr.noEvidencePromotion = true;

            truthraw::sha256_v0_69::Hasher chain;
            constexpr char chainDomain[] =
                "D_RAW_TRUTHNEGATIVE_QUERY_CHAIN_V0_6";
            chain.update(
                reinterpret_cast<const std::uint8_t*>(chainDomain),
                sizeof(chainDomain) - 1u);
            chain.update(state.stateSha256);
            hash_u32(chain, spec.width);
            hash_u32(chain, spec.height);

            for (std::uint32_t y = 0u; y < spec.height; ++y) {
                for (std::uint32_t x = 0u; x < spec.width; ++x) {
                    tn::QueryResult query{};
                    if (!resolver.resolvePixel(x, y, query)) return false;

                    rr.stateIdentityPreserved =
                        rr.stateIdentityPreserved &&
                        query.stateSha256 == state.stateSha256 &&
                        !query.stateIdentityChangedByTargetRaster;
                    rr.footprintsNormalized =
                        rr.footprintsNormalized &&
                        normalized_footprint(query.pixel);

                    const bool noPromotion =
                        query.pixel.measuredTargetClaimCount == 0u &&
                        !query.pixel.createsNewEvidence &&
                        query.pixel.physicalFrameCount == 1u &&
                        query.pixel.independentEvidenceCount == 1u &&
                        !query.createsNewEvidence &&
                        !query.scientificWritebackAllowed;
                    rr.noEvidencePromotion =
                        rr.noEvidencePromotion && noPromotion;

                    rr.measuredTargetClaimCount +=
                        query.pixel.measuredTargetClaimCount;
                    rr.footprintLinkCount +=
                        static_cast<std::uint64_t>(
                            query.pixel.footprint.size());

                    for (std::size_t c = 0u; c < 3u; ++c) {
                        rr.meanSceneLinear[c] +=
                            query.pixel.sceneLinear[c] /
                            static_cast<double>(count);
                        ++rr.resolvedAuthorityCounts[
                            authority_index(
                                query.pixel.support[c].authority)];
                    }
                    chain.update(query.querySha256);
                }
            }

            rr.queryChainSha256 = chain.finalize();
            if (!nonzero(rr.queryChainSha256)) return false;

            for (std::size_t c = 0u; c < 3u; ++c) {
                rr.absoluteMeanError[c] =
                    std::abs(
                        rr.meanSceneLinear[c] -
                        out.canonicalGlobalMean[c]);
                out.maximumAbsoluteMeanError =
                    std::max(
                        out.maximumAbsoluteMeanError,
                        rr.absoluteMeanError[c]);
                if (rr.absoluteMeanError[c] > tolerance) {
                    allConserved = false;
                }
            }

            allState = allState && rr.stateIdentityPreserved;
            allFootprints =
                allFootprints && rr.footprintsNormalized;
            allNoPromotion =
                allNoPromotion && rr.noEvidencePromotion &&
                rr.measuredTargetClaimCount == 0u;

            hash_u32(oracleHasher, spec.width);
            hash_u32(oracleHasher, spec.height);
            oracleHasher.update(rr.queryChainSha256);
            for (double v : rr.meanSceneLinear) hash_f64(oracleHasher, v);
            for (double v : rr.absoluteMeanError) hash_f64(oracleHasher, v);
            for (auto n : rr.resolvedAuthorityCounts) {
                hash_u64(oracleHasher, n);
            }
            hash_u64(oracleHasher, rr.footprintLinkCount);

            out.rasters.push_back(rr);
        }

        out.globalAreaConserved = allConserved;
        out.stateIdentityPreserved = allState;
        out.footprintsNormalized = allFootprints;
        out.noEvidencePromotion = allNoPromotion;
        out.createsNewEvidence = false;
        out.scientificWritebackAllowed = false;
        out.oracleSha256 = oracleHasher.finalize();

        return nonzero(out.oracleSha256) &&
               out.globalAreaConserved &&
               out.stateIdentityPreserved &&
               out.footprintsNormalized &&
               out.noEvidencePromotion;
    } catch (...) {
        out = Report{};
        return false;
    }
}

const char* schema_name() noexcept {
    return kSchemaName;
}

}  // namespace truthraw::truthnegative_roundtrip_oracle::v0_6
