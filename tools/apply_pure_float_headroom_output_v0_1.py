#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "suite_android" / "app" / "src" / "main" / "cpp" / "photo_export_bridge.cpp"

text = TARGET.read_text(encoding="utf-8")

if "process_pure_extended_linear_headroom_v0_1" in text:
    print("pure float-headroom output v0.1 already applied")
    raise SystemExit(0)

replacements = [
    (
        '#include "adaptive_detail_v47j_adapter.h"\n',
        '#include "adaptive_detail_v47j_adapter.h"\n'
        '#include "advanced_render_edit_tile_source_v0_1.h"\n',
        "extended-linear source include",
    ),
    (
        '#include "presentation_gamut_fit_v0_1.h"\n',
        '#include "presentation_gamut_fit_v0_1.h"\n'
        '#include "presentation_headroom_map_v0_1.h"\n',
        "presentation headroom include",
    ),
    (
        'namespace presentation_gamut = truthraw::presentation_gamut_fit::v0_1;\n',
        'namespace presentation_gamut = truthraw::presentation_gamut_fit::v0_1;\n'
        'namespace presentation_headroom = truthraw::presentation_headroom_map::v0_1;\n'
        'namespace render_edit = truthraw::advanced_render_edit::v0_1;\n',
        "output namespace aliases",
    ),
    (
        '        truthraw::streaming_v0_1::IRawTileSource& source,\n'
        '        float noiseSigmaAt2Pct)\n'
        '        : fd_(fd),\n',
        '        truthraw::streaming_v0_1::IRawTileSource& source,\n'
        '        float noiseSigmaAt2Pct,\n'
        '        bool extendedLinearHeadroomInput)\n'
        '        : fd_(fd),\n',
        "sink constructor argument",
    ),
    (
        '          source_(source),\n'
        '          noiseSigmaAt2Pct_(noiseSigmaAt2Pct),\n',
        '          source_(source),\n'
        '          noiseSigmaAt2Pct_(noiseSigmaAt2Pct),\n'
        '          extendedLinearHeadroomInput_(extendedLinearHeadroomInput),\n',
        "sink constructor member init",
    ),
    (
        '                const float mx=std::max(r,std::max(g,b));\n'
        '                if(mx>0.92f) {\n'
        '                    const float shoulder=\n'
        '                        0.92f+0.08f*(1.0f-std::exp(-3.0f*(mx-0.92f)));\n'
        '                    const float sc=shoulder/std::max(mx,1e-8f);\n'
        '                    r*=sc; g*=sc; b*=sc;\n'
        '                }\n\n'
        '                if(!presentation_gamut::fit_unit_rgb_preserve_luminance(r,g,b)) {\n',
        '                if(extendedLinearHeadroomInput_) {\n'
        '                    // The PURE headroom candidate arrives here as the existing\n'
        '                    // extended-linear Float32 derivative, so this is the one\n'
        '                    // deliberate scene/display boundary. 90/100 is the most\n'
        '                    // conservative previously measured non-baseline headroom\n'
        '                    // candidate. It is APPEARANCE only.\n'
        '                    if(!presentation_headroom::map_90_100(r,g,b)) {\n'
        '                        return StreamStatus::error(\n'
        '                            StreamStatusCode::SinkFailed,\n'
        '                            "PURE extended-linear headroom mapping failed");\n'
        '                    }\n'
        '                } else {\n'
        '                    const float mx=std::max(r,std::max(g,b));\n'
        '                    if(mx>0.92f) {\n'
        '                        const float shoulder=\n'
        '                            0.92f+0.08f*(1.0f-std::exp(-3.0f*(mx-0.92f)));\n'
        '                        const float sc=shoulder/std::max(mx,1e-8f);\n'
        '                        r*=sc; g*=sc; b*=sc;\n'
        '                    }\n'
        '                }\n\n'
        '                if(!presentation_gamut::fit_unit_rgb_preserve_luminance(r,g,b)) {\n',
        "single downstream headroom boundary",
    ),
    (
        '    float noiseSigmaAt2Pct_=0.0f;\n'
        '    float detailMix_=0.0f;\n',
        '    float noiseSigmaAt2Pct_=0.0f;\n'
        '    bool extendedLinearHeadroomInput_=false;\n'
        '    float detailMix_=0.0f;\n',
        "sink headroom member",
    ),
    (
        'StreamingOptions photo_options(std::size_t memoryBudgetBytes, jint flags) {\n',
        'StreamStatus process_pure_extended_linear_headroom_v0_1(\n'
        '    truthraw::streaming_v0_1::IRawTileSource& source,\n'
        '    truthraw::IReconstructionBackend& reconstruction,\n'
        '    const std::array<float,9>& cameraToXyzD50,\n'
        '    FullResNv21Sink& sink,\n'
        '    StreamingResult& stream) {\n'
        '    const auto& m=source.metadata();\n'
        '    if(m.width<=0 || m.height<=0 || (m.width&1)!=0 || (m.height&1)!=0) {\n'
        '        return StreamStatus::error(\n'
        '            StreamStatusCode::InvalidArgument,\n'
        '            "PURE extended-linear headroom route requires even geometry");\n'
        '    }\n\n'
        '    // PURE carries no adjustable Appearance flags here. A default exposure\n'
        '    // plan is therefore intentionally inert in FullResNv21Sink. The source\n'
        '    // below replays the already admitted reconstruction directly into\n'
        '    // extended linear-sRGB and preserves finite values above 1.0.\n'
        '    truthraw::ExposurePlan exposure{};\n'
        '    auto status=sink.beginFrame(\n'
        '        m.width,m.height,m.orientation,exposure,false,false);\n'
        '    if(!status) return status;\n\n'
        '    render_edit::ExtendedLinearSrgbTileSource linearSource(\n'
        '        source,reconstruction,cameraToXyzD50,0u,exposure);\n'
        '    if(linearSource.residentBytesUpperBound()==0u) {\n'
        '        return StreamStatus::error(\n'
        '            StreamStatusCode::BackendFailed,\n'
        '            "PURE extended-linear source failed to initialize");\n'
        '    }\n\n'
        '    std::vector<float> rgb;\n'
        '    std::vector<float> zeroGain;\n'
        '    std::size_t tileCount=0u;\n'
        '    for(int y0=0;y0<m.height;y0+=kTileCore) {\n'
        '        const int y1=std::min(m.height,y0+kTileCore);\n'
        '        for(int x0=0;x0<m.width;x0+=kTileCore) {\n'
        '            const int x1=std::min(m.width,x0+kTileCore);\n'
        '            const int w=x1-x0;\n'
        '            const int h=y1-y0;\n'
        '            rgb.resize(\n'
        '                3u*static_cast<std::size_t>(w)*static_cast<std::size_t>(h));\n'
        '            const auto read=linearSource.readCameraNativeTile(\n'
        '                static_cast<std::uint32_t>(x0),\n'
        '                static_cast<std::uint32_t>(y0),\n'
        '                static_cast<std::uint32_t>(w),\n'
        '                static_cast<std::uint32_t>(h),\n'
        '                rgb.data(),rgb.size());\n'
        '            if(!read) {\n'
        '                return StreamStatus::error(\n'
        '                    StreamStatusCode::BackendFailed,\n'
        '                    "PURE extended-linear tile replay failed: "+read.message);\n'
        '            }\n\n'
        '            const TileRect core{x0,y0,x1,y1,x0,y0,x1,y1};\n'
        '            status=sink.writeSdrTile(core,rgb.data(),rgb.size());\n'
        '            if(!status) return status;\n\n'
        '            const HalfStateRect half{x0/2,y0/2,x1/2,y1/2};\n'
        '            const std::size_t halfCount=\n'
        '                static_cast<std::size_t>(half.x1-half.x0)*\n'
        '                static_cast<std::size_t>(half.y1-half.y0);\n'
        '            zeroGain.assign(halfCount,0.0f);\n'
        '            status=sink.writeHalfLogGainBlock(\n'
        '                half,zeroGain.data(),zeroGain.size());\n'
        '            if(!status) return status;\n'
        '            ++tileCount;\n'
        '        }\n'
        '    }\n\n'
        '    status=sink.finishFrame();\n'
        '    if(!status) return status;\n\n'
        '    stream={};\n'
        '    stream.status=StreamStatus::ok();\n'
        '    stream.width=m.width;\n'
        '    stream.height=m.height;\n'
        '    stream.orientation=m.orientation;\n'
        '    stream.exposure=exposure;\n'
        '    stream.tilesProcessedPass2=tileCount;\n'
        '    stream.memory.sourceResidentUpperBound=source.residentBytesUpperBound();\n'
        '    stream.memory.sinkResidentUpperBound=sink.residentBytesUpperBound();\n'
        '    stream.provenance.physicalFrameCount=1u;\n'
        '    stream.provenance.independentEvidenceCount=1u;\n'
        '    stream.provenance.scientificMasterModifiedByAppearance=false;\n'
        '    stream.provenance.counterfactualObservationCreated=false;\n'
        '    stream.provenance.reconstructionBackend=reconstruction.name();\n'
        '    stream.provenance.appearanceBackend=\n'
        '        "PURE_EXTENDED_LINEAR_FLOAT32_HEADROOM_90_100_APPEARANCE_ONLY_V0_1";\n'
        '    return StreamStatus::ok();\n'
        '}\n\n'
        'StreamingOptions photo_options(std::size_t memoryBudgetBytes, jint flags) {\n',
        "PURE extended-linear processor",
    ),
    (
        '    FullResNv21Sink sink(\n'
        '        static_cast<int>(outputFd),\n'
        '        flags,\n'
        '        userQuarterTurns,\n'
        '        *source,\n'
        '        noiseSigma);\n'
        '    StreamingTruthRawProcessor processor(reconstruction,appearance);\n'
        '    StreamingResult stream;\n'
        '    const auto processed=processor.process(\n'
        '        *source,sink,photo_options(static_cast<std::size_t>(maxLogicalResidentBytes),flags),stream);\n'
        '    if(!processed) { (void)::ftruncate(outputFd,0); return packet(env,stream_status(processed)); }\n',
        '    const bool pureExtendedLinearHeadroomCandidate=(flags==0);\n'
        '    FullResNv21Sink sink(\n'
        '        static_cast<int>(outputFd),\n'
        '        flags,\n'
        '        userQuarterTurns,\n'
        '        *source,\n'
        '        noiseSigma,\n'
        '        pureExtendedLinearHeadroomCandidate);\n'
        '    StreamingResult stream;\n'
        '    StreamStatus processed;\n'
        '    if(pureExtendedLinearHeadroomCandidate) {\n'
        '        processed=process_pure_extended_linear_headroom_v0_1(\n'
        '            *source,*reconstruction,produced.color.cameraToXyzD50,sink,stream);\n'
        '    } else {\n'
        '        StreamingTruthRawProcessor processor(reconstruction,appearance);\n'
        '        processed=processor.process(\n'
        '            *source,sink,\n'
        '            photo_options(static_cast<std::size_t>(maxLogicalResidentBytes),flags),\n'
        '            stream);\n'
        '    }\n'
        '    if(!processed) { (void)::ftruncate(outputFd,0); return packet(env,stream_status(processed)); }\n',
        "PURE runtime route",
    ),
]

for old, new, label in replacements:
    count = text.count(old)
    if count != 1:
        print(f"FAIL: {label}: expected exactly one anchor, found {count}", file=sys.stderr)
        raise SystemExit(2)
    text = text.replace(old, new, 1)

TARGET.write_text(text, encoding="utf-8")
print("PURE_FLOAT_HEADROOM_OUTPUT_V0_1_PATCH_APPLIED")
