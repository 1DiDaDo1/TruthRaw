#!/usr/bin/env python3
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "suite_android/app/src/main/cpp"
JAVA = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"
FIELD = CPP / "open_scene_field_v0_85.h"
FIELD_CPP = CPP / "open_scene_field_v0_85.cpp"
TN_H = ROOT / "docs/research/truthnegative-continuous-v0.5/native/truthnegative_continuous_v0_5.h"
TN_CPP = ROOT / "docs/research/truthnegative-continuous-v0.5/native/truthnegative_continuous_v0_5.cpp"
FIELD_TEST = ROOT / "docs/research/open-scene-field-v0.85/tests/test_open_scene_field_v085.cpp"
TN_TEST = ROOT / "docs/research/truthnegative-continuous-v0.5/tests/test_truthnegative_continuous_v0_5.cpp"
README = ROOT / "docs/research/authority-pixel-triplet-v0.1/README.md"

field_h = FIELD.read_text()
field_cpp = FIELD_CPP.read_text()
tn_h = TN_H.read_text()
tn_cpp = TN_CPP.read_text()
field_test = FIELD_TEST.read_text()
tn_test = TN_TEST.read_text()
pipeline_h = (CPP / "truthnegative_pipeline_bridge_common.h").read_text()
pipeline_cpp = (CPP / "truthnegative_pipeline_bridge_common.cpp").read_text()
bridge = (CPP / "truthnegative_n2_factored_confidence_bridge.cpp").read_text()
binding = (JAVA / "N2LocalSpatialBindingAudit.kt").read_text()
diag = (JAVA / "ResearchPerformanceDiagnosticsV01.kt").read_text()
profiler = (JAVA / "UniversalSourceProfiler.kt").read_text()
readme = README.read_text()
state = json.loads((ROOT / "state/CURRENT_PROJECT_STATE_2026-10-02.json").read_text())

for token in [
    "kCanonicalAuthorityPixelTripletBytes",
    "CanonicalSourcePixelTriplet",
    "encode_source_pixel_triplet_canonical_v1(",
    "measuredChannel",
    "measuredCensored",
]:
    assert token in field_h, f"pixel-triplet contract missing {token}"

for token in [
    "encode_source_pixel_triplet_canonical_v1(",
    "const int measured =",
    "measured_channel(cfa, globalX, globalY)",
    "const bool censored =",
    "source_record_semantics(",
    "write_source_record_canonical(",
]:
    assert token in field_cpp, f"pixel-triplet implementation missing {token}"

triplet_region = field_cpp[
    field_cpp.index("encode_source_pixel_triplet_canonical_v1("):
    field_cpp.index("encode_source_channel_record_canonical_v1(")
]
assert triplet_region.count("measured_channel(") == 1, (
    "triplet encoder must resolve CFA measured channel exactly once per pixel"
)
assert "for (std::size_t ch = 0u; ch < 3u; ++ch)" in triplet_region

for token in [
    "test_pixel_triplet_exact_parity",
    "CfaPattern::BGGR",
    "CfaPattern::RGGB",
    "CfaPattern::GRBG",
    "CfaPattern::GBRG",
    "rawCode>=1023u",
    "pixel triplet bytes equal concatenated single records",
    "pixel triplet fails closed on non-finite source value",
]:
    assert token in field_test, f"triplet parity test missing {token}"

for token in [
    "directPixelTripletCount() const noexcept",
    "genericFallbackPixelCount() const noexcept",
    "accountCanonicalSourcePixelTriplet(",
]:
    assert token in tn_h, f"triplet accumulator contract missing {token}"

append_region = tn_cpp[
    tn_cpp.index("bool AuthorityFieldAccumulator::appendSourceTile("):
    tn_cpp.index("std::size_t AuthorityFieldAccumulator::residentBytesUpperBound",)
]
for token in [
    "encode_source_pixel_triplet_canonical_v1(",
    "accountCanonicalSourcePixelTriplet(",
    "directByteRecordCount_ += 3u",
    "++directPixelTripletCount_",
    "UnsupportedSemanticExtension",
    "genericFallbackRecordCount_ += 3u",
    "++genericFallbackPixelCount_",
]:
    assert token in append_region, f"triplet accumulator path missing {token}"
assert "encode_source_channel_record_canonical_v1(" not in append_region, (
    "fast append path must not call the single-channel canonical encoder"
)

for token in [
    "accumulator.directPixelTripletCount() ==",
    "accumulator.genericFallbackPixelCount() == 0u",
    "fused.contentSha256 == replay.contentSha256",
]:
    assert token in tn_test, f"triplet authority parity assertion missing {token}"

for token in [
    "authorityPixelTripletEncodingActive",
    "authorityCanonicalPixelTripletBytes",
    "authorityDirectPixelTripletCount",
    "authorityGenericFallbackPixelCount",
]:
    assert token in pipeline_h, f"pipeline triplet telemetry missing {token}"
    assert token in bridge, f"JNI triplet telemetry missing {token}"

for token in [
    "authorityObserver.directPixelTripletCount()",
    "authorityObserver.genericFallbackPixelCount()",
    "kCanonicalAuthorityPixelTripletBytes",
]:
    assert token in pipeline_cpp, f"runtime triplet telemetry missing {token}"

for token in [
    "authority_pixel_triplet_encoding_active",
    "authority_canonical_pixel_triplet_bytes",
    "authority_direct_pixel_triplet_count",
    "authority_generic_fallback_pixel_count",
]:
    assert token in binding, f"Kotlin triplet telemetry missing {token}"
    assert token in diag, f"Research triplet telemetry missing {token}"

for token in [
    "D.RAW/UniversalSourceProfileCache/0.2.11-authority-template-reuse-v1",
    "N2_LOCAL_SPATIAL_V01_R11_AUTHORITY_TEMPLATE_REUSE",
]:
    assert token in profiler, f"fresh triplet profiler identity missing {token}"

for token in [
    "exactly 75 bytes",
    "BGGR",
    "RGGB",
    "GRBG",
    "GBRG",
    "fail closed",
    "general route",
    "does **not** include",
]:
    assert token.lower() in readme.lower(), f"triplet README missing {token}"

# 44488 must still point to the proven merged baseline while v0.2.10 is draft.
assert state["continuation_code"] == "44488"
assert state["latest_merged_pr"]["number"] == 116
assert state["source_code_head"] == "edcae3eabb077d53b6a25802e6b9ca62c188b973"
assert state["next_frontier"]["name"] == "Pixel-triplet authority encoder v0.2.10"

for text in [pipeline_cpp, binding, diag]:
    for forbidden in [
        '.put("calibration_promoted", true)',
        '.put("correction_authorized", true)',
        '.put("candidate_applied", true)',
        '.put("creates_new_evidence", true)',
        '.put("scientific_writeback_allowed", true)',
    ]:
        assert forbidden not in text, f"triplet optimization gained authority: {forbidden}"

print("authority_pixel_triplet_v0_2_10_static_integrity=PASS")
