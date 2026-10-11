from pathlib import Path

path = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/SourceCapabilityEnvelopeV02.kt')
text = path.read_text(encoding='utf-8')

key = '"radiometric_linearization_table_v0_1"'
if key in text:
    print('LinearizationTable runtime diagnostic already exposed')
    raise SystemExit(0)

anchor = '''        envelope.put(
            "effective_black_normalization_v0_1",
            SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.describe(
                sourceSha256 = sourceSha256,
                parsedContainerMetadata = parsedContainerMetadata,
            ),
        )
        return envelope.put(
            "dng_version_compatibility_v0_1",
'''
replacement = '''        envelope.put(
            "effective_black_normalization_v0_1",
            SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.describe(
                sourceSha256 = sourceSha256,
                parsedContainerMetadata = parsedContainerMetadata,
            ),
        )
        envelope.put(
            "radiometric_linearization_table_v0_1",
            SourceCapabilityRadiometricLinearizationTableValidatorV01.describe(
                sourceSha256 = sourceSha256,
                parsedContainerMetadata = parsedContainerMetadata,
            ),
        )
        return envelope.put(
            "dng_version_compatibility_v0_1",
'''

if text.count(anchor) != 1:
    raise SystemExit(f'expected exactly one exposure anchor, found {text.count(anchor)}; fail closed')

new_text = text.replace(anchor, replacement, 1)
if new_text == text:
    raise SystemExit('no change produced; fail closed')

path.write_text(new_text, encoding='utf-8')
print('LinearizationTable runtime diagnostic exposure patched')
