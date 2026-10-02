#!/usr/bin/env python3
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "suite_android/app/src/main/cpp"
JAVA = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"
RESEARCH = ROOT / "docs/research/sha256-direct-block-v0.1"

sha_h = (CPP / "truthraw_sha256_v0_69.h").read_text()
sha_cpp = (CPP / "truthraw_sha256_v0_69.cpp").read_text()
test = (
    RESEARCH /
    "tests/test_truthraw_sha256_direct_block_v0_1.cpp"
).read_text()
readme = (RESEARCH / "README.md").read_text()
tn_h = (
    ROOT /
    "docs/research/truthnegative-continuous-v0.5/native/"
    "truthnegative_continuous_v0_5.h"
).read_text()
tn_cpp = (
    ROOT /
    "docs/research/truthnegative-continuous-v0.5/native/"
    "truthnegative_continuous_v0_5.cpp"
).read_text()
pipeline_h = (CPP / "truthnegative_pipeline_bridge_common.h").read_text()
pipeline_cpp = (CPP / "truthnegative_pipeline_bridge_common.cpp").read_text()
bridge = (CPP / "truthnegative_n2_factored_confidence_bridge.cpp").read_text()
binding = (JAVA / "N2LocalSpatialBindingAudit.kt").read_text()
diag = (JAVA / "ResearchPerformanceDiagnosticsV01.kt").read_text()
profiler = (JAVA / "UniversalSourceProfiler.kt").read_text()
state = json.loads(
    (ROOT / "state/CURRENT_PROJECT_STATE_2026-10-02.json").read_text()
)

for token in [
    "directInputBlockTransformCount() const noexcept",
    "bufferedInputBlockTransformCount() const noexcept",
    "directInputBlockTransforms_",
    "bufferedInputBlockTransforms_",
]:
    assert token in sha_h, f"SHA transport diagnostics missing {token}"

for token in [
    "if(used_!=0u)",
    "transform(block_.data())",
    "++bufferedInputBlockTransforms_",
    "while(size>=block_.size())",
    "transform(data)",
    "++directInputBlockTransforms_",
    "std::memcpy(block_.data(),data,size)",
]:
    assert token in sha_cpp, f"SHA direct-block implementation missing {token}"

# Compression itself must remain the established scalar SHA-256 transform.
for token in [
    "constexpr std::array<std::uint32_t,64> K",
    "void Hasher::transform(const std::uint8_t* b) noexcept",
    "auto ch=(e&f)^((~e)&g)",
    "auto maj=(a&b0)^(a&c)^(b0&c)",
    "state_[0]+=a",
]:
    assert token in sha_cpp, f"SHA compression contract missing {token}"

for token in [
    "e3b0c44298fc1c149afbf4c8996fb924",
    "ba7816bf8f01cfea414140de5dae2223",
    "248d6a61d20638b8e5c026930c3e6039",
    "cdc76e5c9914fb9281a1c7e284d73e67",
    "authorityBatch{{2400u}}",
    "mixed{{1u,63u,64u,65u,127u,1024u,2400u,17u}}",
    "direct==0u",
    "buffered==64u",
]:
    assert token in test, f"SHA parity test missing {token}"

for token in [
    "same digest",
    "unchanged",
    "generic byte transport only",
    "arbitrary chunk",
    "real-device",
]:
    assert token.lower() in readme.lower(), f"SHA README missing {token}"

for token in [
    "shaDirectInputBlockTransformCount() const noexcept",
    "shaBufferedInputBlockTransformCount() const noexcept",
]:
    assert token in tn_h, f"authority SHA counter contract missing {token}"

for token in [
    "hasher_.directInputBlockTransformCount()",
    "hasher_.bufferedInputBlockTransformCount()",
]:
    assert token in tn_cpp, f"authority SHA counter binding missing {token}"

for token in [
    "authorityShaDirectBlockTransportActive",
    "authorityShaDirectInputBlockTransformCount",
    "authorityShaBufferedInputBlockTransformCount",
    "authorityShaDirectInputBytes",
]:
    assert token in pipeline_h, f"pipeline SHA telemetry missing {token}"
    assert token in bridge, f"JNI SHA telemetry missing {token}"

for token in [
    "authorityObserver.shaDirectInputBlockTransformCount()",
    "authorityObserver.shaBufferedInputBlockTransformCount()",
    "authorityShaDirectInputBlockTransformCount * 64u",
]:
    assert token in pipeline_cpp, f"runtime SHA telemetry missing {token}"

for token in [
    "authority_sha_direct_block_transport_active",
    "authority_sha_direct_input_block_transform_count",
    "authority_sha_buffered_input_block_transform_count",
    "authority_sha_direct_input_bytes",
]:
    assert token in binding, f"Kotlin SHA telemetry missing {token}"
    assert token in diag, f"Research SHA telemetry missing {token}"

for token in [
    "D.RAW/UniversalSourceProfileCache/0.2.9-sha-direct-block-v1",
    "N2_LOCAL_SPATIAL_V01_R9_SHA_DIRECT_BLOCK",
]:
    assert token in profiler, f"fresh SHA profiler identity missing {token}"

# 44488 remains the current recovery anchor and v0.2.8 remains the proven
# baseline while v0.2.9 is still only a candidate.
assert state["continuation_code"] == "44488"
assert state["latest_merged_pr"]["number"] == 115
assert state["source_code_head"] == "85e67ca3090aa113edda7133d656f28087e3ad22"
assert state["next_frontier"]["name"] == "SHA-256 direct-block transport v0.2.9"

# This candidate is transport-only and must not gain scientific authority.
for text in [pipeline_cpp, binding, diag]:
    for forbidden in [
        '.put("calibration_promoted", true)',
        '.put("correction_authorized", true)',
        '.put("candidate_applied", true)',
        '.put("creates_new_evidence", true)',
        '.put("scientific_writeback_allowed", true)',
    ]:
        assert forbidden not in text, f"SHA transport gained authority: {forbidden}"

print("sha256_direct_block_v0_2_9_static_integrity=PASS")
