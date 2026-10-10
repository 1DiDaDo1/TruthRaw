#!/usr/bin/env python3
from pathlib import Path

TARGET = Path("suite_android/app/src/main/cpp/photo_export_bridge.cpp")


def main() -> None:
    text = TARGET.read_text(encoding="utf-8")
    marker = "        const bool naturalLightEnabled=(flags_&kFlagLight)!=0;\n"
    old = (
        marker
        + "        const float exposureGain=advanced_controls::presentation_exposure_gain(\n"
    )
    new = (
        "        // Natural Light gate is defined once with the local-field support geometry.\n"
        "        const float exposureGain=advanced_controls::presentation_exposure_gain(\n"
    )

    count = text.count(marker)
    if count == 2 and old in text:
        text = text.replace(old, new, 1)
        TARGET.write_text(text, encoding="utf-8")
        count = text.count(marker)
        print("NATURAL_LIGHT_FIELD_TONE_V0_1_DUPLICATE_GATE_FIXED")
    elif count == 1:
        print("NATURAL_LIGHT_FIELD_TONE_V0_1_GATE_ALREADY_CANONICAL")
    else:
        raise SystemExit(f"unexpected Natural Light gate count: {count}")

    if count != 1:
        raise SystemExit("Natural Light gate canonicalization failed")


if __name__ == "__main__":
    main()
