#!/usr/bin/env python3
from pathlib import Path

TARGET = Path("suite_android/app/src/main/cpp/photo_export_bridge.cpp")


def require_replace(text: str, old: str, new: str, label: str) -> str:
    if new in text:
        return text
    if old not in text:
        raise SystemExit(f"missing contract anchor: {label}")
    return text.replace(old, new, 1)


def main() -> None:
    text = TARGET.read_text(encoding="utf-8")
    original = text

    text = require_replace(
        text,
        '#include "presentation_illuminant_warmth_retention_v0_1.h"\n',
        '#include "presentation_illuminant_warmth_retention_v0_1.h"\n'
        '#include "presentation_natural_light_field_tone_v0_1.h"\n',
        "Natural Light field-tone include",
    )

    ns_old = (
        "namespace presentation_illuminant_warmth = "
        "truthraw::presentation_illuminant_warmth_retention::v0_1;\n"
    )
    ns_new = (
        ns_old
        + "namespace presentation_natural_light_field = "
          "truthraw::presentation_natural_light_field_tone::v0_1;\n"
    )
    text = require_replace(text, ns_old, ns_new, "Natural Light field-tone namespace")

    support_old = (
        "        constexpr int kSupportHalo=3;\n"
        "        const int sx0=std::max(0,x0-kSupportHalo);\n"
        "        const int sy0=std::max(0,y0-kSupportHalo);\n"
        "        const int sx1=std::min(sourceWidth_,x1+kSupportHalo);\n"
        "        const int sy1=std::min(sourceHeight_,y1+kSupportHalo);\n"
        "        const int sw=sx1-sx0;\n"
    )
    support_new = (
        "        const bool naturalLightEnabled=(flags_&kFlagLight)!=0;\n"
        "        const bool naturalLightFieldEnabled=\n"
        "            naturalLightEnabled && !extendedLinearHeadroomInput_;\n"
        "        const int localFieldRadius=naturalLightFieldEnabled\n"
        "            ? std::clamp(std::min(sourceWidth_,sourceHeight_)/128,12,32)\n"
        "            : 3;\n"
        "        const int supportHalo=std::max(3,localFieldRadius);\n"
        "        const int sx0=std::max(0,x0-supportHalo);\n"
        "        const int sy0=std::max(0,y0-supportHalo);\n"
        "        const int sx1=std::min(sourceWidth_,x1+supportHalo);\n"
        "        const int sy1=std::min(sourceHeight_,y1+supportHalo);\n"
        "        const int sw=sx1-sx0;\n"
        "        const int sh=sy1-sy0;\n"
    )
    # Later downstream patches may widen supportHalo for their own isolated
    # authority evaluation. Once the Natural Light field geometry already
    # exists, do not require this script's historical exact supportHalo line.
    if "const bool naturalLightFieldEnabled=" not in text:
        text = require_replace(text, support_old, support_new, "field support geometry")
    else:
        accepted_support = (
            "const int supportHalo=std::max(3,localFieldRadius);" in text
            or "std::max({3,localFieldRadius,reconstructionSupportRadius_})" in text
        )
        if not accepted_support:
            raise SystemExit("Natural Light field support geometry is present but unrecognized")

    mask_old = (
        "        st=readMaskRect(sx0,sy0,sx1,sy1,supportMask);\n"
        "        if(!st) return st;\n\n"
        "        const int ax0=std::max(0,x0-1);\n"
    )
    mask_new = (
        "        st=readMaskRect(sx0,sy0,sx1,sy1,supportMask);\n"
        "        if(!st) return st;\n\n"
        "        // Build a deterministic integral image from already-rendered RGB.\n"
        "        // Censored samples are excluded: this stage is a View/Appearance\n"
        "        // neighbourhood cue, never clipped-radiance recovery or light-transport proof.\n"
        "        const int fieldIntegralWidth=sw+1;\n"
        "        std::vector<double> fieldLumaIntegral;\n"
        "        std::vector<std::uint32_t> fieldCountIntegral;\n"
        "        if(naturalLightFieldEnabled) {\n"
        "            const std::size_t integralCells=\n"
        "                static_cast<std::size_t>(fieldIntegralWidth)*static_cast<std::size_t>(sh+1);\n"
        "            fieldLumaIntegral.assign(integralCells,0.0);\n"
        "            fieldCountIntegral.assign(integralCells,0u);\n"
        "            for(int fy=0;fy<sh;++fy) {\n"
        "                double rowLuma=0.0;\n"
        "                std::uint32_t rowCount=0u;\n"
        "                for(int fx=0;fx<sw;++fx) {\n"
        "                    const std::size_t si=\n"
        "                        static_cast<std::size_t>(fy)*sw+static_cast<std::size_t>(fx);\n"
        "                    if(supportMask[si]==0u) {\n"
        "                        const float rr=supportRgb[3u*si];\n"
        "                        const float gg=supportRgb[3u*si+1u];\n"
        "                        const float bb=supportRgb[3u*si+2u];\n"
        "                        if(std::isfinite(rr)&&std::isfinite(gg)&&std::isfinite(bb)) {\n"
        "                            const float yy=std::clamp(\n"
        "                                presentation_natural_light_field::luminance709(rr,gg,bb),\n"
        "                                0.0f,1.0f);\n"
        "                            rowLuma+=static_cast<double>(yy);\n"
        "                            ++rowCount;\n"
        "                        }\n"
        "                    }\n"
        "                    const std::size_t p=\n"
        "                        static_cast<std::size_t>(fy+1)*fieldIntegralWidth+\n"
        "                        static_cast<std::size_t>(fx+1);\n"
        "                    const std::size_t above=\n"
        "                        static_cast<std::size_t>(fy)*fieldIntegralWidth+\n"
        "                        static_cast<std::size_t>(fx+1);\n"
        "                    fieldLumaIntegral[p]=fieldLumaIntegral[above]+rowLuma;\n"
        "                    fieldCountIntegral[p]=fieldCountIntegral[above]+rowCount;\n"
        "                }\n"
        "            }\n"
        "        }\n\n"
        "        const int ax0=std::max(0,x0-1);\n"
    )
    # The highlight-authority split may insert its own integral between the
    # shared mask read and this field integral. If the field integral already
    # exists, treat it as authoritative and do not demand historical adjacency.
    if "const int fieldIntegralWidth=sw+1;" not in text:
        text = require_replace(text, mask_old, mask_new, "field integral build")

    gate_marker = "        const bool naturalLightEnabled=(flags_&kFlagLight)!=0;\n"
    exposure_old = (
        gate_marker
        + "        const float exposureGain=advanced_controls::presentation_exposure_gain(\n"
    )
    exposure_new = (
        "        const float exposureGain=advanced_controls::presentation_exposure_gain(\n"
    )
    if exposure_old in text:
        text = text.replace(exposure_old, exposure_new, 1)
    elif text.count(gate_marker) != 1:
        raise SystemExit("Natural Light gate must exist exactly once after field wiring")

    shadow_old = (
        "        const float shadowMix=advanced_controls::shadow_recovery_mix(\n"
        "            static_cast<std::uint32_t>(flags_));\n\n"
        "        for(int y=ay0;y<ay1;++y) {\n"
    )
    shadow_new = (
        "        const float shadowMix=advanced_controls::shadow_recovery_mix(\n"
        "            static_cast<std::uint32_t>(flags_));\n\n"
        "        auto localFieldMeanAt=[&](int px,int py) noexcept -> float {\n"
        "            if(!naturalLightFieldEnabled || fieldLumaIntegral.empty() ||\n"
        "               fieldCountIntegral.empty()) return 0.0f;\n"
        "            const int rx0=std::max(sx0,px-localFieldRadius);\n"
        "            const int ry0=std::max(sy0,py-localFieldRadius);\n"
        "            const int rx1=std::min(sx1,px+localFieldRadius+1);\n"
        "            const int ry1=std::min(sy1,py+localFieldRadius+1);\n"
        "            const int ix0=rx0-sx0, iy0=ry0-sy0;\n"
        "            const int ix1=rx1-sx0, iy1=ry1-sy0;\n"
        "            const auto at=[&](const auto& integral,int ix,int iy) noexcept {\n"
        "                return integral[static_cast<std::size_t>(iy)*fieldIntegralWidth+\n"
        "                                static_cast<std::size_t>(ix)];\n"
        "            };\n"
        "            const double sum=\n"
        "                at(fieldLumaIntegral,ix1,iy1)-at(fieldLumaIntegral,ix0,iy1)-\n"
        "                at(fieldLumaIntegral,ix1,iy0)+at(fieldLumaIntegral,ix0,iy0);\n"
        "            const std::uint32_t count=\n"
        "                at(fieldCountIntegral,ix1,iy1)-at(fieldCountIntegral,ix0,iy1)-\n"
        "                at(fieldCountIntegral,ix1,iy0)+at(fieldCountIntegral,ix0,iy0);\n"
        "            if(count==0u || !std::isfinite(sum) || sum<0.0) return 0.0f;\n"
        "            const float mean=static_cast<float>(sum/static_cast<double>(count));\n"
        "            return std::clamp(mean*exposureGain,0.0f,1.0f);\n"
        "        };\n\n"
        "        for(int y=ay0;y<ay1;++y) {\n"
    )
    text = require_replace(text, shadow_old, shadow_new, "field mean query")

    runtime_old = (
        "                } else {\n"
        "                    // Natural Light may retain a bounded fraction of a warm\n"
    )
    runtime_new = (
        "                } else {\n"
        "                    // A bounded image-space luminous-field cue improves Natural\n"
        "                    // Light appearance without claiming physical light transport.\n"
        "                    // It is a common RGB gain and censored samples are protected.\n"
        "                    const float localFieldY=localFieldMeanAt(x,y);\n"
        "                    if(!presentation_natural_light_field::apply(\n"
        "                            r,g,b,localFieldY,censored,naturalLightEnabled)) {\n"
        "                        return StreamStatus::error(\n"
        "                            StreamStatusCode::SinkFailed,\n"
        "                            \"full-res Natural Light local field tone failed\");\n"
        "                    }\n"
        "                    // Natural Light may retain a bounded fraction of a warm\n"
    )
    # Downstream authority-bound Appearance stages may legitimately be inserted
    # between the Natural Light call and the historical warmth comment. Once the
    # call is present, validate it below instead of requiring the old adjacency.
    if "presentation_natural_light_field::apply(" not in text:
        text = require_replace(text, runtime_old, runtime_new, "ADVANCED/PRO field-tone call")

    required = [
        '#include "presentation_natural_light_field_tone_v0_1.h"',
        "namespace presentation_natural_light_field =",
        "const bool naturalLightFieldEnabled=",
        "naturalLightEnabled && !extendedLinearHeadroomInput_;",
        "fieldLumaIntegral",
        "localFieldMeanAt",
        "presentation_natural_light_field::apply(",
        "r,g,b,localFieldY,censored,naturalLightEnabled",
    ]
    for token in required:
        if token not in text:
            raise SystemExit(f"post-patch contract missing: {token}")

    if text.count(gate_marker) != 1:
        raise SystemExit("Natural Light gate count is not canonical")

    pure_pos = text.index("if(extendedLinearHeadroomInput_)")
    else_pos = text.index("                } else {", pure_pos)
    field_pos = text.index("presentation_natural_light_field::apply(", else_pos)
    warmth_pos = text.index("presentation_illuminant_warmth::apply(", field_pos)
    highlight_pos = text.index("presentation_highlight::apply_near_neutral_rolloff(", warmth_pos)
    gamut_pos = text.index("presentation_gamut::fit_unit_rgb_preserve_luminance(", highlight_pos)
    if not (pure_pos < else_pos < field_pos < warmth_pos < highlight_pos < gamut_pos):
        raise SystemExit("Natural Light field-tone stage ordering contract failed")

    if text.count("presentation_natural_light_field::apply(") != 1:
        raise SystemExit("Natural Light field-tone apply must have exactly one runtime call")

    if text == original:
        print("NATURAL_LIGHT_FIELD_TONE_V0_1_ALREADY_APPLIED")
        return

    TARGET.write_text(text, encoding="utf-8")
    print("NATURAL_LIGHT_FIELD_TONE_V0_1_APPLIED")


if __name__ == "__main__":
    main()
