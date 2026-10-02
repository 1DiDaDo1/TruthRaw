#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MODULE = ROOT / "docs/research/truthnegative-n2-row-band-reuse-v0.1"
JAVA = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"
CPP = ROOT / "suite_android/app/src/main/cpp"

header = (MODULE / "native/truthnegative_n2_row_band_reuse_v0_1.h").read_text()
test = (MODULE / "tests/test_truthnegative_n2_row_band_reuse_v0_1.cpp").read_text()
readme = (MODULE / "README.md").read_text()
bridge = (CPP / "truthnegative_n2_factored_confidence_bridge.cpp").read_text()
cmake = (CPP / "CMakeLists.txt").read_text()
binding = (JAVA / "N2LocalSpatialBindingAudit.kt").read_text()
profiler = (JAVA / "UniversalSourceProfiler.kt").read_text()
diag = (JAVA / "ResearchPerformanceDiagnosticsV01.kt").read_text()

for token in [
    "RowBandReuseTileSource",
    "maxCacheBytes",
    "bandFillCount",
    "bandServedRequestCount",
    "bandCacheHitRequestCount",
    "fallbackRequestCount",
    "peakCacheBytes",
    "source_.readRawTile",
    "std::copy_n",
    "scientificValuesModified",
    "createsNewEvidence",
    "scientificWritebackAllowed",
]:
    assert token in header, f"row-band adapter missing {token}"

for forbidden in [
    "scientificValuesModified() const noexcept {\n        return true",
    "createsNewEvidence() const noexcept {\n        return true",
    "scientificWritebackAllowed() const noexcept {\n        return true",
]:
    assert forbidden not in header, f"row-band safety invariant violated: {forbidden}"

for token in [
    "same_v01",
    "same_ce_result",
    "candidateSha256",
    "auditSha256",
    "spatialSha256",
    "appearanceGridSha256",
    "correctedSampleCoordinates",
    "cachedSource.readCalls < directSource.readCalls",
    "tinyCache",
    "fallbackRequestCount() > 0u",
]:
    assert token in test, f"row-band exact parity test missing {token}"

for token in [
    '#include "truthnegative_n2_row_band_reuse_v0_1.h"',
    "row_band::RowBandReuseTileSource n2ReadSource",
    "n2_cfa::run(\n            n2ReadSource",
    "ce_sparse::runSparseReference(\n                n2ReadSource",
    "ce_spatial::run(\n                *ctx->openedSource.source",
    "rowBandReuseActive",
    "v01RowBandFillCount",
    "rowBandFillCountTotal",
    "rowBandPeakCacheBytes",
    "rowBandScientificValuesModified",
]:
    assert token in bridge, f"Android bridge row-band route missing {token}"

for token in [
    "TRUTHNEGATIVE_N2_ROW_BAND_REUSE_V01",
    "truthnegative-n2-row-band-reuse-v0.1/native",
]:
    assert token in cmake, f"Android CMake row-band route missing {token}"

for token in [
    "row_band_reuse_active",
    "v01_row_band_fill_count",
    "v01_row_band_served_request_count",
    "v01_row_band_cache_hit_request_count",
    "v01_row_band_fallback_request_count",
    "row_band_fill_count_total",
    "row_band_peak_cache_bytes",
    "row_band_scientific_values_modified",
]:
    assert token in binding, f"Kotlin row-band telemetry missing {token}"

for token in [
    "D.RAW/UniversalSourceProfileCache/0.2.4-n2-phase-timing-v1",
    "N2_LOCAL_SPATIAL_V01_R4_PHASE_TIMING",
]:
    assert token in profiler, f"row-band profiler identity missing {token}"

for token in [
    "row_band_telemetry_reported",
    "row_band_reuse_active",
    "v01_row_band_fill_count",
    "row_band_fill_count_total",
    "row_band_peak_cache_bytes",
    "row_band_scientific_values_modified",
    "DIAGNOSTIC_RUNTIME_ONLY",
]:
    assert token in diag, f"row-band performance diagnostics missing {token}"

for token in [
    "transport",
    "runtime-only",
    "never evidence",
    "never persisted",
]:
    assert token.lower() in readme.lower(), f"row-band README missing {token}"

# This is a transport-only optimization. Do not permit any new scientific
# authority or writeback capability in the changed telemetry layers.
for text in [binding, diag]:
    for forbidden in [
        '.put("calibration_promoted", true)',
        '.put("correction_authorized", true)',
        '.put("candidate_applied", true)',
        '.put("creates_new_evidence", true)',
        '.put("scientific_writeback_allowed", true)',
    ]:
        assert forbidden not in text, f"row-band transport gained authority: {forbidden}"

print("n2_row_band_reuse_v0_2_3_static_integrity=PASS")
