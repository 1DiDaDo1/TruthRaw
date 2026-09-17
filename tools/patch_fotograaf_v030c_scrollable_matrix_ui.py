#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct the proven v0.30 full-factorial experiment first, then change UI layout only.
# Scientific/acquisition behavior, matrix order, vendor writes, sealing and Stage 3.6/3.7 remain unchanged.
ns = {}
exec(
    compile(
        Path('tools/patch_fotograaf_v030b_vendor_route_full_factorial_matrix.py').read_text(),
        'patch_fotograaf_v030b_vendor_route_full_factorial_matrix.py',
        'exec',
    ),
    ns,
    ns,
)
s = p.read_text()

import_needle = 'import android.widget.LinearLayout\nimport android.widget.TextView\n'
import_replacement = 'import android.widget.LinearLayout\nimport android.widget.ScrollView\nimport android.widget.TextView\n'
if import_needle not in s:
    raise SystemExit('v0.30c ScrollView import anchor not found')
s = s.replace(import_needle, import_replacement, 1)

# A weighted zero-height TextureView assumes a bounded parent height. Once the status/evidence text
# grows, that non-scrollable layout pushes later controls below the viewport. In a ScrollView the
# preview instead gets a stable framing-only height; it has no evidence authority.
preview_needle = '        root.addView(preview, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f))\n'
preview_replacement = '        root.addView(preview, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(220)))\n'
if preview_needle not in s:
    raise SystemExit('v0.30c preview layout anchor not found')
s = s.replace(preview_needle, preview_replacement, 1)

return_needle = '''        return root\n    }\n\n    private fun matrixProfileLabel(runIndex: Int): String {\n'''
return_replacement = '''        return ScrollView(this).apply {\n            isFillViewport = true\n            isVerticalScrollBarEnabled = true\n            addView(\n                root,\n                ViewGroup.LayoutParams(\n                    ViewGroup.LayoutParams.MATCH_PARENT,\n                    ViewGroup.LayoutParams.WRAP_CONTENT,\n                ),\n            )\n        }\n    }\n\n    private fun matrixProfileLabel(runIndex: Int): String {\n'''
if return_needle not in s:
    raise SystemExit('v0.30c buildUi return anchor not found')
s = s.replace(return_needle, return_replacement, 1)

# UI-only invariants and scientific invariants.
assert 'import android.widget.ScrollView' in s
assert 'return ScrollView(this).apply {' in s
assert 'ViewGroup.LayoutParams.MATCH_PARENT, dp(220)' in s
assert 'Camera2VendorRouteFullFactorialMatrix.applyProfile(' in s
assert 'FULL_FACTORIAL_2_LEVEL_4_FACTOR_16_RUN_COMPLEMENT_PAIRED' in s
assert 'persistOriginalRawBuffer(image, stamp)' in s
assert s.index('Camera2PreHalGate.observeSession(') < s.index('Camera2VendorRouteFullFactorialMatrix.applyProfile(')
assert s.index('Camera2VendorRouteFullFactorialMatrix.applyProfile(') < s.index('device.isSessionConfigurationSupported(config)')
assert s.index('persistOriginalRawBuffer(image, stamp)') < s.index('Camera2EnvelopeProbe.observe(image, physical, physicalResult)')
assert s.index('RawSensorRasterAudit.audit(') < s.index('RawPayloadGeometryDecoder.decode(')
assert s.count('Camera2VendorRouteFullFactorialMatrix.applyProfile(') == 1

p.write_text(s)
print('patched scrollable v0.30 matrix UI', p)
print('bytes', p.stat().st_size)
