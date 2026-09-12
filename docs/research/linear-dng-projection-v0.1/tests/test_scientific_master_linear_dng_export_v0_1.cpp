#include "scientific_master_camera_rgb_source_v0_1.h"
#include "scientific_master_streaming_binding_v0_2.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace ldng = truthraw::linear_dng_projection::v0_1;
namespace smsb2 = truthraw::scientific_master_streaming_binding::v0_2;
namespace spb1 = truthraw::scientific_preview_binding_v0_1;

namespace {

#define CHECK_TRUE(expr) do { if (!(expr)) { \
    std::cerr << "CHECK failed at line " << __LINE__ << ": " #expr "\n"; return 1; } } while (0)

class TestReconstruction final : public truthraw::IReconstructionBackend {
public:
    truthraw::ReconstructionQuality quality() const override {
        return truthraw::ReconstructionQuality::ResearchBackend;
    }
    const char* name() const override { return "linear_dng_test_reconstruction"; }
    int requiredHalo() const override { return 0; }
    truthraw::Status reconstructTile(
        const float* stage2, int tileW, int tileH,
        int globalHx0, int globalHy0,
        int coreX0, int coreY0, int coreW, int coreH,
        truthraw::CfaPattern, float* out) override {
        if (!stage2 || !out || tileW <= 0 || tileH <= 0) {
            return truthraw::Status::error(truthraw::StatusCode::BackendFailed, "bad test reconstruction input");
        }
        for (int y=0; y<coreH; ++y) {
            for (int x=0; x<coreW; ++x) {
                const int sx = coreX0 + x - globalHx0;
                const int sy = coreY0 + y - globalHy0;
                if (sx < 0 || sx >= tileW || sy < 0 || sy >= tileH) {
                    return truthraw::Status::error(truthraw::StatusCode::BackendFailed, "bad core coordinates");
                }
                const float v = stage2[static_cast<std::size_t>(sy) * tileW + sx];
                const std::size_t i = (static_cast<std::size_t>(y) * coreW + x) * 3u;
                out[i] = v;
                out[i+1] = v * 0.8f;
                out[i+2] = v * 0.6f;
            }
        }
        return truthraw::Status::ok();
    }
};

class FrameSource final : public truthraw::streaming_v0_1::IRawTileSource {
public:
    FrameSource(int width, int height, std::string sourceId) {
        meta_.width = width;
        meta_.height = height;
        meta_.cfa = truthraw::CfaPattern::BGGR;
        meta_.orientation = truthraw::Orientation::Normal;
        meta_.whiteLevel = 1023.0f;
        meta_.blackPhase = {64.f,64.f,64.f,64.f};
        meta_.hasGainField = false;
        meta_.hasResidualBlack = false;
        meta_.sourceId = std::move(sourceId);
        raw_.resize(static_cast<std::size_t>(width) * height);
        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                raw_[static_cast<std::size_t>(y) * width + x] =
                    static_cast<std::uint16_t>(100 + ((x*17 + y*29 + x*y) % 800));
            }
        }
    }
    const truthraw::DngMetadata& metadata() const override { return meta_; }
    std::size_t residentBytesUpperBound() const override { return raw_.size()*sizeof(std::uint16_t); }
    truthraw::streaming_v0_1::StreamStatus readRawTile(
        const truthraw::TileRect& rect, std::uint16_t* out, std::size_t rawCount,
        float* gainOut, std::size_t gainCount) override {
        const int w=rect.hx1-rect.hx0;
        const int h=rect.hy1-rect.hy0;
        const std::size_t count=static_cast<std::size_t>(w)*h;
        if (!out || rawCount!=count || gainOut || gainCount!=0u) {
            return truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::SourceFailed,"bad tile request");
        }
        ++tileCalls;
        for (int y=0;y<h;++y) for (int x=0;x<w;++x) {
            out[static_cast<std::size_t>(y)*w+x]=
                raw_[static_cast<std::size_t>(rect.hy0+y)*meta_.width+(rect.hx0+x)];
        }
        return truthraw::streaming_v0_1::StreamStatus::ok();
    }
    truthraw::streaming_v0_1::StreamStatus readRowBias(int,int,float*,std::size_t n) override {
        return n==0u ? truthraw::streaming_v0_1::StreamStatus::ok() :
            truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::SourceFailed,"unexpected row bias");
    }
    truthraw::streaming_v0_1::StreamStatus readColBias(int,int,float*,std::size_t n) override {
        return n==0u ? truthraw::streaming_v0_1::StreamStatus::ok() :
            truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::SourceFailed,"unexpected col bias");
    }
    std::size_t tileCalls=0u;
private:
    truthraw::DngMetadata meta_{};
    std::vector<std::uint16_t> raw_;
};

class TransactionSink final : public ldng::ITransactionalByteSink {
public:
    ldng::Status append(const std::uint8_t* data,std::size_t size) override {
        if (committed || aborted || (!data && size!=0u)) {
            return ldng::Status::error(ldng::StatusCode::SinkFailed,"invalid transaction append");
        }
        staged.insert(staged.end(),data,data+size);
        return ldng::Status::ok();
    }
    std::uint64_t bytesWritten() const noexcept override { return staged.size(); }
    ldng::Status commit() override {
        if (aborted || committed) return ldng::Status::error(ldng::StatusCode::SinkFailed,"bad commit state");
        committed=true;
        published=staged;
        return ldng::Status::ok();
    }
    void abort() noexcept override {
        aborted=true;
        committed=false;
        staged.clear();
        published.clear();
    }
    std::vector<std::uint8_t> staged;
    std::vector<std::uint8_t> published;
    bool committed=false;
    bool aborted=false;
};

ldng::ProjectionAdmission make_admission(const smsb2::Result& master,const std::string& sourceId) {
    ldng::ProjectionAdmission a;
    a.source.sha256.fill(0x44u);
    a.source.byteLength=9999;
    a.source.sourceEvidenceId=sourceId;
    a.scientificMasterHash=master.scientificMasterHash;
    a.cameraToXyzD50={1.f,0.f,0.f,0.f,1.f,0.f,0.f,0.f,1.f};
    a.colorAuthority=spb1::ColorBindingAuthority::SourceMetadataBound;
    a.strongerPhysicalColorClaim=false;
    a.physicalFrameCount=1;
    a.independentEvidenceCount=1;
    return a;
}

int test_bound_export_and_abort() {
    const std::string sourceId=
        "sha256:4444444444444444444444444444444444444444444444444444444444444444";
    FrameSource masterSource(130,70,sourceId);
    TestReconstruction masterReconstruction;
    smsb2::Result master{};
    const auto ms=smsb2::bind_scientific_master_streaming(
        masterSource,masterReconstruction,smsb2::Options{},master);
    CHECK_TRUE(static_cast<bool>(ms));
    CHECK_TRUE(master.masterTilesProcessed==6u);
    CHECK_TRUE(master.stage2GaugeScanPasses==2u);

    auto admission=make_admission(master,sourceId);
    FrameSource exportSource(130,70,sourceId);
    auto exportBackend=std::make_shared<TestReconstruction>();
    TransactionSink sink;
    ldng::Options options;
    options.tileEdge=64;
    ldng::ScientificExportAudit audit;
    auto status=ldng::write_linear_dng_from_finalized_scientific_source(
        exportSource,exportBackend,sink,admission,options,audit);
    CHECK_TRUE(static_cast<bool>(status));
    CHECK_TRUE(sink.committed && !sink.aborted && !sink.published.empty());
    CHECK_TRUE(audit.sourceIdentityChecked && audit.scientificMasterDigestReverified &&
               audit.transactionCommitted);
    CHECK_TRUE(!audit.appearanceApplied && !audit.cameraToXyzApplied);
    CHECK_TRUE(!audit.projection.createsEvidence && !audit.projection.scientificMasterModified);
    CHECK_TRUE(audit.projection.physicalFrameCount==1u && audit.projection.independentEvidenceCount==1u);
    CHECK_TRUE(audit.reconstructedTiles==6u);
    CHECK_TRUE(exportSource.tileCalls==6u);

    admission.scientificMasterHash[0]^=1u;
    FrameSource badSource(130,70,sourceId);
    TransactionSink badSink;
    ldng::ScientificExportAudit badAudit;
    status=ldng::write_linear_dng_from_finalized_scientific_source(
        badSource,std::make_shared<TestReconstruction>(),badSink,admission,options,badAudit);
    CHECK_TRUE(status.code==ldng::StatusCode::ScientificIdentityMismatch);
    CHECK_TRUE(badSink.aborted && !badSink.committed);
    CHECK_TRUE(badSink.published.empty() && badSink.staged.empty());

    auto wrongSourceAdmission=make_admission(master,sourceId);
    FrameSource wrongSource(130,70,
        "sha256:5555555555555555555555555555555555555555555555555555555555555555");
    TransactionSink wrongSink;
    status=ldng::write_linear_dng_from_finalized_scientific_source(
        wrongSource,std::make_shared<TestReconstruction>(),wrongSink,
        wrongSourceAdmission,options,badAudit);
    CHECK_TRUE(status.code==ldng::StatusCode::SourceIdentityMismatch);
    CHECK_TRUE(wrongSink.aborted && wrongSink.published.empty());
    return 0;
}

} // namespace

int main() {
    if (test_bound_export_and_abort()!=0) return 1;
    std::cout << "SCIENTIFIC_MASTER_LINEAR_DNG_EXPORT_V0_1_PASS\n"
              << "digest_reverified=1\ntransactional_commit=1\n"
              << "appearance_applied=0\ncamera_to_xyz_applied=0\n";
    return 0;
}
