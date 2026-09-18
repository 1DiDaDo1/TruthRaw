#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct v0.38 exactly, then change UI placement only.
ns = {}
exec(
    compile(
        Path('tools/patch_fotograaf_v038_physical_only_int32_candidate_sweep.py').read_text(),
        'patch_fotograaf_v038_physical_only_int32_candidate_sweep.py',
        'exec',
    ),
    ns,
    ns,
)
s = p.read_text()

# Android 15/16 edge-to-edge can place the fixed control deck underneath the status bar.
# Move the complete app content inside system-bar insets and explicitly keep the control
# deck above the scrolling TextureView layer. No capture/scientific code is touched.
if 'import android.view.WindowInsets' not in s:
    s = s.replace('import android.view.ViewGroup\n', 'import android.view.ViewGroup\nimport android.view.WindowInsets\n', 1)

old = '''        return LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(Color.rgb(10, 12, 15))
            addView(
                fixedControls,
                LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                ),
            )
            addView(
                detailScroll,
                LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    0,
                    1f,
                ),
            )
        }
'''
new = '''        val outer = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(Color.rgb(10, 12, 15))
            addView(
                fixedControls,
                LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                ),
            )
            addView(
                detailScroll,
                LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    0,
                    1f,
                ),
            )
        }

        // Touch-safe header for Android edge-to-edge:
        // 1) never place the fixed deck underneath status/navigation system bars;
        // 2) keep it above TextureView/ScrollView composition if their bounds ever overlap.
        fixedControls.elevation = dp(24).toFloat()
        fixedControls.bringToFront()
        outer.setOnApplyWindowInsetsListener { view, insets ->
            val bars = insets.getInsets(WindowInsets.Type.systemBars())
            view.setPadding(0, bars.top, 0, bars.bottom)
            insets
        }
        outer.requestApplyInsets()
        return outer
'''
if old not in s:
    raise SystemExit('v0.38b outer-layout anchor not found')
s = s.replace(old, new, 1)

s = s.replace(
    'TruthRaw · 200MP Tele Test v0.38 · physical-only INT32 candidate sweep',
    'TruthRaw · 200MP Tele Test v0.38b · touch-safe physical-only INT32 candidate sweep',
    1,
)

# UI-only provenance marker in status/header text.
s = s.replace(
    '3-run physical-only INT32 candidate sweep; v0.36 candidates I/J/K test numeric 1 one key at a time;',
    'touch-safe controls; 3-run physical-only INT32 candidate sweep; v0.36 candidates I/J/K test numeric 1 one key at a time;',
    1,
)

# Preserve every scientific/source-first invariant from v0.38.
assert 'WindowInsets.Type.systemBars()' in s
assert 'fixedControls.bringToFront()' in s
assert 'fixedControls.elevation = dp(24).toFloat()' in s
assert 'Camera2PhysicalOnlyInt32CandidateSweep.applyProfile(' in s
assert s.count('Camera2PhysicalOnlyInt32CandidateSweep.applyProfile(') == 1
assert s.index('Camera2PreHalGate.observeSession(') < s.index('Camera2PhysicalOnlyInt32CandidateSweep.applyProfile(')
assert s.index('Camera2PhysicalOnlyInt32CandidateSweep.applyProfile(') < s.index('device.isSessionConfigurationSupported(config)')
assert s.index('persistOriginalRawBuffer(image, stamp)') < s.index('Camera2EnvelopeProbe.observe(image, physical, physicalResult)')
assert s.index('RawSensorRasterAudit.audit(') < s.index('RawPayloadGeometryDecoder.decode(')
assert 'PHYSICAL_ONLY_INT32_CANDIDATE_SWEEP_I_J_K_NUMERIC_ONE' in s
assert 'staged-evidence.v0.38' in s
assert '_EVIDENCE_v038.json' in s

p.write_text(s)
print('patched v0.38b touch-safe controls', p)
print('bytes', p.stat().st_size)
