#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
JAVA = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"
CPP = ROOT / "suite_android/app/src/main/cpp"
TN = ROOT / "docs/research/truthnegative-continuous-v0.5"
SMSB = ROOT / "docs/research/scientific-master-streaming-binding-v0.2"
FUSION = ROOT / "docs/research/truthnegative-authority-field-fusion-v0.1"

tn_h = (TN / "native/truthnegative_continuous_v0_5.h").read_text()
tn_cpp = (TN / "native/truthnegative_continuous_v0_5.cpp").read_text()
tn_test = (TN / "tests/test_truthnegative_continuous_v0_5.cpp").read_text()
smsb_h = (SMSB / "native/scientific_master_streaming_binding_v0_2.h").read_text()
smsb_cpp = (SMSB / "native/scientific_master_streaming_binding_v0_2.cpp").read_text()
smsb_test = (SMSB / "tests/test_scientific_master_streaming_binding_v0_2.cpp").read_text()
pipeline_h = (CPP / "truthnegative_pipeline_bridge_common.h").read_text()
pipeline_cpp = (CPP / "truthnegative_pipeline_bridge_common.cpp").read_text()
bridge = (CPP / "truthnegative_n2_factored_confidence_bridge.cpp").read_text()
binding = (JAVA / "N2LocalSpatialBindingAudit.kt").read_text()
diag = (JAVA / "ResearchPerformanceDiagnosticsV01.kt").read_text()
profiler = (JAVA / "UniversalSourceProfiler.kt").read_text()
fusion_test = (
    FUSION / "tests/test_truthnegative_authority_field_fusion_v0_1.cpp"
).read_text()
fusion_readme = (FUSION / "README.md").read_text()

for token in [
    "class AuthorityFieldAccumulator",
    "appendRecords(",
    "appendSourceTile(",
    "residentBytesUpperBound() const noexcept",
    "finalize(AuthorityFieldSummary& out)",
]:
    assert token in tn_h, f"authority accumulator contract missing {token}"

for token in [
    "AuthorityFieldAccumulator::appendRecords",
    "AuthorityFieldAccumulator::appendSourceTile",
    "field::build_source_channel_record(",
    "AuthorityFieldAccumulator accumulator(",
    "accumulator.appendRecords(",
    "accumulator.finalize(out)",
]:
    assert token in tn_cpp, f"authority accumulator implementation missing {token}"

for token in [
    "test_authority_accumulator_source_tile_path_is_exact",
    "fused.contentSha256 == replay.contentSha256",
    "fused.creationRoleCounts == replay.creationRoleCounts",
    "fused.authorityCounts == replay.authorityCounts",
]:
    assert token in tn_test, f"authority accumulator parity test missing {token}"

for token in [
    "class ICanonicalTileObserver",
    "residentBytesUpperBound() const noexcept",
    "observeCanonicalTile(",
    "bind_scientific_master_streaming_observed(",
]:
    assert token in smsb_h, f"Scientific Master observer contract missing {token}"

for token in [
    "observer->observeCanonicalTile(",
    "observer_resident_bytes(",
    "bind_impl(",
    "bind_scientific_master_streaming_observed(",
]:
    assert token in smsb_cpp, f"Scientific Master observer implementation missing {token}"

for token in [
    "VerifyingObserver",
    "observed.scientificMasterHash == v2.scientificMasterHash",
    "sourceObserved.rawTileCalls == sourceV2.rawTileCalls",
    "observer.tileCount == tiles",
]:
    assert token in smsb_test, f"Scientific Master observer neutrality test missing {token}"

for token in [
    "authorityFusedFinalizeMs",
    "authorityFusedIntoScientificMasterPass",
    "authorityReplayPassPerformed",
]:
    assert token in pipeline_h, f"pipeline fused authority telemetry missing {token}"

for token in [
    "class AuthorityFieldFusionObserver",
    "AuthorityFieldAccumulator accumulator_",
    "bind_scientific_master_streaming_observed(",
    "authorityObserver.finalize(",
    "authorityFusedIntoScientificMasterPass =",
    "authorityReplayPassPerformed =",
]:
    assert token in pipeline_cpp, f"Android authority fusion missing {token}"

# The candidate pipeline must not execute the historical full replay summary.
prepare_region = pipeline_cpp[
    pipeline_cpp.index("Status prepare("):
    pipeline_cpp.index("Status acquireShared(")
]
assert "summarizeAuthorityField(" not in prepare_region, (
    "Android prepare still performs historical authority replay pass"
)
assert "SourceFieldAdapter" in prepare_region, (
    "field source must remain available for later read-only queries/audits"
)

for token in [
    "authorityFieldFusedIntoScientificMasterPass",
    "authorityFieldReplayPassPerformed",
    "authorityFieldFusedFinalizeMs",
]:
    assert token in bridge, f"JNI fused authority telemetry missing {token}"

for token in [
    "authority_field_fused_into_scientific_master_pass",
    "authority_field_replay_pass_performed",
    "authority_field_fused_finalize_ms",
]:
    assert token in binding, f"Kotlin fused authority telemetry missing {token}"
    assert token in diag, f"Research fused authority telemetry missing {token}"

for token in [
    "D.RAW/UniversalSourceProfileCache/0.2.9-sha-direct-block-v1",
    "N2_LOCAL_SPATIAL_V01_R9_SHA_DIRECT_BLOCK",
]:
    assert token in profiler, f"fresh authority fusion profiler identity missing {token}"

for token in [
    "require_same_summary(",
    "fusedAuthority",
    "replayAuthority",
    "readsAfterFusedScientific + 2u * tiles",
    "directScientific.scientificMasterHash ==",
    "directSource.rawTileCalls ==",
]:
    assert token in fusion_test, f"end-to-end fusion parity test missing {token}"

for token in [
    "same canonical 64x64 tile order",
    "same canonical `build_source_channel_record()` implementation",
    "same authority-field SHA-256",
    "no new evidence",
    "no Scientific Master writeback",
]:
    assert token.lower() in fusion_readme.lower(), f"fusion README missing {token}"

# Fusion is transport/reuse only. It cannot grant authority or write back.
for text in [pipeline_cpp, binding, diag]:
    for forbidden in [
        '.put("calibration_promoted", true)',
        '.put("correction_authorized", true)',
        '.put("candidate_applied", true)',
        '.put("creates_new_evidence", true)',
        '.put("scientific_writeback_allowed", true)',
    ]:
        assert forbidden not in text, f"authority fusion gained authority: {forbidden}"

print("authority_field_fused_pass_v0_2_6_static_integrity=PASS")
