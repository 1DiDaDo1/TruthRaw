#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "suite_android/app/src/main/cpp"
JAVA = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"
FIELD_TEST = ROOT / "docs/research/open-scene-field-v0.85/tests/test_open_scene_field_v085.cpp"
TN = ROOT / "docs/research/truthnegative-continuous-v0.5"
FUSION = ROOT / "docs/research/truthnegative-authority-field-fusion-v0.1"
DIRECT_BYTE = ROOT / "docs/research/truthnegative-authority-direct-byte-v0.1"

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
readme = (DIRECT_BYTE / "README.md").read_text()

for token in [
    "kCanonicalAuthorityRecordBytes = 25u",
    "struct CanonicalSourceChannelRecord",
    "enum class CanonicalSourceEncodingStatus",
    "Encoded = 1u",
    "UnsupportedSemanticExtension = 2u",
    "Invalid = 3u",
    "encode_source_channel_record_canonical_v1(",
]:
    assert token in field_h, f"versioned canonical byte contract missing {token}"

for token in [
    "classify_source_channel(",
    "encode_source_channel_record_canonical_v1(",
    "put_f32_record(out.bytes, offset, cameraNativeValue)",
    "return CanonicalSourceEncodingStatus::Encoded",
    "return validate_record(out)",
]:
    assert token in field_cpp, f"canonical byte/source semantic sharing missing {token}"

for token in [
    "versioned canonical direct-byte source encode",
    "direct-byte/materialized canonical bytes exact",
    "direct-byte/materialized summary fields exact",
    "canonical direct-byte encoder fails closed on non-finite source value",
]:
    assert token in field_test, f"direct-byte record parity test missing {token}"

for token in [
    "kAuthorityDirectHashBatchRecordCount = 96u",
    "kAuthorityDirectHashBatchBytes",
    "directByteRecordCount() const noexcept",
    "genericFallbackRecordCount() const noexcept",
]:
    assert token in tn_h, f"direct-byte accumulator contract missing {token}"

append_source = tn_cpp[
    tn_cpp.index("bool AuthorityFieldAccumulator::appendSourceTile("):
    tn_cpp.index("std::size_t AuthorityFieldAccumulator::residentBytesUpperBound",)
]
for token in [
    "encode_source_channel_record_canonical_v1(",
    "CanonicalSourceEncodingStatus::Encoded",
    "CanonicalSourceEncodingStatus::",
    "UnsupportedSemanticExtension",
    "flushBatch()",
    "build_source_channel_record(",
    "appendRecord(fallback)",
    "++directByteRecordCount_",
    "++genericFallbackRecordCount_",
    "hasher_.update(byteBatch.data(), batchUsed)",
]:
    assert token in append_source, f"direct-byte/fallback route missing {token}"

assert "std::vector<field::ChannelRecord>" not in append_source, (
    "direct-byte source path reintroduced a record vector"
)
assert append_source.index("flushBatch()") < append_source.index("appendRecord(fallback)"), (
    "generic fallback must flush pending direct bytes before canonical record hashing"
)

for token in [
    "accumulator.directByteRecordCount() == fused.recordCount",
    "accumulator.genericFallbackRecordCount() == 0u",
    "fused.contentSha256 == replay.contentSha256",
    "fused.creationRoleCounts == replay.creationRoleCounts",
    "fused.authorityCounts == replay.authorityCounts",
]:
    assert token in tn_test, f"authority direct-byte SHA/count parity missing {token}"

for token in [
    "require_same_summary(",
    "fusedAuthority",
    "replayAuthority",
    "directScientific.scientificMasterHash ==",
]:
    assert token in fusion_test, f"end-to-end authority/master parity missing {token}"

for token in [
    "authorityDirectByteEncodingActive",
    "authorityGenericRecordValidationBypassed",
    "authorityCanonicalRecordBytes",
    "authorityHashBatchRecordCapacity",
    "authorityHashBatchBytes",
    "authorityDirectByteRecordCount",
    "authorityGenericFallbackRecordCount",
]:
    assert token in pipeline_h, f"pipeline direct-byte telemetry missing {token}"
    assert token in bridge, f"JNI direct-byte telemetry missing {token}"

for token in [
    "authorityObserver.directByteRecordCount() > 0u",
    "authorityObserver.genericFallbackRecordCount() == 0u",
    "kCanonicalAuthorityRecordBytes",
    "kAuthorityDirectHashBatchRecordCount",
    "kAuthorityDirectHashBatchBytes",
]:
    assert token in pipeline_cpp, f"runtime direct-byte telemetry binding missing {token}"

for token in [
    "authority_direct_byte_encoding_active",
    "authority_generic_record_validation_bypassed",
    "authority_canonical_record_bytes",
    "authority_hash_batch_record_capacity",
    "authority_hash_batch_bytes",
    "authority_direct_byte_record_count",
    "authority_generic_fallback_record_count",
]:
    assert token in binding, f"Kotlin direct-byte telemetry missing {token}"
    assert token in diag, f"Research direct-byte telemetry missing {token}"

for token in [
    "D.RAW/UniversalSourceProfileCache/0.2.9-sha-direct-block-v1",
    "N2_LOCAL_SPATIAL_V01_R9_SHA_DIRECT_BLOCK",
]:
    assert token in profiler, f"fresh direct-byte profiler identity missing {token}"

for token in [
    "UnsupportedSemanticExtension",
    "general materialized",
    "flushes all pending canonical bytes",
    "zero generic fallback records",
    "fast path is **not** the scientific contract",
]:
    assert token.lower() in readme.lower(), f"flexible-cable contract missing {token}"

# The specialized path remains a transport/performance implementation.
for text in [binding, diag]:
    for forbidden in [
        '.put("calibration_promoted", true)',
        '.put("correction_authorized", true)',
        '.put("candidate_applied", true)',
        '.put("creates_new_evidence", true)',
        '.put("scientific_writeback_allowed", true)',
    ]:
        assert forbidden not in text, f"direct-byte route gained authority: {forbidden}"

print("authority_direct_byte_v0_2_8_static_integrity=PASS")
