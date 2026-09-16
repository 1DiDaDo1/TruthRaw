#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')
s = p.read_text()

# Apply v0.12 first so this patch remains based on the frozen v0.11 source tree.
base_patch = Path('tools/patch_fotograaf_v012_forced_physical_max.py').read_text()
ns = {}
exec(compile(base_patch, 'patch_fotograaf_v012_forced_physical_max.py', 'exec'), ns, ns)
s = p.read_text()

# Version/provenance labels.
s = s.replace('200MP test v0.12', '200MP test v0.13')
s = s.replace('TruthRaw · 200MP Tele Test v0.12', 'TruthRaw · 200MP Tele Test v0.13')
s = s.replace('STAGE 0 PASS · v0.12 UI', 'STAGE 0 PASS · v0.13 UI')
s = s.replace('truthraw-200mp-v012', 'truthraw-200mp-v013')
s = s.replace('truthraw-v012-capability', 'truthraw-v013-capability')
s = s.replace('_v012.dng', '_v013.dng')
s = s.replace('_EVIDENCE_v012.json', '_EVIDENCE_v013.json')
s = s.replace('_v012.${if (contiguous)', '_v013.${if (contiguous)')
s = s.replace('staged-evidence.v0.12', 'staged-evidence.v0.13')
s = s.replace(
    'physical-5-scoped MAX still request with forced physical pixel-mode metadata',
    'physical-5-scoped MAX still request; physical MAX metadata is authoritative for the physical-bound high-res stream',
)
s = s.replace('Stap 3 · FORCE PHYSICAL MAX · 16320×12288', 'Stap 3 · SUBMIT PHYSICAL MAX · 16320×12288')

old = '''            if (!lastGlobalPixelModeWritten || !lastPhysicalPixelModeWritten) {
                setStatusAny(
                    "STAGE 3 BLOCKED BEFORE SUBMIT · MAX metadata kon niet exact worden gezet.\\n" +
                        "globalMAX=$lastGlobalPixelModeWritten · physicalMAX=$lastPhysicalPixelModeWritten · " +
                        "physicalOverrideAdvertised=$lastPhysicalPixelModeAdvertised" +
                        (lastPhysicalPixelModeForceError?.let { "\\nphysicalForceError=$it" } ?: ""),
                )
                runOnUiThread { previewButton.isEnabled = true }
                return
            }
'''
new = '''            // v0.12 on-device result: global SENSOR_PIXEL_MODE is not advertised by logical 0,
            // while setPhysicalCameraKey/read-back for physical 5 succeeds.  AOSP validates
            // SENSOR_PIXEL_MODE per resolved camera id against the high-resolution stream set.
            // This RAW stream is explicitly bound to physical 5, so the physical MAX metadata is
            // the consistency-critical setting.  Do not invent a global requirement that the
            // logical camera itself does not advertise.
            if (!lastPhysicalPixelModeWritten) {
                setStatusAny(
                    "STAGE 3 BLOCKED BEFORE SUBMIT · physical Camera-5 MAX metadata kon niet exact worden gezet.\\n" +
                        "globalMAX=$lastGlobalPixelModeWritten · physicalMAX=$lastPhysicalPixelModeWritten · " +
                        "physicalOverrideAdvertised=$lastPhysicalPixelModeAdvertised" +
                        (lastPhysicalPixelModeForceError?.let { "\\nphysicalForceError=$it" } ?: ""),
                )
                runOnUiThread { previewButton.isEnabled = true }
                return
            }
'''
if old not in s:
    raise SystemExit('v0.12 pre-submit gate not found')
s = s.replace(old, new, 1)

s = s.replace(
    '"STAGE 3 CAPTURE SENT · scopedRequest=true · globalMAX=true · physicalMAX=true · " +',
    '"STAGE 3 CAPTURE SENT · scopedRequest=true · globalMAX=$lastGlobalPixelModeWritten · physicalMAX=true · " +',
    1,
)

# Add a precise evidence interpretation for this experiment.
needle = '                .put("outputMaximumResolutionModeDeclared", true))\n'
replacement = (
    '                .put("outputMaximumResolutionModeDeclared", true)\n'
    '                .put("pixelModeConsistencyInterpretation", "PHYSICAL_BOUND_HIGH_RES_STREAM_REQUIRES_PHYSICAL_CAMERA_5_MAX; LOGICAL_GLOBAL_KEY_NOT_ADVERTISED_ON_DEVICE"))\n'
)
if needle not in s:
    raise SystemExit('requestTopology end marker not found')
s = s.replace(needle, replacement, 1)

assert 'TruthRaw · 200MP Tele Test v0.13' in s
assert 'if (!lastPhysicalPixelModeWritten)' in s
assert '!lastGlobalPixelModeWritten || !lastPhysicalPixelModeWritten' not in s
assert 'setPhysicalCameraKey(' in s
assert 'getPhysicalCameraKey(' in s
assert 'staged-evidence.v0.13' in s

p.write_text(s)
print('patched', p)
print('bytes', p.stat().st_size)
