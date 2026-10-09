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

# Carry the already-existing reconstruction halo into the presentation sink.
# This radius is used ONLY to derive highlight chroma authority. It must not
# broaden the pre-existing centre-sample mask used by restoration, HDR, or the
# Natural Light local-field path.
if 'int reconstructionSupportRadius_' not in text:
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

# Restore/retain the historical centre-sample censor mask in writeSdrTile.
# A previous candidate broadened this shared mask to reconstruction support,
# which would also alter restoration/HDR/local-field decisions. That is too
# broad for a highlight-colour fix, so normalize it back to centre-only here.
central_staging = '''        TileRect rawRect{r.x0,r.y0,r.x1,r.y1,r.x0,r.y0,r.x1,r.y1};
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

support_staging = '''        const int radius=reconstructionSupportRadius_;
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

if central_staging not in text:
    if support_staging not in text:
        raise SystemExit("neither centre-only nor previous support-based staging mask found")
    text = text.replace(support_staging, central_staging, 1)

# Ensure finalizeCoreTile has enough staged-mask halo to evaluate the same
# reconstruction support used by OutputChannelAuthority, without changing the
# meaning of the shared centre-only supportMask itself.
old_halo = '        const int supportHalo=std::max(3,localFieldRadius);\n'
new_halo = (
    '        const int supportHalo=\n'
    '            std::max({3,localFieldRadius,reconstructionSupportRadius_});\n'
)
if new_halo not in text:
    if old_halo not in text:
        raise SystemExit("support halo anchor missing")
    text = text.replace(old_halo, new_halo, 1)

highlight_integral_marker = 'const auto highlightCensoredAt='
if highlight_integral_marker not in text:
    anchor = '''        st=readMaskRect(sx0,sy0,sx1,sy1,supportMask);
        if(!st) return st;

'''
    block = '''        st=readMaskRect(sx0,sy0,sx1,sy1,supportMask);
        if(!st) return st;

        // Highlight chroma authority is stricter than the historical shared
        // centre-sample mask. Build a local integral image from that unchanged
        // mask so only the highlight-colour guard sees reconstruction support.
        // Restoration, HDR and Natural Light field tone continue to consume the
        // original centre-only `censored` state below.
        const int highlightIntegralWidth=sw+1;
        std::vector<std::uint32_t> highlightCensorIntegral(
            static_cast<std::size_t>(highlightIntegralWidth)*
            static_cast<std::size_t>(sh+1),0u);
        for(int hy=0;hy<sh;++hy) {
            std::uint32_t rowCount=0u;
            for(int hx=0;hx<sw;++hx) {
                const std::size_t si=
                    static_cast<std::size_t>(hy)*static_cast<std::size_t>(sw)+
                    static_cast<std::size_t>(hx);
                rowCount += supportMask[si]!=0u?1u:0u;
                highlightCensorIntegral[
                    static_cast<std::size_t>(hy+1)*highlightIntegralWidth+
                    static_cast<std::size_t>(hx+1)] =
                    highlightCensorIntegral[
                        static_cast<std::size_t>(hy)*highlightIntegralWidth+
                        static_cast<std::size_t>(hx+1)] + rowCount;
            }
        }
        const auto highlightCensoredAt=[&](int px,int py) noexcept -> bool {
            const int radius=reconstructionSupportRadius_;
            const int ax0=std::max(sx0,px-radius);
            const int ay0=std::max(sy0,py-radius);
            const int ax1=std::min(sx1,px+radius+1);
            const int ay1=std::min(sy1,py+radius+1);
            const int lx0=ax0-sx0;
            const int ly0=ay0-sy0;
            const int lx1=ax1-sx0;
            const int ly1=ay1-sy0;
            const std::uint32_t count=
                highlightCensorIntegral[
                    static_cast<std::size_t>(ly1)*highlightIntegralWidth+lx1] -
                highlightCensorIntegral[
                    static_cast<std::size_t>(ly0)*highlightIntegralWidth+lx1] -
                highlightCensorIntegral[
                    static_cast<std::size_t>(ly1)*highlightIntegralWidth+lx0] +
                highlightCensorIntegral[
                    static_cast<std::size_t>(ly0)*highlightIntegralWidth+lx0];
            return count>0u;
        };

'''
    if anchor not in text:
        raise SystemExit("support-mask read anchor missing")
    text = text.replace(anchor, block, 1)

# Preserve the original shared mask for restoration/HDR/local field and derive a
# separate stricter boolean only for the highlight chroma guard.
shared_censor = '                const bool censored=supportMask[si]!=0u;\n'
highlight_censor = (
    shared_censor +
    '                const bool highlightCensored=highlightCensoredAt(x,y);\n'
)
if 'const bool highlightCensored=highlightCensoredAt(x,y);' not in text:
    if shared_censor not in text:
        raise SystemExit("shared centre-only censor anchor missing")
    text = text.replace(shared_censor, highlight_censor, 1)

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
                            r,g,b,highlightCensored)) {
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
else:
    old_call = '''presentation_highlight::apply_near_neutral_rolloff(
                            r,g,b,censored)'''
    new_call = '''presentation_highlight::apply_near_neutral_rolloff(
                            r,g,b,highlightCensored)'''
    if old_call in text:
        text = text.replace(old_call, new_call, 1)

required = (
    include_marker,
    alias_marker,
    call_marker,
    'if(extendedLinearHeadroomInput_) {',
    'presentation_headroom::map_90_100(r,g,b)',
    'int reconstructionSupportRadius_',
    'std::max({3,localFieldRadius,reconstructionSupportRadius_})',
    'const auto highlightCensoredAt=',
    'const bool censored=supportMask[si]!=0u;',
    'const bool highlightCensored=highlightCensoredAt(x,y);',
    'r,g,b,highlightCensored)',
    'std::max(0,reconstruction->requiredHalo())',
    'static_cast<float>(raw[local])>=source_.metadata().whiteLevel?1u:0u;',
)
missing = [token for token in required if token not in text]
if missing:
    raise SystemExit("highlight authority-split contract missing: " + ", ".join(missing))

# The shared write-stage mask must remain centre-only. No reconstruction-radius
# expansion or support-prefix logic is allowed in that block anymore.
write_start = text.index('    StreamStatus writeSdrTile(')
write_end = text.index('        stagedPixels_+=pixels;', write_start)
write_block = text[write_start:write_end]
for forbidden in ('supportPixels', 'saturatedPrefix', 'reconstructionSupportRadius_'):
    if forbidden in write_block:
        raise SystemExit(f"shared staged censor mask was broadened: {forbidden}")

# Restoration/HDR/local field must still use `censored`; only the highlight guard
# gets `highlightCensored`.
if 'if((flags_&kFlagHdr)!=0 && hdrPipelineEnabled_ && !censored)' not in text:
    raise SystemExit("HDR no longer uses historical centre-only censor state")
if 'r,g,b,localFieldY,censored,naturalLightEnabled' not in text:
    raise SystemExit("Natural Light local field no longer uses historical centre-only censor state")

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
print("PRESENTATION_HIGHLIGHT_CENSOR_AUTHORITY_SPLIT_V01_APPLIED")
