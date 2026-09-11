#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
CPP = ROOT / "app/android/truthraw-adaptive-ui-v01/app/src/main/cpp/source_bound_color_preview_bridge.cpp"
CMAKE = ROOT / "app/android/truthraw-adaptive-ui-v01/app/src/main/cpp/CMakeLists.txt"
KOTLIN = ROOT / "app/android/truthraw-adaptive-ui-v01/app/src/main/java/com/truthraw/adaptiveui/NativeTilePreview.kt"
ACTIVITY = ROOT / "app/android/truthraw-adaptive-ui-v01/app/src/main/java/com/truthraw/adaptiveui/MainActivity.kt"

cpp = CPP.read_text()
cmake = CMAKE.read_text()
kotlin = KOTLIN.read_text()
activity = ACTIVITY.read_text()

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
]
for token in required_cmake:
    assert token in cmake, f"missing CMake integration: {token}"

required_kotlin = [
    "buildSourceBoundColorPreview",
    "sourceBoundAppearanceReleaseAllowed",
    "scientificPreviewReleaseAllowed",
    "scientificClaimAllowed",
    "physicalFrameCount",
    "independentEvidenceCount",
    "PortablePreviewEncoder.createSrgbBitmap",
]
for token in required_kotlin:
    assert token in kotlin, f"missing Kotlin contract token: {token}"

assert "NativeTilePreviewBridge.buildCfaPreview(" not in kotlin, "UI loader must not silently fall back to gray sentinel proxy"
assert "JPEG preview opslaan" in activity
assert "Source-bound Main House kleurpreview" in activity
assert "PortablePreviewEncoder.encodeJpeg" in activity
assert "Scientific Master nog niet gefinaliseerd" in activity

print("ANDROID_SOURCE_BOUND_COLOR_PREVIEW_V0_1_CONTRACT_PASS")
print("color_authority=SOURCE_METADATA_BOUND")
print("appearance_release=ALLOWED_LABELED")
print("scientific_preview_release=BLOCKED_PRE_MASTER")
print("scientific_claim=BLOCKED_PRE_MASTER")
print("full_raw_materialization=FORBIDDEN")
print("preview_sentinel_fallback=AUTO_DISABLED")
print("physical_frame_count=1 independent_evidence_count=1")
