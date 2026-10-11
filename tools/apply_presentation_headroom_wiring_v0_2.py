#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
JAVA = ROOT / "suite_android" / "app" / "src" / "main" / "java" / "com" / "truthraw" / "adaptiveui"
CPP = ROOT / "suite_android" / "app" / "src" / "main" / "cpp" / "photo_export_bridge.cpp"

MODE_FILE = JAVA / "PresentationHeadroomModeV01.kt"
BINDING_FILE = JAVA / "DrawPhotoOutputCableV01.kt"
EXPORT_FILE = JAVA / "FullResJpegExport.kt"
MAIN_FILE = JAVA / "MainActivity.kt"

MODE_TEXT = '''package com.truthraw.adaptiveui

/**
 * Downstream presentation/output selector only.
 *
 * This value must never be interpreted as scientific authority, sensor headroom,
 * recovered measurement, or permission to write back into Scientific Master.
 */
object PresentationHeadroomModeV01 {
    const val OFF = 0
    const val PURE_MAP_90_TO_100 = 1

    fun forPhotoOutputRoute(route: String): Int? = when (route) {
        TruthRawSuiteLauncherActivity.OUTPUT_PURE -> PURE_MAP_90_TO_100
        TruthRawSuiteLauncherActivity.OUTPUT_ADVANCED,
        TruthRawSuiteLauncherActivity.OUTPUT_PRO -> OFF
        else -> null
    }

    fun isKnown(mode: Int): Boolean =
        mode == OFF || mode == PURE_MAP_90_TO_100
}
'''


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        print(f"FAIL: {label}: expected exactly one anchor, found {count}", file=sys.stderr)
        raise SystemExit(2)
    return text.replace(old, new, 1)


MODE_FILE.write_text(MODE_TEXT, encoding="utf-8")

binding = BINDING_FILE.read_text(encoding="utf-8")
if "val presentationHeadroomMode: Int," not in binding:
    binding = replace_once(
        binding,
        "    val userQuarterTurns: Int,\n    val adapterId: String,\n",
        "    val userQuarterTurns: Int,\n    val presentationHeadroomMode: Int,\n    val adapterId: String,\n",
        "binding field",
    )
    binding = replace_once(
        binding,
        "                userQuarterTurns = userQuarterTurns,\n                adapterId = JPEG_FULL_RES_ADAPTER,\n",
        "                userQuarterTurns = userQuarterTurns,\n"
        "                presentationHeadroomMode = PresentationHeadroomModeV01.forPhotoOutputRoute(route)\n"
        "                    ?: return DrawPhotoOutputBindResultV01.Failed(\n"
        "                        \"JPG-output geblokkeerd: onbekende presentation-headroom route.\",\n"
        "                    ),\n"
        "                adapterId = JPEG_FULL_RES_ADAPTER,\n",
        "binding construction",
    )
    binding = replace_once(
        binding,
        "        if (binding.createsNewEvidence ||\n            binding.scientificWritebackAllowed ||\n            binding.sourceMutationAllowed ||\n            binding.previewRequired\n        ) {\n",
        "        val expectedHeadroomMode = PresentationHeadroomModeV01.forPhotoOutputRoute(binding.route)\n"
        "            ?: return \"JPG-output geblokkeerd: onbekende presentation-headroom route in binding.\"\n"
        "        if (!PresentationHeadroomModeV01.isKnown(binding.presentationHeadroomMode) ||\n"
        "            binding.presentationHeadroomMode != expectedHeadroomMode\n"
        "        ) {\n"
        "            return \"JPG-output geblokkeerd: presentation-headroom contract mismatch.\"\n"
        "        }\n"
        "        if (binding.createsNewEvidence ||\n            binding.scientificWritebackAllowed ||\n            binding.sourceMutationAllowed ||\n            binding.previewRequired\n        ) {\n",
        "binding safety validation",
    )
    binding = replace_once(
        binding,
        "        if (currentRoute != binding.route) {\n            return \"JPG-output geblokkeerd: uitvoerroute veranderde tijdens de bestandsdialoog.\"\n        }\n        if (currentRouteFlags != binding.routeFlags) {\n",
        "        if (currentRoute != binding.route) {\n            return \"JPG-output geblokkeerd: uitvoerroute veranderde tijdens de bestandsdialoog.\"\n        }\n"
        "        val currentHeadroomMode = PresentationHeadroomModeV01.forPhotoOutputRoute(currentRoute)\n"
        "            ?: return \"JPG-output geblokkeerd: onbekende presentation-headroom route tijdens de bestandsdialoog.\"\n"
        "        if (currentHeadroomMode != binding.presentationHeadroomMode) {\n"
        "            return \"JPG-output geblokkeerd: presentation-headroom contract veranderde tijdens de bestandsdialoog.\"\n"
        "        }\n"
        "        if (currentRouteFlags != binding.routeFlags) {\n",
        "binding freshness validation",
    )
    BINDING_FILE.write_text(binding, encoding="utf-8")

export = EXPORT_FILE.read_text(encoding="utf-8")
if "presentationHeadroomMode: Int," not in export:
    export = replace_once(
        export,
        "        userQuarterTurns: Int,\n        maxSourceResidentBytes: Int,\n",
        "        userQuarterTurns: Int,\n        presentationHeadroomMode: Int,\n        maxSourceResidentBytes: Int,\n",
        "JNI headroom argument",
    )
    export = replace_once(
        export,
        "        workingDir: File,\n    ): FullResJpegResult {\n        if (!job.source.format.nativeProcessingReady || job.source.format.id != \"DNG\") {\n",
        "        workingDir: File,\n        presentationHeadroomMode: Int = PresentationHeadroomModeV01.OFF,\n    ): FullResJpegResult {\n        if (!job.source.format.nativeProcessingReady || job.source.format.id != \"DNG\") {\n",
        "exporter headroom parameter",
    )
    export = replace_once(
        export,
        "            return FullResJpegResult.Failed(\"Full-resolution JPG is alleen beschikbaar voor de admitted DNG-route.\")\n        }\n        FullResPresentationRasterRegistryV01.clear()\n",
        "            return FullResJpegResult.Failed(\"Full-resolution JPG is alleen beschikbaar voor de admitted DNG-route.\")\n"
        "        }\n"
        "        if (!PresentationHeadroomModeV01.isKnown(presentationHeadroomMode)) {\n"
        "            return FullResJpegResult.Failed(\"JPG: onbekende presentation-headroom modus.\")\n"
        "        }\n"
        "        FullResPresentationRasterRegistryV01.clear()\n",
        "exporter headroom validation",
    )
    export = replace_once(
        export,
        "                        userQuarterTurns,\n                        MAX_SOURCE_RESIDENT_BYTES,\n",
        "                        userQuarterTurns,\n                        presentationHeadroomMode,\n                        MAX_SOURCE_RESIDENT_BYTES,\n",
        "JNI headroom forwarding",
    )
    EXPORT_FILE.write_text(export, encoding="utf-8")

cpp = CPP.read_text(encoding="utf-8")
if "jint presentationHeadroomMode" not in cpp:
    cpp = replace_once(
        cpp,
        "constexpr jint kAllowedFlags = static_cast<jint>(advanced_controls::kAllowedFlags);\nconstexpr int kTileCore = 128;\n",
        "constexpr jint kAllowedFlags = static_cast<jint>(advanced_controls::kAllowedFlags);\n"
        "constexpr jint kPresentationHeadroomOff = 0;\n"
        "constexpr jint kPresentationHeadroomPureMap90To100 = 1;\n"
        "constexpr int kTileCore = 128;\n",
        "native headroom constants",
    )
    cpp = replace_once(
        cpp,
        "    JNIEnv* env, jobject, jint sourceFd, jint outputFd, jint flags, jint sourceRouteCode,\n"
        "    jint userQuarterTurns, jint maxSourceResidentBytes, jint maxLogicalResidentBytes) {\n"
        "    if (sourceFd<0 || outputFd<0 || maxSourceResidentBytes<=0 || maxLogicalResidentBytes<=0 ||\n"
        "        userQuarterTurns<0 || userQuarterTurns>3 ||\n"
        "        (flags&~kAllowedFlags)!=0 || (sourceRouteCode!=0 && sourceRouteCode!=1)) {\n",
        "    JNIEnv* env, jobject, jint sourceFd, jint outputFd, jint flags, jint sourceRouteCode,\n"
        "    jint userQuarterTurns, jint presentationHeadroomMode,\n"
        "    jint maxSourceResidentBytes, jint maxLogicalResidentBytes) {\n"
        "    if (sourceFd<0 || outputFd<0 || maxSourceResidentBytes<=0 || maxLogicalResidentBytes<=0 ||\n"
        "        userQuarterTurns<0 || userQuarterTurns>3 ||\n"
        "        (presentationHeadroomMode!=kPresentationHeadroomOff &&\n"
        "         presentationHeadroomMode!=kPresentationHeadroomPureMap90To100) ||\n"
        "        (presentationHeadroomMode==kPresentationHeadroomPureMap90To100 && flags!=0) ||\n"
        "        (flags&~kAllowedFlags)!=0 || (sourceRouteCode!=0 && sourceRouteCode!=1)) {\n",
        "native JNI selector argument",
    )
    cpp = replace_once(
        cpp,
        "    const bool pureExtendedLinearHeadroomCandidate=(flags==0);\n",
        "    // Selection is an explicit downstream output contract. flags==0 alone is\n"
        "    // not sufficient: scientific/helper renders also legitimately carry zero\n"
        "    // appearance flags and must not silently enter the PURE headroom path.\n"
        "    const bool pureExtendedLinearHeadroomCandidate=\n"
        "        presentationHeadroomMode==kPresentationHeadroomPureMap90To100;\n",
        "native explicit selector",
    )
    CPP.write_text(cpp, encoding="utf-8")

main = MAIN_FILE.read_text(encoding="utf-8")
if "presentationHeadroomMode = binding.presentationHeadroomMode" not in main:
    main = replace_once(
        main,
        "                val rendered = FullResJpegExporter.renderToPrivateJpeg(\n"
        "                    contentResolver, job, flags, quarterTurns, dir,\n"
        "                )\n",
        "                val rendered = FullResJpegExporter.renderToPrivateJpeg(\n"
        "                    contentResolver, job, flags, quarterTurns, dir,\n"
        "                    presentationHeadroomMode = binding.presentationHeadroomMode,\n"
        "                )\n",
        "product JPEG explicit headroom forwarding",
    )
    MAIN_FILE.write_text(main, encoding="utf-8")

print("PRESENTATION_HEADROOM_WIRING_V0_2_PATCH_APPLIED")
