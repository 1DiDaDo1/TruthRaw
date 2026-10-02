#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
JAVA = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"
CPP = ROOT / "suite_android/app/src/main/cpp"
FIELD_TEST = ROOT / "docs/research/open-scene-field-v0.85/tests/test_open_scene_field_v085.cpp"
TN = ROOT / "docs/research/truthnegative-continuous-v0.5"
FUSION = ROOT / "docs/research/truthnegative-authority-field-fusion-v0.1"
DIRECT = ROOT / "docs/research/truthnegative-authority-direct-stream-v0.1"

field_h = (CPP / "open_scene_field_v0_85.h").read_text()
field_cpp = (CPP / "open_scene_field_v0_85.cpp").read_text()
field_test = FIELD_TEST.read_text()
tn_h = (TN / "native/truthnegative_continuous_v0_5.h").read_text()
tn_cpp = (TN / "native/truthnegative_continuous_v0_5.cpp").read_text()
tn_test = (TN / "tests/test_truthnegative_continuous_v0_5.cpp").read_text()
fusion_test = (
    FUSION / "tests/test_truthnegative_authority_field_fusion_v0_1.cpp"
).read_text()
pipeline_h = (CPP / "truthnegative_pipeline_bridge_common.h").read_text()
pipeline_cpp = (CPP / "truthnegative_pipeline_bridge_common.cpp").read_text()
bridge = (CPP / "truthnegative_n2_factored_confidence_bridge.cpp").read_text()
binding = (JAVA / "N2LocalSpatialBindingAudit.kt").read_text()
diag = (JAVA / "ResearchPerformanceDiagnosticsV01.kt").read_text()
profiler = (JAVA / "UniversalSourceProfiler.kt").read_text()
readme = (DIRECT / "README.md").read_text()

for token in [
    "build_source_channel_record(",
    "std::uint16_t rawCode",
    "float cameraNativeValue",
    "ChannelRecord& out",
]:
    assert token in field_h, f"single source-record contract missing {token}"

for token in [
    "bool build_source_channel_record(",
    "measured_channel(cfa, globalX, globalY)",
    "static_cast<float>(rawCode) >= whiteLevel",
    "return validate_record(out)",
    "if(!build_source_channel_record(",
]:
    assert token in field_cpp, f"canonical single-record implementation missing {token}"

for token in [
    "single/vector classification exact",
    "single/vector value bits exact",
    "single/vector p95 bits exact",
    "single/vector support bits exact",
    "single/vector bound bits exact",
    "single/vector contribution mask exact",
]:
    assert token in field_test, f"single/vector bit parity test missing {token}"

assert "std::vector<field::ChannelRecord> scratch_" not in tn_h, (
    "direct accumulator still owns a temporary record vector"
)
for token in [
    "AuthorityFieldAccumulator::beginTile",
    "AuthorityFieldAccumulator::appendRecord",
    "AuthorityFieldAccumulator::finishTile",
    "field::build_source_channel_record(",
    "std::size_t AuthorityFieldAccumulator::residentBytesUpperBound() const noexcept",
    "return 0u;",
]:
    assert token in tn_cpp, f"direct authority accumulator missing {token}"

append_source = tn_cpp[
    tn_cpp.index("bool AuthorityFieldAccumulator::appendSourceTile("):
    tn_cpp.index("std::size_t AuthorityFieldAccumulator::residentBytesUpperBound",)
]
assert "build_source_tile_records(" not in append_source, (
    "direct authority path still materializes a tile record vector"
)
assert "std::vector<field::ChannelRecord>" not in append_source, (
    "direct authority path allocates record vector"
)

for token in [
    "accumulator.residentBytesUpperBound() == 0u",
    "fused.contentSha256 == replay.contentSha256",
    "fused.creationRoleCounts == replay.creationRoleCounts",
    "fused.authorityCounts == replay.authorityCounts",
]:
    assert token in tn_test, f"direct authority parity test missing {token}"

for token in [
    "observer.residentBytesUpperBound() == 0u",
    "require_same_summary(",
    "fusedAuthority",
    "replayAuthority",
    "directScientific.scientificMasterHash ==",
]:
    assert token in fusion_test, f"end-to-end direct-stream parity missing {token}"

for token in [
    "authorityDirectRecordStreamingActive",
    "authorityTemporaryRecordVectorUsed",
    "authorityDirectRecordStreamMs",
    "authorityDirectRecordStreamTileCount",
    "authorityDirectRecordStreamRecordCount",
    "authorityAccumulatorResidentBytesUpperBound",
]:
    assert token in pipeline_h, f"pipeline direct-stream telemetry missing {token}"
    assert token in bridge, f"JNI direct-stream telemetry missing {token}"

for token in [
    "directRecordStreamMs_",
    "recordCount_",
    "authorityDirectRecordStreamingActive = true",
    "authorityTemporaryRecordVectorUsed = false",
]:
    assert token in pipeline_cpp, f"runtime direct-stream state missing {token}"

for token in [
    "authority_direct_record_streaming_active",
    "authority_temporary_record_vector_used",
    "authority_direct_record_stream_ms",
    "authority_direct_record_stream_tile_count",
    "authority_direct_record_stream_record_count",
    "authority_accumulator_resident_bytes_upper_bound",
]:
    assert token in binding, f"Kotlin direct-stream telemetry missing {token}"
    assert token in diag, f"Research direct-stream telemetry missing {token}"

for token in [
    "D.RAW/UniversalSourceProfileCache/0.2.7-authority-direct-stream-v1",
    "N2_LOCAL_SPATIAL_V01_R7_AUTHORITY_DIRECT_STREAM",
]:
    assert token in profiler, f"fresh direct-stream profiler identity missing {token}"

for token in [
    "single canonical source-record",
    "zero resident record-vector scratch",
    "same records were counted and hashed",
    "no source value",
    "scientific writeback",
]:
    assert token.lower() in readme.lower(), f"direct-stream README missing {token}"

for text in [binding, diag]:
    for forbidden in [
        '.put("calibration_promoted", true)',
        '.put("correction_authorized", true)',
        '.put("candidate_applied", true)',
        '.put("creates_new_evidence", true)',
        '.put("scientific_writeback_allowed", true)',
    ]:
        assert forbidden not in text, f"direct stream gained authority: {forbidden}"

print("authority_direct_stream_v0_2_7_static_integrity=PASS")
