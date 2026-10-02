#!/usr/bin/env python3
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "suite_android/app/src/main/cpp"
JAVA = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"

header = (CPP / "truthnegative_pipeline_bridge_common.h").read_text()
pipeline = (CPP / "truthnegative_pipeline_bridge_common.cpp").read_text()
bridge = (CPP / "truthnegative_n2_factored_confidence_bridge.cpp").read_text()
binding = (JAVA / "N2LocalSpatialBindingAudit.kt").read_text()
gradle = (ROOT / "suite_android/app/build.gradle.kts").read_text()
version_code = int((ROOT / "suite_android/VERSION_CODE").read_text().strip())
lineage = json.loads((ROOT / "state/DRAW_ANDROID_VERSION_LINEAGE_V01.json").read_text())
readme = (ROOT / "docs/research/scientific-master-bind-profile-v0.1/README.md").read_text()

for token in [
    "scientificMasterBindProfileAvailable",
    "scientificMasterSourceReadRawMs",
    "scientificMasterSourceReadRowBiasMs",
    "scientificMasterSourceReadColBiasMs",
    "scientificMasterReconstructionMs",
    "scientificMasterAuthorityObserverMs",
    "scientificMasterBindUnattributedMs",
    "scientificMasterBindTimingIsScientificEvidence",
    "scientificMasterBindTimingMayChangeScientificAuthority",
]:
    assert token in header, f"Scientific Master bind profile field missing {token}"

for token in [
    "class ScientificMasterSourceProfilingProxy final",
    "return delegate_.metadata();",
    "return delegate_.residentBytesUpperBound();",
    "delegate_.readRawTile(",
    "delegate_.readRowBias(",
    "delegate_.readColBias(",
    "class ScientificMasterReconstructionProfilingProxy final",
    "return delegate_.quality();",
    "return delegate_.name();",
    "return delegate_.requiredHalo();",
    "delegate_.reconstructTile(",
    "profiledSource",
    "profiledReconstruction",
    "bind_scientific_master_streaming_observed(",
    "authorityObserver.directRecordStreamMs()",
    "scientificMasterBindUnattributedMs",
]:
    assert token in pipeline, f"Scientific Master profiling implementation missing {token}"

# The profiling proxies must only wrap the binder call. The original objects
# remain the prepared-context objects used by the downstream Scientific Master
# tile source and dense local field adapter.
assert "out.reconstruction =" in pipeline
assert "StreamingScientificMasterTileSource>(\n                        *out.openedSource.source,\n                        *out.reconstruction)" in pipeline

for token in [
    "scientificMasterBindProfileAvailable",
    "scientificMasterSourceReadRawMs",
    "scientificMasterSourceReadRawCallCount",
    "scientificMasterReconstructionMs",
    "scientificMasterAuthorityObserverMs",
    "scientificMasterBindUnattributedMs",
    "scientificMasterBindTimingIsScientificEvidence",
    "scientificMasterBindTimingMayChangeScientificAuthority",
]:
    assert token in bridge, f"native bind profile telemetry missing {token}"

for token in [
    '"scientific_master_bind_profile_v0_1"',
    '"D.RAW/ScientificMasterBindProfile/0.1"',
    '"source_read_raw_ms"',
    '"source_read_raw_call_count"',
    '"reconstruction_ms"',
    '"authority_observer_ms"',
    '"unattributed_bind_ms"',
    '"timing_is_scientific_evidence"',
    '"timing_may_change_scientific_authority"',
]:
    assert token in binding, f"Kotlin bind profile telemetry missing {token}"

# Timing is explicitly non-authoritative at its native source.
assert "scientificMasterBindTimingIsScientificEvidence = false" in pipeline
assert "scientificMasterBindTimingMayChangeScientificAuthority = false" in pipeline

# Do not allow timing/profile names into the established scientific hashes.
for forbidden in [
    "scientificMasterBindProfileAvailable",
    "scientificMasterSourceReadRawMs",
    "scientificMasterReconstructionMs",
    "scientificMasterBindUnattributedMs",
]:
    assert forbidden not in pipeline[pipeline.index("technical_backplane_phase2::v0_1::Phase2Input phaseInput"):pipeline.index("out.width =")], (
        f"Scientific Master profile telemetry leaked into Phase2 authority region: {forbidden}"
    )

assert version_code >= 26100121
assert lineage["current_version_code"] == version_code
assert lineage["current_version_code"] > lineage["previous_version_code"]
assert f'versionName = "{lineage["version_name"]}"' in gradle
assert lineage["scientific_authority_affected"] is False

for token in [
    "diagnostic runtime profiling only",
    "not scientific evidence",
    "exact arguments",
    "original reconstruction object remains",
    "does not modify source values",
]:
    assert token.lower() in readme.lower(), f"README missing {token}"

for text in [header, pipeline, bridge, binding, readme]:
    for forbidden in [
        'candidate_applied\",true',
        'creates_new_evidence\",true',
        'scientific_writeback_allowed\",true',
        '.put("candidate_applied", true)',
        '.put("creates_new_evidence", true)',
        '.put("scientific_writeback_allowed", true)',
        '.put("calibration_promoted", true)',
        '.put("correction_authorized", true)',
    ]:
        assert forbidden not in text, f"profiling gained scientific authority: {forbidden}"

print("scientific_master_bind_profile_v0_1_static_integrity=PASS")
