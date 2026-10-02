#include "truthnegative_pipeline_bridge_common.h"

#include "tile_native_dng_source_v0_1.h"

#include <chrono>
#include <mutex>
#include <span>
#include <utility>
#include <unistd.h>

namespace truthraw::android_truthnegative_pipeline::v0_1 {
namespace {

using SteadyClock = std::chrono::steady_clock;

double elapsed_ms(
    SteadyClock::time_point started,
    SteadyClock::time_point finished) noexcept {
    return std::chrono::duration<double,std::milli>(
        finished-started).count();
}

Status fail(int code, std::string message) {
    return {code, std::move(message)};
}

std::mutex gSharedCacheMutex;
std::shared_ptr<Context> gSharedContext;
std::size_t gSharedMaxSourceResidentBytes = 0u;
std::size_t gSharedMaxLogicalResidentBytes = 0u;

std::shared_ptr<int> duplicate_owned_fd(int sourceFd) {
    const int duplicated = ::dup(sourceFd);
    if (duplicated < 0) return {};
    return std::shared_ptr<int>(
        new int(duplicated),
        [](int* fd) {
            if (fd != nullptr) {
                if (*fd >= 0) ::close(*fd);
                delete fd;
            }
        });
}

bool same_source_seal(
    const scientific_preview_binding_v0_1::SourceSeal& a,
    const scientific_preview_binding_v0_1::SourceSeal& b) noexcept {
    return a.sha256 == b.sha256 &&
        a.byteLength == b.byteLength;
}

class AuthorityFieldFusionObserver final
    : public scientific_master_streaming_binding::v0_2::
          ICanonicalTileObserver {
public:
    explicit AuthorityFieldFusionObserver(
        const DngMetadata& metadata) noexcept
        : cfa_(metadata.cfa),
          whiteLevel_(metadata.whiteLevel),
          accumulator_(
              metadata.width > 0
                  ? static_cast<std::uint32_t>(metadata.width)
                  : 0u,
              metadata.height > 0
                  ? static_cast<std::uint32_t>(metadata.height)
                  : 0u) {}

    std::size_t residentBytesUpperBound() const noexcept override {
        return accumulator_.residentBytesUpperBound();
    }

    bool observeCanonicalTile(
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t width,
        std::uint32_t height,
        const std::uint16_t* rawCore,
        std::size_t rawCount,
        const float* cameraNativeRgb,
        std::size_t floatCount) noexcept override {
        if (!rawCore || !cameraNativeRgb) return false;
        const auto started = SteadyClock::now();
        const bool ok = accumulator_.appendSourceTile(
            cfa_,
            x,
            y,
            width,
            height,
            std::span<const std::uint16_t>(
                rawCore,
                rawCount),
            whiteLevel_,
            std::span<const float>(
                cameraNativeRgb,
                floatCount));
        const auto finished = SteadyClock::now();
        if (ok) {
            directRecordStreamMs_ +=
                elapsed_ms(started, finished);
            ++tileCount_;
            recordCount_ +=
                static_cast<std::uint64_t>(rawCount) * 3u;
        }
        return ok;
    }

    bool finalize(
        truthnegative_continuous::v0_5::
            AuthorityFieldSummary& out) noexcept {
        return accumulator_.finalize(out);
    }

    double directRecordStreamMs() const noexcept {
        return directRecordStreamMs_;
    }

    std::uint64_t tileCount() const noexcept {
        return tileCount_;
    }

    std::uint64_t recordCount() const noexcept {
        return recordCount_;
    }

    std::size_t accumulatorResidentBytesUpperBound() const noexcept {
        return accumulator_.residentBytesUpperBound();
    }

    std::uint64_t directByteRecordCount() const noexcept {
        return accumulator_.directByteRecordCount();
    }

    std::uint64_t genericFallbackRecordCount() const noexcept {
        return accumulator_.genericFallbackRecordCount();
    }

private:
    CfaPattern cfa_ = CfaPattern::BGGR;
    float whiteLevel_ = 0.0f;
    truthnegative_continuous::v0_5::
        AuthorityFieldAccumulator accumulator_;
    double directRecordStreamMs_ = 0.0;
    std::uint64_t tileCount_ = 0u;
    std::uint64_t recordCount_ = 0u;
};

}  // namespace

Status prepare(
    int sourceFd,
    std::size_t maxSourceResidentBytes,
    std::size_t maxLogicalResidentBytes,
    Context& out,
    PreparationTiming* timing) noexcept {
    out = Context{};
    if (timing) *timing = PreparationTiming{};
    const auto totalStarted = SteadyClock::now();
    try {
        if (sourceFd < 0 ||
            maxSourceResidentBytes == 0u ||
            maxLogicalResidentBytes == 0u) {
            return fail(-1, "invalid pipeline arguments");
        }

        const auto duplicateStarted = SteadyClock::now();
        out.ownedSourceFd = duplicate_owned_fd(sourceFd);
        if (!out.ownedSourceFd) {
            return fail(-8, "source fd duplication failed");
        }
        out.bytes =
            std::make_shared<tile_dng_v0_1::PosixFdByteSource>(
                *out.ownedSourceFd);
        const auto duplicateFinished = SteadyClock::now();
        if (timing) {
            timing->duplicateAndByteSourceMs =
                elapsed_ms(duplicateStarted, duplicateFinished);
        }

        const auto sealStarted = SteadyClock::now();
        const auto sealed =
            scientific_preview_binding_v0_1::seal_source_sha256(
                *out.bytes, out.sourceSeal);
        if (!sealed) {
            return fail(
                2000 + static_cast<int>(sealed.code),
                sealed.message);
        }
        const auto sealFinished = SteadyClock::now();
        if (timing) timing->sealSourceMs = elapsed_ms(sealStarted, sealFinished);

        const auto colorStarted = SteadyClock::now();
        const auto color =
            dng_color_binding_producer_v0_2::
                produce_source_metadata_color_binding(
                    *out.bytes, out.sourceSeal, out.produced);
        if (!color) {
            return fail(
                2100 + static_cast<int>(color.code),
                color.message);
        }
        const auto colorFinished = SteadyClock::now();
        if (timing) timing->colorBindingMs = elapsed_ms(colorStarted, colorFinished);

        const auto prepareColorStarted = SteadyClock::now();
        const auto preparedStatus =
            scientific_preview_binding_v0_2::
                prepare_scientific_color_source(
                    out.sourceSeal,
                    out.produced.color,
                    out.prepared);
        if (!preparedStatus) {
            return fail(
                2000 + static_cast<int>(preparedStatus.code),
                preparedStatus.message);
        }
        if (!out.prepared.mainHouseComputeAllowed ||
            out.prepared.physicalFrameCount != 1u ||
            out.prepared.independentEvidenceCount != 1u) {
            return fail(-2, "prepared source evidence invariant failed");
        }
        const auto prepareColorFinished = SteadyClock::now();
        if (timing) {
            timing->prepareColorSourceMs =
                elapsed_ms(prepareColorStarted, prepareColorFinished);
        }

        const auto preStarted = SteadyClock::now();
        const auto pre =
            scientific_preview_binding_v0_1::reverify_source_sha256(
                *out.bytes, out.sourceSeal);
        if (!pre) {
            return fail(
                2000 + static_cast<int>(pre.code),
                pre.message);
        }
        const auto preFinished = SteadyClock::now();
        if (timing) timing->preOpenReverifyMs = elapsed_ms(preStarted, preFinished);

        auto options = out.prepared.tileNativeOptions;
        options.maxResidentBytes = maxSourceResidentBytes;
        const auto openStarted = SteadyClock::now();
        const auto opened =
            android_raw_adapter_bridge::v0_1::openDngViaAdapter(
                out.bytes,
                out.sourceSeal,
                options,
                out.openedSource);
        if (!opened) {
            return fail(
                7000 + static_cast<int>(opened.code),
                opened.message);
        }
        const auto openFinished = SteadyClock::now();
        if (timing) timing->openDngAdapterMs = elapsed_ms(openStarted, openFinished);

        const auto& sourceMetadata =
            out.openedSource.source->metadata();
        if (sourceMetadata.width <= 0 ||
            sourceMetadata.height <= 0) {
            return fail(-4, "invalid source geometry");
        }

        AuthorityFieldFusionObserver authorityObserver(
            sourceMetadata);

        out.reconstruction =
            std::make_shared<
                scientific_master_f64_reconstruction_v0_1::
                    ResearchEdgeAwareMeasuredPreservingReconstructionF64>();

        scientific_master_streaming_binding::v0_2::Options scienceOptions;
        scienceOptions.memoryBudgetBytes = maxLogicalResidentBytes;
        const auto scienceStarted = SteadyClock::now();
        const auto scienceStatus =
            scientific_master_streaming_binding::v0_2::
                bind_scientific_master_streaming_observed(
                    *out.openedSource.source,
                    *out.reconstruction,
                    scienceOptions,
                    authorityObserver,
                    out.scientific);
        if (!scienceStatus) {
            return fail(
                8000 + static_cast<int>(scienceStatus.code),
                scienceStatus.message);
        }
        const auto scienceFinished = SteadyClock::now();
        if (timing) {
            timing->bindScientificMasterMs =
                elapsed_ms(scienceStarted, scienceFinished);
            timing->authorityDirectRecordStreamingActive = true;
            timing->authorityTemporaryRecordVectorUsed = false;
            timing->authorityDirectByteEncodingActive =
                authorityObserver.directByteRecordCount() > 0u;
            timing->authorityGenericRecordValidationBypassed =
                authorityObserver.genericFallbackRecordCount() == 0u;
            timing->authorityCanonicalRecordBytes =
                open_scene_field::v0_85::
                    kCanonicalAuthorityRecordBytes;
            timing->authorityHashBatchRecordCapacity =
                truthnegative_continuous::v0_5::
                    kAuthorityDirectHashBatchRecordCount;
            timing->authorityHashBatchBytes =
                truthnegative_continuous::v0_5::
                    kAuthorityDirectHashBatchBytes;
            timing->authorityDirectByteRecordCount =
                authorityObserver.directByteRecordCount();
            timing->authorityGenericFallbackRecordCount =
                authorityObserver.genericFallbackRecordCount();
            timing->authorityDirectRecordStreamMs =
                authorityObserver.directRecordStreamMs();
            timing->authorityDirectRecordStreamTileCount =
                authorityObserver.tileCount();
            timing->authorityDirectRecordStreamRecordCount =
                authorityObserver.recordCount();
            timing->authorityAccumulatorResidentBytesUpperBound =
                authorityObserver.accumulatorResidentBytesUpperBound();
        }

        technical_backplane_phase2::v0_1::Phase2Input phaseInput;
        phaseInput.prepared = out.prepared;
        phaseInput.scientificMasterHash =
            out.scientific.scientificMasterHash;
        phaseInput.zeroLineGauge = out.scientific.zeroLineGauge;
        phaseInput.sceneBinding = out.scientific.sceneBinding;
        phaseInput.roomStatus.fill(
            technical_backplane::v0_1::RoomStatus::ResearchOnly);
        phaseInput.claimStatus =
            technical_backplane::v0_1::ClaimStatus::Candidate;

        const auto phaseStarted = SteadyClock::now();
        const auto phaseStatus =
            technical_backplane_phase2::v0_1::finalize_phase2(
                phaseInput, out.phase2);
        if (!phaseStatus) {
            return fail(
                9000 + static_cast<int>(phaseStatus.code),
                phaseStatus.message);
        }
        const auto phaseFinished = SteadyClock::now();
        if (timing) timing->finalizePhase2Ms = elapsed_ms(phaseStarted, phaseFinished);

        if (out.phase2.admission.claimScope ==
                scientific_preview_binding_v0_1::ColorClaimScope::None ||
            out.phase2.backplane.sourceEvidenceHash !=
                out.sourceSeal.sha256 ||
            out.phase2.backplane.scientificMasterHash !=
                out.scientific.scientificMasterHash ||
            out.phase2.backplane.forbiddenFlags != 0u ||
            out.phase2.backplane.physicalFrameCount != 1u ||
            out.phase2.backplane.independentEvidenceCount != 1u ||
            out.scientific.physicalFrameCount != 1u ||
            out.scientific.independentEvidenceCount != 1u) {
            return fail(-3, "phase2/master lineage invariant failed");
        }

        out.width =
            static_cast<std::uint32_t>(sourceMetadata.width);
        out.height =
            static_cast<std::uint32_t>(sourceMetadata.height);

        out.masterSource =
            std::make_unique<
                scientific_master_linear_dng_projection::v0_1::
                    StreamingScientificMasterTileSource>(
                        *out.openedSource.source,
                        *out.reconstruction);
        out.fieldSource =
            std::make_unique<
                truthnegative_dense_local_field_adapter::v0_4::
                    SourceFieldAdapter>(
                        *out.openedSource.source,
                        *out.masterSource);

        const auto authorityStarted = SteadyClock::now();
        if (!authorityObserver.finalize(
                out.authorityField) ||
            out.authorityField.createsNewEvidence ||
            out.authorityField.scientificWritebackAllowed) {
            return fail(-5, "fused authority field binding failed");
        }
        const auto authorityFinished = SteadyClock::now();
        if (timing) {
            const auto authorityFinalizeMs =
                elapsed_ms(
                    authorityStarted,
                    authorityFinished);
            timing->summarizeAuthorityFieldMs =
                authorityFinalizeMs;
            timing->authorityFusedFinalizeMs =
                authorityFinalizeMs;
            timing->authorityFusedIntoScientificMasterPass =
                true;
            timing->authorityReplayPassPerformed =
                false;
        }

        truthnegative_continuous::v0_5::StateInput stateInput{};
        stateInput.sourceEvidenceSha256 = out.sourceSeal.sha256;
        stateInput.scientificMasterSha256 =
            out.scientific.scientificMasterHash;
        stateInput.authorityFieldSha256 =
            out.authorityField.contentSha256;
        stateInput.width = out.width;
        stateInput.height = out.height;
        stateInput.reconstructionBackendId =
            out.reconstruction->name();
        stateInput.colourBindingId =
            out.produced.color.bindingId;
        stateInput.physicalFrameCount = 1u;
        stateInput.independentEvidenceCount = 1u;

        const auto tnStarted = SteadyClock::now();
        if (!truthnegative_continuous::v0_5::finalizeState(
                stateInput, out.truthNegativeState) ||
            !out.truthNegativeState.isRasterIndependent ||
            out.truthNegativeState.createsNewEvidence ||
            out.truthNegativeState.scientificWritebackAllowed) {
            return fail(-6, "TruthNegative continuous parent state failed");
        }
        const auto tnFinished = SteadyClock::now();
        if (timing) {
            timing->finalizeTruthNegativeMs =
                elapsed_ms(tnStarted, tnFinished);
        }

        const std::string sourceHex =
            sha256_v0_69::hex(out.sourceSeal.sha256);
        drawnegative::v0_1::Input drawNegativeInput{};
        drawNegativeInput.truthNegativeState =
            out.truthNegativeState;
        drawNegativeInput.observationId =
            std::string("DRAW_OBS_") + sourceHex;
        drawNegativeInput.scaleGaugeId =
            std::string("DRAW_SOURCE_LOCAL_GAUGE_") + sourceHex;
        drawNegativeInput.sharedFreeWorldGaugeId.clear();
        drawNegativeInput.gaugeRelation =
            drawnegative::v0_1::GaugeRelation::SourceLocalOnly;
        drawNegativeInput.canonicalStorage =
            drawnegative::v0_1::CanonicalStorage::Float32Validated;
        drawNegativeInput.truthRangeCoordinateFamilyDeclared = true;
        drawNegativeInput.perSampleTruthRangeMaterialized = false;

        const auto drawStarted = SteadyClock::now();
        if (!drawnegative::v0_1::finalize(
                drawNegativeInput, out.drawNegativeState) ||
            !out.drawNegativeState.isRasterIndependent ||
            !out.drawNegativeState.isPerObservationLineage ||
            out.drawNegativeState.commonGaugeAdmitted ||
            out.drawNegativeState.crossObservationRadiometricFusionAllowed ||
            out.drawNegativeState.createsNewEvidence ||
            out.drawNegativeState.scientificWritebackAllowed ||
            out.drawNegativeState.parentTruthNegativeStateSha256 !=
                out.truthNegativeState.stateSha256) {
            return fail(-7, "D.RAWnegative v0.1 state failed");
        }
        const auto drawFinished = SteadyClock::now();
        if (timing) {
            timing->finalizeDrawNegativeMs =
                elapsed_ms(drawStarted, drawFinished);
            timing->totalMs = elapsed_ms(totalStarted, drawFinished);
        }

        return {};
    } catch (...) {
        out = Context{};
        return fail(-99, "unexpected pipeline exception");
    }
}

Status acquireShared(
    int sourceFd,
    std::size_t maxSourceResidentBytes,
    std::size_t maxLogicalResidentBytes,
    std::shared_ptr<Context>& out,
    bool& cacheHit,
    SharedAcquireTiming* timing) noexcept {
    out.reset();
    cacheHit = false;
    if (timing) *timing = SharedAcquireTiming{};

    try {
        if (sourceFd < 0 ||
            maxSourceResidentBytes == 0u ||
            maxLogicalResidentBytes == 0u) {
            return fail(-1, "invalid shared pipeline arguments");
        }

        tile_dng_v0_1::PosixFdByteSource probeBytes(sourceFd);
        scientific_preview_binding_v0_1::SourceSeal probeSeal{};
        const auto probeStarted = SteadyClock::now();
        const auto sealed =
            scientific_preview_binding_v0_1::seal_source_sha256(
                probeBytes,
                probeSeal);
        if (!sealed) {
            return fail(
                2000 + static_cast<int>(sealed.code),
                sealed.message);
        }
        const auto probeFinished = SteadyClock::now();
        if (timing) timing->probeSealMs = elapsed_ms(probeStarted, probeFinished);

        const auto lookupStarted = SteadyClock::now();
        {
            std::lock_guard<std::mutex> guard(gSharedCacheMutex);
            if (gSharedContext &&
                gSharedMaxSourceResidentBytes == maxSourceResidentBytes &&
                gSharedMaxLogicalResidentBytes == maxLogicalResidentBytes &&
                same_source_seal(
                    gSharedContext->sourceSeal,
                    probeSeal)) {
                out = gSharedContext;
                cacheHit = true;
                if (timing) {
                    timing->cacheHit = true;
                    timing->cacheLookupMs =
                        elapsed_ms(lookupStarted, SteadyClock::now());
                }
                return {};
            }

            // Do not retain the previous RAW's heavy prepared context while
            // constructing the next one. This bounds peak memory on Android.
            gSharedContext.reset();
            gSharedMaxSourceResidentBytes = 0u;
            gSharedMaxLogicalResidentBytes = 0u;
        }
        if (timing) {
            timing->cacheLookupMs =
                elapsed_ms(lookupStarted, SteadyClock::now());
        }

        auto fresh = std::make_shared<Context>();
        PreparationTiming preparationTiming{};
        const auto prepareStarted = SteadyClock::now();
        const auto prepared =
            prepare(
                sourceFd,
                maxSourceResidentBytes,
                maxLogicalResidentBytes,
                *fresh,
                &preparationTiming);
        const auto prepareFinished = SteadyClock::now();
        if (timing) {
            timing->prepareTotalMs =
                elapsed_ms(prepareStarted, prepareFinished);
            timing->preparation = preparationTiming;
        }
        if (!prepared) {
            return prepared;
        }
        if (!same_source_seal(fresh->sourceSeal, probeSeal)) {
            return fail(-9, "source changed while preparing shared context");
        }

        {
            std::lock_guard<std::mutex> guard(gSharedCacheMutex);
            gSharedContext = fresh;
            gSharedMaxSourceResidentBytes = maxSourceResidentBytes;
            gSharedMaxLogicalResidentBytes = maxLogicalResidentBytes;
        }
        out = std::move(fresh);
        cacheHit = false;
        if (timing) timing->cacheHit = false;
        return {};
    } catch (...) {
        out.reset();
        cacheHit = false;
        return fail(-99, "unexpected shared pipeline exception");
    }
}

void clearSharedCache() noexcept {
    std::lock_guard<std::mutex> guard(gSharedCacheMutex);
    gSharedContext.reset();
    gSharedMaxSourceResidentBytes = 0u;
    gSharedMaxLogicalResidentBytes = 0u;
}

bool reverify(const Context& context) noexcept {
    if (!context.bytes) return false;
    const auto status =
        scientific_preview_binding_v0_1::reverify_source_sha256(
            *context.bytes, context.sourceSeal);
    return static_cast<bool>(status);
}

}  // namespace truthraw::android_truthnegative_pipeline::v0_1
