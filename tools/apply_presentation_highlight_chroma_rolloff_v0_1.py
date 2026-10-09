from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "suite_android" / "app" / "src" / "main" / "cpp" / "photo_export_bridge.cpp"

text = TARGET.read_text(encoding="utf-8")

include_marker = '#include "presentation_highlight_chroma_rolloff_v0_1.h"'
if include_marker not in text:
    anchor = '#include "presentation_gamut_fit_v0_1.h"\n'
    if anchor not in text:
        raise SystemExit("presentation gamut include anchor missing")
    text = text.replace(anchor, anchor + include_marker + "\n", 1)

alias_marker = (
    'namespace presentation_highlight = '
    'truthraw::presentation_highlight_chroma_rolloff::v0_1;'
)
if alias_marker not in text:
    anchor = (
        'namespace presentation_gamut = '
        'truthraw::presentation_gamut_fit::v0_1;\n'
    )
    if anchor not in text:
        raise SystemExit("presentation gamut namespace anchor missing")
    text = text.replace(anchor, anchor + alias_marker + "\n", 1)

call_marker = 'presentation_highlight::apply_near_neutral_rolloff('
if call_marker not in text:
    old = '''                } else {
                    const float mx=std::max(r,std::max(g,b));
                    if(mx>0.92f) {
                        const float shoulder=
                            0.92f+0.08f*(1.0f-std::exp(-3.0f*(mx-0.92f)));
                        const float sc=shoulder/std::max(mx,1e-8f);
                        r*=sc; g*=sc; b*=sc;
                    }
                }

                if(!presentation_gamut::fit_unit_rgb_preserve_luminance(r,g,b)) {
'''
    new = '''                } else {
                    const float mx=std::max(r,std::max(g,b));
                    if(mx>0.92f) {
                        const float shoulder=
                            0.92f+0.08f*(1.0f-std::exp(-3.0f*(mx-0.92f)));
                        const float sc=shoulder/std::max(mx,1e-8f);
                        r*=sc; g*=sc; b*=sc;
                    }
                    if(!presentation_highlight::apply_near_neutral_rolloff(
                            r,g,b,censored)) {
                        return StreamStatus::error(
                            StreamStatusCode::SinkFailed,
                            "full-res presentation highlight chroma roll-off failed");
                    }
                }

                if(!presentation_gamut::fit_unit_rgb_preserve_luminance(r,g,b)) {
'''
    if old not in text:
        raise SystemExit(
            "ADVANCED/PRO presentation shoulder target not found; refusing non-deterministic patch"
        )
    text = text.replace(old, new, 1)

# Keep presentation censoring authority aligned with OutputChannelAuthority v0.84.
# The old staging mask looked only at the CFA sample at the output coordinate.
# Dense RGB reconstruction uses neighbourhood support, so a clipped neighbour can
# invalidate chroma authority even when the centre CFA sample itself is below
# WhiteLevel. This patch carries the already-existing reconstruction halo into
# the sink and builds the same conservative support-based censor mask. It only
# changes the mask/authority signal; staged RGB samples are never rewritten here.
support_marker = 'reconstructionSupportRadius_'
if support_marker not in text:
    old = '''        float noiseSigmaAt2Pct,
        presentation_illuminant_warmth::SourceWhitePoint presentationSourceWhite,
        bool extendedLinearHeadroomInput)
'''
    new = '''        float noiseSigmaAt2Pct,
        presentation_illuminant_warmth::SourceWhitePoint presentationSourceWhite,
        int reconstructionSupportRadius,
        bool extendedLinearHeadroomInput)
'''
    if old not in text:
        raise SystemExit("full-res sink constructor anchor missing")
    text = text.replace(old, new, 1)

    old = '''          noiseSigmaAt2Pct_(noiseSigmaAt2Pct),
          presentationSourceWhite_(presentationSourceWhite),
          extendedLinearHeadroomInput_(extendedLinearHeadroomInput),
'''
    new = '''          noiseSigmaAt2Pct_(noiseSigmaAt2Pct),
          presentationSourceWhite_(presentationSourceWhite),
          reconstructionSupportRadius_(reconstructionSupportRadius),
          extendedLinearHeadroomInput_(extendedLinearHeadroomInput),
'''
    if old not in text:
        raise SystemExit("full-res sink initializer anchor missing")
    text = text.replace(old, new, 1)

    old = '''            !valid_orientation(orientation) || diagnosticsEnabled ||
            !std::isfinite(noiseSigmaAt2Pct_) || noiseSigmaAt2Pct_ < 0.0f) {
'''
    new = '''            !valid_orientation(orientation) || diagnosticsEnabled ||
            !std::isfinite(noiseSigmaAt2Pct_) || noiseSigmaAt2Pct_ < 0.0f ||
            reconstructionSupportRadius_ < 0) {
'''
    if old not in text:
        raise SystemExit("full-res sink begin-frame validation anchor missing")
    text = text.replace(old, new, 1)

    old = '''        TileRect rawRect{r.x0,r.y0,r.x1,r.y1,r.x0,r.y0,r.x1,r.y1};
        std::vector<std::uint16_t> raw(pixels);
        std::vector<float> gain;
        if(source_.metadata().hasGainField) gain.resize(pixels);
        const auto rawStatus=source_.readRawTile(
            rawRect,
            raw.data(),
            raw.size(),
            source_.metadata().hasGainField?gain.data():nullptr,
            source_.metadata().hasGainField?gain.size():0u);
        if(!rawStatus) return rawStatus;

        std::vector<std::uint8_t> maskRow(static_cast<std::size_t>(w));
        for(int y=0;y<h;++y) {
            for(int x=0;x<w;++x) {
                const std::size_t local=
                    static_cast<std::size_t>(y)*static_cast<std::size_t>(w)+
                    static_cast<std::size_t>(x);
                maskRow[static_cast<std::size_t>(x)] =
                    static_cast<float>(raw[local])>=source_.metadata().whiteLevel?1u:0u;
            }
            const std::uint64_t pixelIndex=
                static_cast<std::uint64_t>(r.y0+y)*sourceWidth_+r.x0;
            if(!pwrite_all(
                    fd_,
                    scratchMaskOffset_+pixelIndex,
                    maskRow.data(),
                    maskRow.size())) {
                return StreamStatus::error(StreamStatusCode::SinkFailed,"staged censor-mask write failed");
            }
        }
'''
    new = '''        const int radius=reconstructionSupportRadius_;
        const int rx0=std::max(0,r.x0-radius);
        const int ry0=std::max(0,r.y0-radius);
        const int rx1=std::min(sourceWidth_,r.x1+radius);
        const int ry1=std::min(sourceHeight_,r.y1+radius);
        const int rw=rx1-rx0;
        const int rh=ry1-ry0;
        const std::size_t supportPixels=
            static_cast<std::size_t>(rw)*static_cast<std::size_t>(rh);

        TileRect rawRect{rx0,ry0,rx1,ry1,rx0,ry0,rx1,ry1};
        std::vector<std::uint16_t> raw(supportPixels);
        std::vector<float> gain;
        if(source_.metadata().hasGainField) gain.resize(supportPixels);
        const auto rawStatus=source_.readRawTile(
            rawRect,
            raw.data(),
            raw.size(),
            source_.metadata().hasGainField?gain.data():nullptr,
            source_.metadata().hasGainField?gain.size():0u);
        if(!rawStatus) return rawStatus;

        // Same conservative support rule as OutputChannelAuthority v0.84:
        // any saturated CFA sample in the reconstruction support marks the
        // dense RGB output pixel CENSORED for downstream colour authority.
        // This map never modifies scratch RGB/luminance/detail samples.
        const int prefixW=rw+1;
        std::vector<std::uint32_t> saturatedPrefix(
            static_cast<std::size_t>(prefixW)*static_cast<std::size_t>(rh+1),0u);
        for(int yy=0;yy<rh;++yy) {
            std::uint32_t rowCount=0u;
            for(int xx=0;xx<rw;++xx) {
                const std::size_t ri=
                    static_cast<std::size_t>(yy)*static_cast<std::size_t>(rw)+
                    static_cast<std::size_t>(xx);
                rowCount += static_cast<float>(raw[ri])>=source_.metadata().whiteLevel?1u:0u;
                saturatedPrefix[
                    static_cast<std::size_t>(yy+1)*static_cast<std::size_t>(prefixW)+
                    static_cast<std::size_t>(xx+1)] =
                    saturatedPrefix[
                        static_cast<std::size_t>(yy)*static_cast<std::size_t>(prefixW)+
                        static_cast<std::size_t>(xx+1)] + rowCount;
            }
        }
        const auto saturated_count = [&](int ax0,int ay0,int ax1,int ay1) {
            const int lx0=ax0-rx0;
            const int ly0=ay0-ry0;
            const int lx1=ax1-rx0;
            const int ly1=ay1-ry0;
            return
                saturatedPrefix[
                    static_cast<std::size_t>(ly1)*static_cast<std::size_t>(prefixW)+lx1] -
                saturatedPrefix[
                    static_cast<std::size_t>(ly0)*static_cast<std::size_t>(prefixW)+lx1] -
                saturatedPrefix[
                    static_cast<std::size_t>(ly1)*static_cast<std::size_t>(prefixW)+lx0] +
                saturatedPrefix[
                    static_cast<std::size_t>(ly0)*static_cast<std::size_t>(prefixW)+lx0];
        };

        std::vector<std::uint8_t> maskRow(static_cast<std::size_t>(w));
        for(int y=0;y<h;++y) {
            const int sy=r.y0+y;
            for(int x=0;x<w;++x) {
                const int sx=r.x0+x;
                const int ax0=std::max(0,sx-radius);
                const int ay0=std::max(0,sy-radius);
                const int ax1=std::min(sourceWidth_,sx+radius+1);
                const int ay1=std::min(sourceHeight_,sy+radius+1);
                maskRow[static_cast<std::size_t>(x)] =
                    saturated_count(ax0,ay0,ax1,ay1)>0u?1u:0u;
            }
            const std::uint64_t pixelIndex=
                static_cast<std::uint64_t>(sy)*sourceWidth_+r.x0;
            if(!pwrite_all(
                    fd_,
                    scratchMaskOffset_+pixelIndex,
                    maskRow.data(),
                    maskRow.size())) {
                return StreamStatus::error(StreamStatusCode::SinkFailed,"staged censor-mask write failed");
            }
        }
'''
    if old not in text:
        raise SystemExit("central-only staged censor-mask block missing")
    text = text.replace(old, new, 1)

    old = '''    float noiseSigmaAt2Pct_=0.0f;
    presentation_illuminant_warmth::SourceWhitePoint presentationSourceWhite_{};
    bool extendedLinearHeadroomInput_=false;
'''
    new = '''    float noiseSigmaAt2Pct_=0.0f;
    presentation_illuminant_warmth::SourceWhitePoint presentationSourceWhite_{};
    int reconstructionSupportRadius_=0;
    bool extendedLinearHeadroomInput_=false;
'''
    if old not in text:
        raise SystemExit("full-res sink member anchor missing")
    text = text.replace(old, new, 1)

    old = '''        *source,
        noiseSigma,
        presentationSourceWhite,
        pureExtendedLinearHeadroomCandidate);
'''
    new = '''        *source,
        noiseSigma,
        presentationSourceWhite,
        std::max(0,reconstruction->requiredHalo()),
        pureExtendedLinearHeadroomCandidate);
'''
    if old not in text:
        raise SystemExit("full-res sink construction anchor missing")
    text = text.replace(old, new, 1)

required = (
    include_marker,
    alias_marker,
    call_marker,
    'if(extendedLinearHeadroomInput_) {',
    'presentation_headroom::map_90_100(r,g,b)',
    'int reconstructionSupportRadius_',
    'const int radius=reconstructionSupportRadius_;',
    'const auto saturated_count =',
    'saturated_count(ax0,ay0,ax1,ay1)>0u?1u:0u',
    'std::max(0,reconstruction->requiredHalo())',
)
missing = [token for token in required if token not in text]
if missing:
    raise SystemExit("highlight/support-authority contract missing: " + ", ".join(missing))

# The old centre-only rule must not survive: it can misclassify reconstructed
# RGB chroma authority when a neighbouring CFA phase clips.
old_central_rule = (
    'static_cast<float>(raw[local])>=source_.metadata().whiteLevel?1u:0u;'
)
if old_central_rule in text:
    raise SystemExit("central-only censor mask survived support-authority patch")

# The highlight guard remains in the ADVANCED/PRO `else` branch only. PURE's
# proven 90/100 mapping is not changed by this patch.
pure_pos = text.index('if(extendedLinearHeadroomInput_) {')
map_pos = text.index('presentation_headroom::map_90_100(r,g,b)', pure_pos)
else_pos = text.index('                } else {', map_pos)
call_pos = text.index(call_marker, else_pos)
gamut_pos = text.index('presentation_gamut::fit_unit_rgb_preserve_luminance', call_pos)
if not (pure_pos < map_pos < else_pos < call_pos < gamut_pos):
    raise SystemExit("highlight guard escaped ADVANCED/PRO presentation branch")

TARGET.write_text(text, encoding="utf-8")
print("PRESENTATION_HIGHLIGHT_CHROMA_ROLLOFF_V01_APPLIED")
print("PRESENTATION_CENSOR_SUPPORT_AUTHORITY_V01_APPLIED")
