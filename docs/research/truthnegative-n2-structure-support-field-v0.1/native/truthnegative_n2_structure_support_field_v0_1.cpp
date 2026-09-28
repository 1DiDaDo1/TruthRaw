#include "truthnegative_n2_structure_support_field_v0_1.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace truthraw::truthnegative_n2_structure_support_field::v0_1 {
namespace {

bool nonzero(const Digest& d) noexcept {
    return std::any_of(
        d.begin(), d.end(), [](std::uint8_t v){ return v != 0u; });
}

std::string hex(const Digest& d) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out(d.size() * 2u, '0');
    for (std::size_t i = 0u; i < d.size(); ++i) {
        out[2u * i] = kHex[d[i] >> 4u];
        out[2u * i + 1u] = kHex[d[i] & 0x0fu];
    }
    return out;
}

double fraction(std::uint64_t n, std::uint64_t d) noexcept {
    if (d == 0u) return 0.0;
    return static_cast<double>(n) / static_cast<double>(d);
}

bool finite_fraction(double v) noexcept {
    return std::isfinite(v) && v >= 0.0 && v <= 1.0;
}

} // namespace

bool encode(
    const Binding& binding,
    std::uint32_t sourceWidth,
    std::uint32_t sourceHeight,
    const cfa::Result& audit,
    Report& out) noexcept {
    out = {};
    try {
        if (sourceWidth == 0u || sourceHeight == 0u ||
            !nonzero(binding.sourceEvidenceSha256) ||
            !nonzero(binding.scientificMasterSha256) ||
            !nonzero(binding.authorityFieldSha256) ||
            !nonzero(binding.truthNegativeStateSha256) ||
            !nonzero(audit.candidateSha256) ||
            !nonzero(audit.auditSha256) ||
            !nonzero(audit.spatialSha256) ||
            audit.tileEdge != kTileEdge ||
            audit.samplingPeriod != kSamplingPeriod ||
            audit.sampled == 0u ||
            audit.tiles.empty() ||
            audit.audit.total != audit.sampled ||
            audit.sourceValuesModified ||
            audit.truthNegativeModified ||
            audit.createsNewEvidence ||
            audit.scientificWritebackAllowed) {
            return false;
        }

        std::uint64_t tileSamples = 0u;
        std::uint64_t structureProtected = 0u;
        std::uint64_t censoredProtected = 0u;
        std::uint64_t censorBoundaryProtected = 0u;

        for (const auto& t : audit.tiles) {
            if (t.width == 0u || t.height == 0u ||
                t.x + t.width > sourceWidth ||
                t.y + t.height > sourceHeight ||
                t.audit.total != t.sampled ||
                t.sampled == 0u ||
                t.audit.structureProtected > t.sampled ||
                t.audit.censoredProtected > t.sampled ||
                t.audit.censorBoundaryProtected > t.sampled) {
                return false;
            }
            tileSamples += t.sampled;
            structureProtected += t.audit.structureProtected;
            censoredProtected += t.audit.censoredProtected;
            censorBoundaryProtected += t.audit.censorBoundaryProtected;
        }

        if (tileSamples != audit.sampled ||
            structureProtected != audit.audit.structureProtected ||
            censoredProtected != audit.audit.censoredProtected ||
            censorBoundaryProtected !=
                audit.audit.censorBoundaryProtected) {
            return false;
        }

        const double globalStructureFraction =
            fraction(structureProtected, audit.sampled);
        const double globalCensorFraction =
            fraction(censoredProtected, audit.sampled);
        const double globalBoundaryFraction =
            fraction(censorBoundaryProtected, audit.sampled);
        if (!finite_fraction(globalStructureFraction) ||
            !finite_fraction(globalCensorFraction) ||
            !finite_fraction(globalBoundaryFraction)) {
            return false;
        }

        std::ostringstream o;
        o.setf(std::ios::fixed);
        o << std::setprecision(12);
        o << "{\n";
        o << "  \"schema\":\"" << kSchemaName << "\",\n";
        o << "  \"source_width\":" << sourceWidth << ",\n";
        o << "  \"source_height\":" << sourceHeight << ",\n";
        o << "  \"tile_edge\":" << audit.tileEdge << ",\n";
        o << "  \"sampling_period\":" << audit.samplingPeriod << ",\n";
        o << "  \"source_sha256\":\""
          << hex(binding.sourceEvidenceSha256) << "\",\n";
        o << "  \"scientific_master_sha256\":\""
          << hex(binding.scientificMasterSha256) << "\",\n";
        o << "  \"authority_field_sha256\":\""
          << hex(binding.authorityFieldSha256) << "\",\n";
        o << "  \"truthnegative_state_sha256\":\""
          << hex(binding.truthNegativeStateSha256) << "\",\n";
        o << "  \"candidate_sha256\":\""
          << hex(audit.candidateSha256) << "\",\n";
        o << "  \"audit_sha256\":\""
          << hex(audit.auditSha256) << "\",\n";
        o << "  \"spatial_sha256\":\""
          << hex(audit.spatialSha256) << "\",\n";
        o << "  \"sample_grid_evidence_only\":true,\n";
        o << "  \"unsampled_pixels_inferred\":false,\n";
        o << "  \"can_reduce_protection\":false,\n";
        o << "  \"can_enable_correction\":false,\n";
        o << "  \"promotion_eligible\":false,\n";
        o << "  \"candidate_applied\":false,\n";
        o << "  \"creates_new_evidence\":false,\n";
        o << "  \"scientific_writeback_allowed\":false,\n";
        o << "  \"global\":{";
        o << "\"sampled\":" << audit.sampled;
        o << ",\"structure_protected\":" << structureProtected;
        o << ",\"structure_protection_fraction\":"
          << globalStructureFraction;
        o << ",\"censored_protected\":" << censoredProtected;
        o << ",\"censor_protection_fraction\":"
          << globalCensorFraction;
        o << ",\"censor_boundary_protected\":"
          << censorBoundaryProtected;
        o << ",\"censor_boundary_protection_fraction\":"
          << globalBoundaryFraction;
        o << "},\n";
        o << "  \"tiles\":[\n";

        for (std::size_t i = 0u; i < audit.tiles.size(); ++i) {
            const auto& t = audit.tiles[i];
            const double sf =
                fraction(t.audit.structureProtected, t.sampled);
            const double cf =
                fraction(t.audit.censoredProtected, t.sampled);
            const double bf =
                fraction(t.audit.censorBoundaryProtected, t.sampled);
            if (!finite_fraction(sf) ||
                !finite_fraction(cf) ||
                !finite_fraction(bf)) {
                return false;
            }

            o << "    {\"x\":" << t.x
              << ",\"y\":" << t.y
              << ",\"width\":" << t.width
              << ",\"height\":" << t.height
              << ",\"sampled\":" << t.sampled
              << ",\"structure_protected\":"
              << t.audit.structureProtected
              << ",\"structure_protection_fraction\":" << sf
              << ",\"censored_protected\":"
              << t.audit.censoredProtected
              << ",\"censor_protection_fraction\":" << cf
              << ",\"censor_boundary_protected\":"
              << t.audit.censorBoundaryProtected
              << ",\"censor_boundary_protection_fraction\":" << bf
              << ",\"structure_protection_present\":"
              << (t.audit.structureProtected > 0u ? "true" : "false")
              << "}";
            if (i + 1u < audit.tiles.size()) o << ",";
            o << "\n";
        }

        o << "  ]\n";
        o << "}\n";

        out.json = o.str();
        truthraw::sha256_v0_69::Hasher hasher;
        hasher.update(
            reinterpret_cast<const std::uint8_t*>(out.json.data()),
            out.json.size());
        out.jsonSha256 = hasher.finalize();

        out.tileCount = audit.tiles.size();
        out.sampled = audit.sampled;
        out.structureProtected = structureProtected;
        out.censoredProtected = censoredProtected;
        out.censorBoundaryProtected = censorBoundaryProtected;
        out.sampleGridEvidenceOnly = true;
        out.unsampledPixelsInferred = false;
        out.canReduceProtection = false;
        out.canEnableCorrection = false;
        out.createsNewEvidence = false;
        out.scientificWritebackAllowed = false;
        out.candidateApplied = false;

        return !out.json.empty() && nonzero(out.jsonSha256);
    } catch (...) {
        out = {};
        return false;
    }
}

} // namespace truthraw::truthnegative_n2_structure_support_field::v0_1
