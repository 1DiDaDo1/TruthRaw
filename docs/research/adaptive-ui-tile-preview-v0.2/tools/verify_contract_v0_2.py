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
    assert token in native, f"missing native contract token: {token}"

required_loader = [
    'openFileDescriptor(job.source.uri, "r")',
    "MAX_PREVIEW_EDGE = 384",
    "MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024",
    "fullRawMaterialized",
    "bitmap.setPixels(packet, HEADER_INTS",
]
for token in required_loader:
    assert token in loader, f"missing Kotlin preview contract token: {token}"

for forbidden in ["openInputStream", "readBytes()", "detachFd()", "copyOfRange(HEADER_INTS"]:
    assert forbidden not in loader, f"forbidden preview ingress/workspace operation: {forbidden}"

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
print("java_second_pixel_array=FORBIDDEN")
print("scientific_color_authority=NOT_GRANTED")
print("full_raw_copy_fallback=FORBIDDEN")
