#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
JAVA = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"
CPP = ROOT / "suite_android/app/src/main/cpp"
V01_H = ROOT / "docs/research/truthnegative-n2-cfa-audit-v0.1/native/truthnegative_n2_cfa_audit_v0_1.h"
V01_CPP = ROOT / "docs/research/truthnegative-n2-cfa-audit-v0.1/native/truthnegative_n2_cfa_audit_v0_1.cpp"
V022_H = ROOT / "docs/research/truthnegative-center-excluded-spatial-audit-v0.2.2/native/truthnegative_center_excluded_spatial_audit_v0_2_2.h"
V022_CPP = ROOT / "docs/research/truthnegative-center-excluded-spatial-audit-v0.2.2/native/truthnegative_center_excluded_spatial_audit_v0_2_2.cpp"

v01h = V01_H.read_text()
v01cpp = V01_CPP.read_text()
v022h = V022_H.read_text()
v022cpp = V022_CPP.read_text()
bridge = (CPP / "truthnegative_n2_factored_confidence_bridge.cpp").read_text()
cmake = (CPP / "CMakeLists.txt").read_text()
profiler = (JAVA / "UniversalSourceProfiler.kt").read_text()
binding = (JAVA / "N2LocalSpatialBindingAudit.kt").read_text()
diag = (JAVA / "ResearchPerformanceDiagnosticsV01.kt").read_text()

for token in [
    "CorrectedSampleCoordinate",
    "correctedSampleOffset",
    "correctedSampleCount",
    "correctedSampleCoordinates",
    "correctedSampleCoordinatesComplete",
]:
    assert token in v01h, f"v0.1 sparse runtime index missing {token}"

for token in [
    "out.correctedSampleCoordinates.push_back",
    "tile.correctedSampleCount",
    "out.correctedSampleCoordinates.size()!=out.audit.corrected",
    "kMaxSparseCorrectedSampleCoordinates",
    "out.correctedSampleCoordinatesComplete=false",
]:
    assert token in v01cpp, f"v0.1 sparse capture missing {token}"

# The new runtime index must not become part of any established v0.1 hash.
hash_region = v01cpp[v01cpp.index("truthraw::sha256_v0_69::Hasher candidateHasher"):]
for forbidden in [
    "candidateHasher.update(out.correctedSampleCoordinates",
    "spatialHasher.update(out.correctedSampleCoordinates",
    "auditHasher.update(out.correctedSampleCoordinates",
    "hash_u32(candidateHasher,point.x",
    "hash_u32(spatialHasher,point.x",
    "hash_u32(auditHasher,point.x",
]:
    assert forbidden not in hash_region, f"sparse runtime index leaked into v0.1 hash: {forbidden}"

for token in [
    "runSparseReference",
    "const v021::Binding& binding",
    "v021::Result& out",
]:
    assert token in v022h, f"v0.2.2 stable ABI contract missing {token}"

assert "v01::run(" not in v022cpp, "v0.2.2 must not rerun v0.1"
for token in [
    "referenceV01.correctedSampleCoordinates",
    "!referenceV01.correctedSampleCoordinatesComplete",
    "sparseCount!=referenceTile.audit.corrected",
    "D_RAW_TN_N2_CENTER_EXCLUDED_SPATIAL_AUDIT_V0_2_1",
    "out.v01TileParityVerified=true",
    "out.candidateApplied=false",
    "out.createsNewEvidence=false",
    "out.scientificWritebackAllowed=false",
]:
    assert token in v022cpp, f"v0.2.2 fail-closed/equivalence contract missing {token}"

for token in [
    '#include "truthnegative_center_excluded_spatial_audit_v0_2_2.h"',
    "v01.correctedSampleCoordinatesComplete",
    "ce_sparse::runSparseReference",
    "ce_spatial::run",
    "ce_spatial::Result ceAudit{}",
    "confidence::derive(",
    "factored::encode(",
    "v01SparseReferenceReuseVerified",
    "v01RerunPerformed",
    "v01SparseReferenceIndexComplete",
    '(v01SparseReferenceReuseVerified?"true":"false")',
    '(v01RerunPerformed?"true":"false")',
    '(v01.correctedSampleCoordinatesComplete?"true":"false")',
]:
    assert token in bridge, f"Android bridge sparse route missing {token}"

for token in [
    "TRUTHNEGATIVE_CENTER_EXCLUDED_SPATIAL_V022",
    "truthnegative_center_excluded_spatial_audit_v0_2_2.cpp",
]:
    assert token in cmake, f"Android native build missing {token}"

assert "N2_LOCAL_SPATIAL_V01_R7_AUTHORITY_DIRECT_STREAM" in profiler
for token in [
    "v01_sparse_reference_reuse_verified",
    "v01_rerun_performed",
    "v01_sparse_reference_index_complete",
    "row_band_reuse_active",
    "row_band_scientific_values_modified",
]:
    assert token in binding, f"profile telemetry missing {token}"

for token in [
    "sparse_reference_telemetry_reported",
    "v01_sparse_reference_reuse_verified",
    "v01_rerun_performed",
    "v01_sparse_reference_index_complete",
    "LEGACY_V0_2_1_RERUN_IF_SPARSE_INDEX_INCOMPLETE",
    "N2_CENTER_EXCLUDED_SPATIAL_AUDIT_V0_2_1_BYTE_IDENTICAL",
    "DIAGNOSTIC_RUNTIME_ONLY",
]:
    assert token in diag, f"Foundation diagnostic route missing {token}"

print("n2_sparse_reference_reuse_v0_2_2_static_integrity=PASS")
