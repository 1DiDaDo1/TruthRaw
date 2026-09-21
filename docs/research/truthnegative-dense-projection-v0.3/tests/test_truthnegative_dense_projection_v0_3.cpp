#include "truthnegative_dense_projection_v0_3.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace tn = truthraw::truthnegative_dense_projection::v0_3;
namespace fd = truthraw::scientific_master_linear_dng_projection::v0_1;

#define REQUIRE(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)

class ExactHostAccelerator final : public tn::IExactDenseAccelerator {
public:
    bool exactScientificEligible() const noexcept override { return true; }
    const char* backendName() const noexcept override { return "HOST_EXACT_TEST"; }
    std::size_t residentBytesUpperBound() const noexcept override { return 4096u; }

    bool projectPatch(
        const tn::AcceleratorPatchRequest& q,
        const float* sourceRgb,
        std::size_t sourceFloatCount,
        float* targetRgb,
        std::size_t targetFloatCount,
        std::string& error) noexcept override {
        const std::size_t expectedSource =
            static_cast<std::size_t>(q.patchWidth) * q.patchHeight * 3u;
        const std::size_t expectedTarget =
            static_cast<std::size_t>(q.targetWidth) * q.targetHeight * 3u;
        if (!sourceRgb || !targetRgb ||
            sourceFloatCount != expectedSource ||
            targetFloatCount != expectedTarget) {
            error = "host accelerator shape mismatch";
            return false;
        }
        const auto map = [](std::uint32_t t, std::uint32_t limit,
                            std::uint32_t& a, std::uint32_t& b, float& f) {
            const std::uint32_t n=t>>2u;
            int base=0;
            switch(t&3u) {
                case 0u: base=static_cast<int>(n)-1; f=0.625f; break;
                case 1u: base=static_cast<int>(n)-1; f=0.875f; break;
                case 2u: base=static_cast<int>(n); f=0.125f; break;
                default: base=static_cast<int>(n); f=0.375f; break;
            }
            const auto clamp=[limit](int v) {
                if(v<=0) return 0u;
                const auto u=static_cast<std::uint32_t>(v);
                return u>=limit ? limit-1u : u;
            };
            a=clamp(base); b=clamp(base+1);
        };

        for(std::uint32_t oy=0;oy<q.targetHeight;++oy) {
            std::uint32_t y0=0,y1=0; float fy=0;
            map(q.targetOriginY+oy,q.sourceFullHeight,y0,y1,fy);
            if(y0<q.patchOriginY||y1<q.patchOriginY) {
                error="host accelerator y footprint"; return false;
            }
            const auto ly0=y0-q.patchOriginY, ly1=y1-q.patchOriginY;
            for(std::uint32_t ox=0;ox<q.targetWidth;++ox) {
                std::uint32_t x0=0,x1=0; float fx=0;
                map(q.targetOriginX+ox,q.sourceFullWidth,x0,x1,fx);
                if(x0<q.patchOriginX||x1<q.patchOriginX) {
                    error="host accelerator x footprint"; return false;
                }
                const auto lx0=x0-q.patchOriginX, lx1=x1-q.patchOriginX;
                for(std::uint32_t c=0;c<3u;++c) {
                    const auto at=[&](std::uint32_t xx,std::uint32_t yy) {
                        return sourceRgb[
                            (static_cast<std::size_t>(yy)*q.patchWidth+xx)*3u+c];
                    };
                    const float p00=at(lx0,ly0), p10=at(lx1,ly0);
                    const float p01=at(lx0,ly1), p11=at(lx1,ly1);
                    const float top=p00+(p10-p00)*fx;
                    const float bottom=p01+(p11-p01)*fx;
                    targetRgb[
                        (static_cast<std::size_t>(oy)*q.targetWidth+ox)*3u+c] =
                        top+(bottom-top)*fy;
                }
            }
        }
        return true;
    }
};

class FailingHostAccelerator final : public tn::IExactDenseAccelerator {
public:
    bool exactScientificEligible() const noexcept override { return true; }
    const char* backendName() const noexcept override { return "HOST_FAILING_TEST"; }
    std::size_t residentBytesUpperBound() const noexcept override { return 8192u; }
    bool projectPatch(
        const tn::AcceleratorPatchRequest&,
        const float*, std::size_t,
        float*, std::size_t,
        std::string& error) noexcept override {
        error="intentional host accelerator failure";
        return false;
    }
};

class SyntheticSource final : public fd::IScientificMasterTileSource {
public:
    SyntheticSource(std::uint32_t w, std::uint32_t h, bool constant = false)
        : width_(w), height_(h), constant_(constant) {}

    std::size_t residentBytesUpperBound() const noexcept override { return 0u; }

    static std::array<float,3> pixel(
        std::uint32_t x, std::uint32_t y, bool constant) {
        if (constant) return {-0.25f, 0.5f, 1.25f};
        return {
            -0.10f + 0.003f * static_cast<float>(x) +
                0.002f * static_cast<float>(y),
            0.20f + 0.004f * static_cast<float>(x) +
                0.001f * static_cast<float>(y),
            0.95f + 0.006f * static_cast<float>(x) +
                0.003f * static_cast<float>(y),
        };
    }

    fd::Status readCameraNativeTile(
        std::uint32_t x, std::uint32_t y,
        std::uint32_t width, std::uint32_t height,
        float* rgb, std::size_t floatCount) noexcept override {
        if (!rgb || x >= width_ || y >= height_) {
            return fd::Status::error(fd::StatusCode::InvalidArgument, "bad source request");
        }
        const auto expectedW = std::min(fd::kCanonicalTileEdge, width_ - x);
        const auto expectedH = std::min(fd::kCanonicalTileEdge, height_ - y);
        if ((x % fd::kCanonicalTileEdge) != 0u ||
            (y % fd::kCanonicalTileEdge) != 0u ||
            width != expectedW || height != expectedH ||
            floatCount != static_cast<std::size_t>(width) * height * 3u) {
            return fd::Status::error(fd::StatusCode::InvalidArgument, "noncanonical source request");
        }
        for (std::uint32_t yy=0; yy<height; ++yy) {
            for (std::uint32_t xx=0; xx<width; ++xx) {
                const auto p = pixel(x+xx, y+yy, constant_);
                const std::size_t i =
                    (static_cast<std::size_t>(yy)*width + xx)*3u;
                rgb[i+0]=p[0]; rgb[i+1]=p[1]; rgb[i+2]=p[2];
            }
        }
        return fd::Status::ok();
    }

private:
    std::uint32_t width_;
    std::uint32_t height_;
    bool constant_;
};

void test_constant_field_and_geometry() {
    SyntheticSource src(80u, 70u, true);
    tn::DenseProjectionTileSource dense(src, 80u, 70u);
    REQUIRE(dense.valid());
    const auto g = dense.geometry();
    REQUIRE(g.sourceWidth == 80u);
    REQUIRE(g.sourceHeight == 70u);
    REQUIRE(g.targetWidth == 320u);
    REQUIRE(g.targetHeight == 280u);

    std::vector<float> tile(64u*64u*3u);
    auto st = dense.readCameraNativeTile(
        128u, 64u, 64u, 64u, tile.data(), tile.size());
    REQUIRE(st);
    for (std::size_t i=0; i<tile.size(); i+=3u) {
        REQUIRE(std::memcmp(&tile[i+0], &SyntheticSource::pixel(0,0,true)[0], sizeof(float)) == 0);
        REQUIRE(std::memcmp(&tile[i+1], &SyntheticSource::pixel(0,0,true)[1], sizeof(float)) == 0);
        REQUIRE(std::memcmp(&tile[i+2], &SyntheticSource::pixel(0,0,true)[2], sizeof(float)) == 0);
    }
}

void test_convex_interpolation_and_extended_range() {
    constexpr std::uint32_t sw=80u, sh=70u;
    SyntheticSource src(sw, sh, false);
    tn::DenseProjectionTileSource dense(src, sw, sh);
    REQUIRE(dense.valid());

    std::vector<float> tile(64u*64u*3u);
    auto st = dense.readCameraNativeTile(
        0u, 0u, 64u, 64u, tile.data(), tile.size());
    REQUIRE(st);

    float sourceMin[3] = {1e9f,1e9f,1e9f};
    float sourceMax[3] = {-1e9f,-1e9f,-1e9f};
    for (std::uint32_t y=0; y<sh; ++y) {
        for (std::uint32_t x=0; x<sw; ++x) {
            const auto p=SyntheticSource::pixel(x,y,false);
            for(int c=0;c<3;++c) {
                sourceMin[c]=std::min(sourceMin[c],p[c]);
                sourceMax[c]=std::max(sourceMax[c],p[c]);
            }
        }
    }
    bool sawNegative=false, sawOverOne=false;
    for (std::size_t i=0; i<tile.size(); i+=3u) {
        for(int c=0;c<3;++c) {
            REQUIRE(std::isfinite(tile[i+c]));
            REQUIRE(tile[i+c] >= sourceMin[c] - 1e-6f);
            REQUIRE(tile[i+c] <= sourceMax[c] + 1e-6f);
            if(tile[i+c] < 0.0f) sawNegative=true;
            if(tile[i+c] > 1.0f) sawOverOne=true;
        }
    }
    REQUIRE(sawNegative);
    REQUIRE(sawOverOne);
}

void test_projected_identity_deterministic() {
    SyntheticSource srcA(80u,70u,false);
    tn::DenseProjectionTileSource denseA(srcA,80u,70u);
    tn::Result a{};
    auto sa=tn::compute_projected_raster_identity(denseA,a);
    REQUIRE(sa);
    REQUIRE(a.projectedRasterIdentityAvailable);
    REQUIRE(a.projectedPixels == 320ull*280ull);
    REQUIRE(a.measuredTargetClaimCount == 0u);
    REQUIRE(!a.createsNewEvidence);
    REQUIRE(!a.impliesPhysicalSensorGeometry);
    REQUIRE(a.physicalFrameCount == 1u);
    REQUIRE(a.independentEvidenceCount == 1u);
    REQUIRE(a.targetAuthority == "RECONSTRUCTED_DENSE_SUPPORT");
    REQUIRE(a.negativeComponentCount > 0u);
    REQUIRE(a.overOneComponentCount > 0u);

    SyntheticSource srcB(80u,70u,false);
    tn::DenseProjectionTileSource denseB(srcB,80u,70u);
    tn::Result b{};
    auto sb=tn::compute_projected_raster_identity(denseB,b);
    REQUIRE(sb);
    REQUIRE(a.projectedRasterSha256 == b.projectedRasterSha256);
    REQUIRE(tn::authority_manifest(a).find("measured_target_claim_count=0") != std::string::npos);
    REQUIRE(tn::authority_manifest(a).find("creates_new_evidence=0") != std::string::npos);
}

void test_accelerator_identity_and_failover() {
    SyntheticSource cpuSource(80u,70u,false);
    tn::DenseProjectionTileSource cpu(cpuSource,80u,70u);
    tn::Result cpuResult{};
    REQUIRE(tn::compute_projected_raster_identity(cpu,cpuResult));

    ExactHostAccelerator exactAccelerator;
    SyntheticSource exactSource(80u,70u,false);
    tn::DenseProjectionTileSource exact(
        exactSource,80u,70u,&exactAccelerator);
    tn::Result exactResult{};
    REQUIRE(tn::compute_projected_raster_identity(exact,exactResult));
    REQUIRE(exactResult.acceleratorEligible);
    REQUIRE(exactResult.acceleratorUsed);
    REQUIRE(exactResult.acceleratorBackend == "HOST_EXACT_TEST");
    REQUIRE(exactResult.projectedRasterSha256 == cpuResult.projectedRasterSha256);

    FailingHostAccelerator failingAccelerator;
    SyntheticSource fallbackSource(80u,70u,false);
    tn::DenseProjectionTileSource fallback(
        fallbackSource,80u,70u,&failingAccelerator);
    tn::Result fallbackResult{};
    REQUIRE(tn::compute_projected_raster_identity(fallback,fallbackResult));
    REQUIRE(!fallbackResult.acceleratorUsed);
    REQUIRE(fallbackResult.acceleratorBackend == "CPU_REFERENCE");
    REQUIRE(fallbackResult.projectedRasterSha256 == cpuResult.projectedRasterSha256);
    REQUIRE(fallback.residentBytesUpperBound() >= 8192u);
}

int main() {
    test_constant_field_and_geometry();
    test_convex_interpolation_and_extended_range();
    test_projected_identity_deterministic();
    test_accelerator_identity_and_failover();
    std::cout << "TruthNegativeDenseProjection/0.3 PASS\n";
    std::cout << "method=" << tn::kMethodId << "\n";
    std::cout << "authority=" << tn::kAuthority << "\n";
    std::cout << "measured_target_claim_count=0\n";
    return 0;
}
