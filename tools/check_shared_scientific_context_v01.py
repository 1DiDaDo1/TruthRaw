#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "suite_android/app/src/main/cpp"
JAVA = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"

header = (CPP / "truthnegative_pipeline_bridge_common.h").read_text()
common = (CPP / "truthnegative_pipeline_bridge_common.cpp").read_text()
service = (JAVA / "TruthRawMediaProcessingForegroundService.kt").read_text()
factored_kt = (JAVA / "TruthNegativeN2FactoredConfidence.kt").read_text()

for needle in [
    "std::shared_ptr<std::mutex> useMutex",
    "std::shared_ptr<int> ownedSourceFd",
    "Status acquireShared(",
    "void clearSharedCache() noexcept",
]:
    assert needle in header, f"missing shared-context header invariant: {needle}"

for needle in [
    "std::shared_ptr<Context> gSharedContext",
    "duplicate_owned_fd",
    "seal_source_sha256(",
    "same_source_seal(",
    "gSharedContext.reset()",
    "Status acquireShared(",
    "void clearSharedCache() noexcept",
]:
    assert needle in common, f"missing shared-context implementation invariant: {needle}"

# One process-local slot only: no source map / unbounded context collection.
for forbidden in [
    "unordered_map<",
    "std::map<",
    "std::vector<std::shared_ptr<Context>>",
]:
    assert forbidden not in common, f"unbounded shared-context structure detected: {forbidden}"

bridges = [
    "truthnegative_n2_factored_confidence_bridge.cpp",
    "truthnegative_n2_structure_support_bridge.cpp",
    "truthnegative_n2_support_distance_bridge.cpp",
    "anchor_constrained_local_reconstruction_bridge.cpp",
]
for name in bridges:
    text = (CPP / name).read_text()
    for needle in [
        "pipeline::acquireShared(",
        "std::lock_guard<std::mutex> sharedContextUse(*ctx->useMutex)",
        "pipeline::reverify(*ctx)",
        "sharedPipelineCacheHit",
        "scientificWritebackAllowed",
        "candidateApplied",
        "createsNewEvidence",
    ]:
        assert needle in text, f"{name}: missing invariant: {needle}"

assert "clearSharedPipelineCache" in factored_kt
for needle in [
    "finally {",
    ".clearSharedPipelineCache()",
    "UniversalSourceProfiler.profile(",
]:
    assert needle in service, f"service missing bounded-cache lifecycle invariant: {needle}"

# Optimization is preparation reuse only; the research service must still
# execute one UniversalSourceProfiler profile at a time.
assert "Thread({" in service
assert "session.jobs.forEachIndexed" in service
assert "scientific_writeback_allowed" in (JAVA / "UniversalSourceProfiler.kt").read_text()

print("shared_scientific_context_v0_1_integrity=PASS")
