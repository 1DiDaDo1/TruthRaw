#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
CPP = ROOT / "app/android/truthraw-adaptive-ui-v01/app/src/main/cpp/source_bound_color_preview_bridge.cpp"
CMAKE = ROOT / "app/android/truthraw-adaptive-ui-v01/app/src/main/cpp/CMakeLists.txt"
KOTLIN = ROOT / "app/android/truthraw-adaptive-ui-v01/app/src/main/java/com/truthraw/adaptiveui/NativeTilePreview.kt"
ACTIVITY = ROOT / "app/android/truthraw-adaptive-ui-v01/app/src/main/java/com/truthraw/adaptiveui/MainActivity.kt"

cpp = CPP.read_text(encoding="utf-8")
cmake = CMAKE.read_text(encoding="utf-8")
kotlin = KOTLIN.read_text(encoding="utf-8")
activity = ACTIVITY.read_text(encoding="utf-8")

required_cpp = [
    "seal_source_sha256",
    "produce_source_metadata_color_binding",
    "prepare_scientific_color_source",
    "sourceBoundAppearanceReleaseAllowed",
    "scientificPreviewReleaseAllowed",
    "scientificClaimAllowed",
    "TileNativeDngSource::open",
    "StreamingTruthRawProcessor",
    "ResearchEdgeAwareMeasuredPreservingReconstruction",
    "NeutralReferenceAppearance",
    "BoundedSrgbPreviewSink",
    "adapterOwnsFullRawFrame",
    "physicalFrameCount",
    "independentEvidenceCount",
]
for token in required_cpp:
    assert token in cpp, f"missing native contract token: {token}"

for forbidden in [
    "PreviewSentinel",
    "scientificMasterHash.fill",
    "zeroLineHash.fill",
    "sceneScaleHash.fill",
    "readBytes(",
    "mmap(",
]:
    assert forbidden not in cpp, f"forbidden native authority/materialization token: {forbidden}"

required_cmake = [
    "dng_color_binding_producer_v0_1.cpp",
    "scientific_preview_source_binding_v0_1.cpp",
    "scientific_preview_source_binding_v0_2.cpp",
    "full_frame_streaming_v0_1_pass1.cpp",
    "full_frame_streaming_v0_1_pass2.cpp",
    "bounded_srgb_preview_sink_v0_1.cpp",
    "technical_backplane_v0_1.cpp",
    "core.cpp",
    "target_compile_options(truthraw_ui_preview_bridge PRIVATE -Wall -Wextra -Werror)",
]
for token in required_cmake:
    assert token in cmake, f"missing CMake integration: {token}"

# Frozen canonical v4.7i contains one compact line rejected by Android Clang 18
# under -Werror=misleading-indentation. The compatibility exception must remain
# exactly source-scoped to core.cpp; a global warning downgrade is forbidden.
compat = '-Wno-error=misleading-indentation'
assert cmake.count(compat) == 1, "canonical warning adapter must occur exactly once"
assert "set_source_files_properties(" in cmake
assert "${V47I_NATIVE}/src/core.cpp" in cmake
assert "PROPERTIES COMPILE_OPTIONS \"-Wno-error=misleading-indentation\"" in cmake
assert f"target_compile_options(truthraw_ui_preview_bridge PRIVATE {compat}" not in cmake
assert "-Wno-error" not in cmake.replace(compat, ""), "no additional warning downgrade is allowed"

required_kotlin = [
    "buildSourceBoundColorPreview",
    "buildFinalizedScientificColorPreview",
    "sourceBoundAppearanceReleaseAllowed",
    "scientificPreviewReleaseAllowed",
    "scientificClaimAllowed",
    "physicalFrameCount",
    "independentEvidenceCount",
    "PortablePreviewEncoder.createSrgbBitmap",
]
for token in required_kotlin:
    assert token in kotlin, f"missing Kotlin contract token: {token}"

# The source-bound path is retained as a diagnostic/pre-master route, but the
# default UI must consume the finalized route. This replaces old assertions on
# presentation copy, which were brittle and no longer represented authority.
assert "NativeTilePreviewBridge.buildCfaPreview(" not in kotlin, \
    "UI loader must not silently fall back to gray sentinel proxy"
assert "NativeTilePreviewBridge.buildFinalizedScientificColorPreview(" in kotlin, \
    "default UI loader must use the finalized Scientific Preview route"
assert "Pre-master fallback/diagnostic path" in kotlin, \
    "source-bound path must stay explicitly diagnostic/pre-master"
assert "Finalized Scientific Preview" in activity
assert "Scientific Master/TruthRange/Backplane-lineage" in activity
assert "PortablePreviewEncoder.encodeJpeg" in activity
assert "Source-bound Main House kleurpreview" not in activity, \
    "obsolete pre-master label must not be presented as the current finalized UI"

print("ANDROID_SOURCE_BOUND_COLOR_PREVIEW_V0_1_CONTRACT_PASS")
print("color_authority=SOURCE_METADATA_BOUND")
print("source_bound_path=DIAGNOSTIC_PRE_MASTER_ONLY")
print("default_ui_route=FINALIZED_SCIENTIFIC_PREVIEW")
print("scientific_claim=AUTHORITY_GATED")
print("full_raw_materialization=FORBIDDEN")
print("preview_sentinel_fallback=AUTO_DISABLED")
print("canonical_v4_7i_bytes=UNCHANGED")
print("canonical_android_clang_warning_adapter=SOURCE_SCOPED_ONLY")
print("strict_werror=RETAINED_FOR_INTEGRATION_SOURCES")
print("physical_frame_count=1 independent_evidence_count=1")
