#include "truthnegative_camera5_color_highlight_oracle_v0_1.h"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace o = truthraw::truthnegative_camera5_color_highlight_oracle::v0_1;
namespace fw = truthraw::free_world_pixel_resolve_2d::v0_2;

#define REQUIRE(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)

namespace {

o::Digest digest(std::uint8_t seed) {
    o::Digest d{};
    for (std::size_t i = 0; i < d.size(); ++i) {
        d[i] = static_cast<std::uint8_t>(seed + i);
    }
    return d;
}

o::Input baseInput() {
    o::Input in{};
    in.sourceEvidenceSha256 = digest(1);
    in.scientificMasterSha256 = digest(33);
    in.truthNegativeStateSha256 = digest(65);
    in.colorBindingSha256 = digest(97);
    in.physicalCameraId = 5;
    in.whiteLevel = 1023.0;
    in.blackPhase = {64.0,64.0,64.0,63.75};
    in.asShotNeutral = {0.8701171875,1.0,0.345703125};
    in.asShotNeutralKnown = true;
    in.remosaicState = o::RemosaicState::Unknown;
    return in;
}

o::Sample candidate(
    double cameraR,
    double cameraB,
    double lowR,
    double lowG,
    double lowB,
    bool clipped = false) {
    o::Sample s{};
    s.cameraNativeRgb = {cameraR, 1.0, cameraB};
    s.xyzD50 = {0.95, 1.0, 1.08};
    s.encodedRgbByExposure = {{
        {0.96,0.96,0.96},
        {0.82,0.83,0.81},
        {0.65,0.68,0.63},
        {0.38,0.43,0.36},
        {lowR,lowG,lowB},
    }};
    s.displayClampByExposure = {true,false,false,false,false};
    s.authority = {
        fw::ResolvedAuthority::Unknown,
        clipped ? fw::ResolvedAuthority::Censored
                : fw::ResolvedAuthority::Reconstructed,
        fw::ResolvedAuthority::Unknown};
    return s;
}

void test_known_camera5_neutral_mismatch_localizes_metadata() {
    auto in = baseInput();
    for (int i=0;i<8;++i) {
        in.samples.push_back(candidate(0.59,0.55,0.27,0.36,0.25));
    }
    o::Report report{};
    REQUIRE(o::run(in, report));
    REQUIRE(report.apparentWhiteHighlightCandidates == 8u);
    REQUIRE(report.empiricalCameraNeutralKnown);
    REQUIRE(report.metadataNeutralMismatch);
    REQUIRE(report.firstFailureStage ==
        o::FirstFailureStage::MetadataNeutralMismatch);
    REQUIRE(!report.remosaicScientificallyResolved);
    REQUIRE(!report.createsNewEvidence);
    REQUIRE(!report.scientificWritebackAllowed);
}

void test_censoring_precedes_color_claim() {
    auto in = baseInput();
    for (int i=0;i<8;++i) {
        in.samples.push_back(candidate(0.59,0.55,0.27,0.36,0.25,true));
    }
    o::Report report{};
    REQUIRE(o::run(in, report));
    REQUIRE(report.sourceCensoringDominant);
    REQUIRE(report.firstFailureStage ==
        o::FirstFailureStage::SourceCensoring);
}

void test_neutral_stable_case_has_no_failure() {
    auto in = baseInput();
    in.asShotNeutral = {0.87,1.0,0.35};
    for (int i=0;i<8;++i) {
        auto s = candidate(0.87,0.35,0.30,0.30,0.30);
        s.encodedRgbByExposure = {{
            {0.90,0.90,0.90},
            {0.75,0.75,0.75},
            {0.60,0.60,0.60},
            {0.42,0.42,0.42},
            {0.30,0.30,0.30},
        }};
        s.displayClampByExposure = {false,false,false,false,false};
        in.samples.push_back(s);
    }
    o::Report report{};
    REQUIRE(o::run(in, report));
    REQUIRE(!report.metadataNeutralMismatch);
    REQUIRE(!report.colorBindingBiasDetected);
    REQUIRE(!report.appearanceDisplayDriftDetected);
    REQUIRE(report.firstFailureStage == o::FirstFailureStage::None);
}

}

int main() {
    test_known_camera5_neutral_mismatch_localizes_metadata();
    test_censoring_precedes_color_claim();
    test_neutral_stable_case_has_no_failure();
    std::cout << "TruthNegativeCamera5ColorHighlightOracle/0.1 PASS\n";
    return 0;
}
