#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
APP = ROOT / "app/android/truthraw-adaptive-ui-v01/app/src/main"
KOTLIN = APP / "java/com/truthraw/adaptiveui"
CPP = APP / "cpp"

native = (CPP / "native_tile_preview_bridge.cpp").read_text()
loader = (KOTLIN / "NativeTilePreview.kt").read_text()
ingress = (KOTLIN / "IngressModels.kt").read_text()
cmake = (CPP / "CMakeLists.txt").read_text()
gradle = (ROOT / "app/android/truthraw-adaptive-ui-v01/app/build.gradle.kts").read_text()

# The historical v0.2 diagnostic bridge remains required even when a newer
# source-bound color route owns the active UI. Its sentinel stays diagnostic
# only and may never be promoted into scientific color authority.
required_native = [
    "PosixFdByteSource",
    "TileNativeDngSource::open",
    "readRawTile",
    "kChunkSamples = 1024",
    "kAbsoluteMaxPreviewEdge = 512",
    "fullRawMaterialized",
    "ui-preview-parser-sentinel-not-scientific-v0.2",
]
for token in required_native:
    assert token in native, f"missing historical native diagnostic contract token: {token}"

required_common_loader = [
    'openFileDescriptor(job.source.uri, "r")',
    "MAX_PREVIEW_EDGE = 384",
    "MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024",
    "fullRawMaterialized",
]
for token in required_common_loader:
    assert token in loader, f"missing common Kotlin preview contract token: {token}"

for forbidden in ["openInputStream", "readBytes()", "detachFd()", "copyOfRange(HEADER_INTS"]:
    assert forbidden not in loader, f"forbidden preview ingress/workspace operation: {forbidden}"

active_source_bound = "buildSourceBoundColorPreview" in loader
if active_source_bound:
    # v0.2 remains as an available gray diagnostic implementation, but the
    # active UI is explicitly superseded by v0.1 source-bound color preview.
    new_contract = ROOT / "docs/research/android-source-bound-color-preview-v0.1/tools/verify_contract_v0_1.py"
    new_readme = ROOT / "docs/research/android-source-bound-color-preview-v0.1/README.md"
    assert new_contract.is_file(), "active source-bound route requires its own contract verifier"
    assert new_readme.is_file(), "active source-bound route requires its own research boundary document"
    for token in [
        "PortablePreviewEncoder.createSrgbBitmap",
        "sourceBoundAppearanceReleaseAllowed",
        "scientificPreviewReleaseAllowed",
        "scientificClaimAllowed",
        "physicalFrameCount",
        "independentEvidenceCount",
    ]:
        assert token in loader, f"missing source-bound supersession token: {token}"
    assert "NativeTilePreviewBridge.buildCfaPreview(" not in loader, (
        "active UI must not silently fall back to the historical gray sentinel proxy"
    )
    active_route = "SOURCE_BOUND_COLOR_PREVIEW_V0_1"
else:
    # Exact historical UI proof path retained for branches that have not yet
    # adopted the explicit source-bound color successor.
    assert "bitmap.setPixels(packet, HEADER_INTS" in loader, (
        "legacy v0.2 route must retain its direct bitmap copy contract"
    )
    active_route = "LEGACY_GRAY_CFA_DIAGNOSTIC_V0_2"

assert "payload buffer" in ingress
assert "tile_native_dng_source_v0_1.cpp" in cmake
assert 'abiFilters += listOf("arm64-v8a")' in gradle
assert 'ndkVersion = "27.2.12479018"' in gradle

print("ADAPTIVE_UI_TILE_PREVIEW_V0_2_CONTRACT_PASS")
print("uri_to_fd=OPEN_FILE_DESCRIPTOR_BORROWED")
print("tile_source=UPSTREAM_TILE_NATIVE_DNG_V0_1")
print("caller_raw_workspace_samples=1024")
print("preview_edge_dp_independent_max=384")
print("native_absolute_preview_edge_max=512")
print("source_resident_cap_bytes=8388608")
print("legacy_diagnostic_scientific_color_authority=NOT_GRANTED")
print(f"active_ui_route={active_route}")
print("full_raw_copy_fallback=FORBIDDEN")
