#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct v0.30d first. v0.30e changes device usability/recovery only:
# - scientific v0.30 2^4 matrix, vendor writes, acquisition, sealing and Stage 3.6/3.7 stay unchanged;
# - essential controls are moved into a fixed compact 3-row control deck that never depends on scrolling;
# - long telemetry/status/preview/output controls remain in a detail pane below;
# - cache recovery selects the FIRST MISSING run, not highest-completed+1, so out-of-order device runs are recoverable;
# - after each successful capture the next selected profile is again the first missing cached run.
ns = {}
exec(
    compile(
        Path('tools/patch_fotograaf_v030d_scroll_recovery_bundle.py').read_text(),
        'patch_fotograaf_v030d_scroll_recovery_bundle.py',
        'exec',
    ),
    ns,
    ns,
)
s = p.read_text()

# Compact the matrix selector label: it is part of the always-visible control deck.
label_needle = '''        return "Matrix run ${p.runIndex + 1}/16 · ABCD=${p.bits} · ${p.id} · tik = volgende"\n'''
label_replacement = '''        return "R${p.runIndex + 1}/16 · ABCD=${p.bits} · tik→volgende"\n'''
if label_needle not in s:
    raise SystemExit('v0.30e compact matrix label anchor not found')
s = s.replace(label_needle, label_replacement, 1)

# Add a first-missing helper before cache recovery. Filename evidence remains the sole recovery input;
# this does not inspect or mutate RAW source bytes.
recovery_anchor = '''    private fun restoreMatrixProgressFromCache() {\n'''
recovery_helper = '''    private fun firstMissingMatrixRunIndexFromCache(): Int? {\n        val completed = cachedMatrixEvidenceFiles().mapNotNull { file ->\n            matrixEvidenceNameRegex.matchEntire(file.name)?.groupValues?.getOrNull(1)?.toIntOrNull()\n        }.filter { it in 1..Camera2VendorRouteFullFactorialMatrix.TOTAL_RUNS }.toSet()\n        return (1..Camera2VendorRouteFullFactorialMatrix.TOTAL_RUNS)\n            .firstOrNull { it !in completed }\n            ?.minus(1)\n    }\n\n    private fun restoreMatrixProgressFromCache() {\n'''
if recovery_anchor not in s:
    raise SystemExit('v0.30e recovery helper anchor not found')
s = s.replace(recovery_anchor, recovery_helper, 1)

old_recovery = '''        val highestCompletedRun = completed.maxOrNull() ?: return\n        matrixRunIndex = if (highestCompletedRun >= Camera2VendorRouteFullFactorialMatrix.TOTAL_RUNS) {\n            0\n        } else {\n            highestCompletedRun\n        }\n        activeMatrixRunIndex = matrixRunIndex\n        if (::matrixProfileButton.isInitialized) matrixProfileButton.text = matrixProfileLabel(matrixRunIndex)\n        val next = Camera2VendorRouteFullFactorialMatrix.profileForRun(matrixRunIndex)\n        setStatus(\n            "v0.30d cache recovery: ${files.size} matrix evidence JSON(s) gevonden; voltooide runs=${completed.joinToString()}.\\n" +\n                "Volgende geselecteerde run=${next.runIndex + 1}/16 · ABCD=${next.bits} · ${next.id}.\\n" +\n                "Gebruik de ZIP-knop om ook eerder vastgelegde JSONs veilig uit app-cache te exporteren.",\n        )\n'''
new_recovery = '''        val firstMissingRunIndex = firstMissingMatrixRunIndexFromCache()\n        matrixRunIndex = firstMissingRunIndex ?: 0\n        activeMatrixRunIndex = matrixRunIndex\n        if (::matrixProfileButton.isInitialized) matrixProfileButton.text = matrixProfileLabel(matrixRunIndex)\n        val next = Camera2VendorRouteFullFactorialMatrix.profileForRun(matrixRunIndex)\n        val nextText = if (firstMissingRunIndex == null) {\n            "Alle 16 matrixruns zijn in cache aanwezig; R01 is alleen als handmatige selector teruggezet."\n        } else {\n            "Eerste ontbrekende run=${next.runIndex + 1}/16 · ABCD=${next.bits} · ${next.id}."\n        }\n        setStatus(\n            "v0.30e cache recovery: ${files.size} matrix evidence JSON(s) gevonden; aanwezige runs=${completed.joinToString()}.\\n" +\n                nextText + "\\n" +\n                "Essentiële knoppen staan nu vast bovenaan; scrollen is niet meer nodig voor matrix capture/JSON/ZIP.",\n        )\n'''
if old_recovery not in s:
    raise SystemExit('v0.30e first-missing recovery replacement anchor not found')
s = s.replace(old_recovery, new_recovery, 1)

# After a PASS, do not blindly advance to the numerically next run. The evidence JSON has already
# been written at this point, so select the first profile not represented in cache. This safely
# fills holes left by out-of-order runs such as R04/R16/R02.
success_advance_needle = '''                matrixRunIndex = (activeMatrixRunIndex + 1) % Camera2VendorRouteFullFactorialMatrix.TOTAL_RUNS\n                matrixProfileButton.text = matrixProfileLabel(matrixRunIndex)\n'''
success_advance_replacement = '''                matrixRunIndex = firstMissingMatrixRunIndexFromCache()\n                    ?: ((activeMatrixRunIndex + 1) % Camera2VendorRouteFullFactorialMatrix.TOTAL_RUNS)\n                matrixProfileButton.text = matrixProfileLabel(matrixRunIndex)\n'''
if success_advance_needle not in s:
    raise SystemExit('v0.30e success first-missing anchor not found')
s = s.replace(success_advance_needle, success_advance_replacement, 1)

# v0.30c/d wrapped the entire screen in one ScrollView. On the real device this still did not make
# the lower controls reliably reachable. Keep a detail ScrollView, but detach all essential controls
# from the long root and place them in a fixed compact deck above it. Even if detail scrolling fails,
# Step1, Step2, profile selection, capture, evidence JSON and bundle ZIP remain reachable.
return_needle = '''        return ScrollView(this).apply {\n            isFillViewport = true\n            isVerticalScrollBarEnabled = true\n            addView(\n                root,\n                ViewGroup.LayoutParams(\n                    ViewGroup.LayoutParams.MATCH_PARENT,\n                    ViewGroup.LayoutParams.WRAP_CONTENT,\n                ),\n            )\n        }\n    }\n\n    private val matrixEvidenceNameRegex'''
return_replacement = '''        val essentialButtons = listOf(\n            capabilityButton,\n            previewButton,\n            matrixProfileButton,\n            captureButton,\n            saveJsonButton,\n            exportMatrixEvidenceBundleButton,\n        )\n        essentialButtons.forEach { button ->\n            (button.parent as? ViewGroup)?.removeView(button)\n            button.textSize = 11f\n            button.minHeight = 0\n            button.minimumHeight = 0\n            button.maxLines = 2\n            button.setPadding(dp(5), dp(3), dp(5), dp(3))\n        }\n        capabilityButton.text = "1 · Routes"\n        previewButton.text = "2 · Live 3.7×"\n        matrixProfileButton.text = matrixProfileLabel(matrixRunIndex)\n        captureButton.text = "3 · Matrix capture"\n        saveJsonButton.text = "Bewaar JSON"\n        exportMatrixEvidenceBundleButton.text = "Bundle ZIP"\n\n        fun fixedRow(left: Button, right: Button): LinearLayout = LinearLayout(this).apply {\n            orientation = LinearLayout.HORIZONTAL\n            addView(left, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))\n            addView(right, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))\n        }\n\n        val fixedControls = LinearLayout(this).apply {\n            orientation = LinearLayout.VERTICAL\n            setPadding(dp(6), dp(4), dp(6), dp(4))\n            setBackgroundColor(Color.rgb(10, 12, 15))\n            addView(fixedRow(capabilityButton, previewButton))\n            addView(fixedRow(matrixProfileButton, captureButton))\n            addView(fixedRow(saveJsonButton, exportMatrixEvidenceBundleButton))\n        }\n\n        val detailScroll = ScrollView(this).apply {\n            isFillViewport = true\n            isVerticalScrollBarEnabled = true\n            addView(\n                root,\n                ViewGroup.LayoutParams(\n                    ViewGroup.LayoutParams.MATCH_PARENT,\n                    ViewGroup.LayoutParams.WRAP_CONTENT,\n                ),\n            )\n        }\n\n        return LinearLayout(this).apply {\n            orientation = LinearLayout.VERTICAL\n            setBackgroundColor(Color.rgb(10, 12, 15))\n            addView(\n                fixedControls,\n                LinearLayout.LayoutParams(\n                    ViewGroup.LayoutParams.MATCH_PARENT,\n                    ViewGroup.LayoutParams.WRAP_CONTENT,\n                ),\n            )\n            addView(\n                detailScroll,\n                LinearLayout.LayoutParams(\n                    ViewGroup.LayoutParams.MATCH_PARENT,\n                    0,\n                    1f,\n                ),\n            )\n        }\n    }\n\n    private val matrixEvidenceNameRegex'''
if return_needle not in s:
    raise SystemExit('v0.30e fixed-control return anchor not found')
s = s.replace(return_needle, return_replacement, 1)

# Device-facing version text only. This does not change evidence schema v0.30 or scientific design.
s = s.replace(
    'TruthRaw · 200MP Tele Test v0.30 · Camera-5 vendor route full-factorial matrix',
    'TruthRaw · 200MP Tele Test v0.30e · fixed-control matrix recovery',
)

# UI/recovery and scientific invariants.
assert 'firstMissingMatrixRunIndexFromCache()' in s
assert 'val fixedControls = LinearLayout(this).apply {' in s
assert 'val detailScroll = ScrollView(this).apply {' in s
assert 'saveJsonButton.text = "Bewaar JSON"' in s
assert 'exportMatrixEvidenceBundleButton.text = "Bundle ZIP"' in s
assert 'FULL_FACTORIAL_2_LEVEL_4_FACTOR_16_RUN_COMPLEMENT_PAIRED' in s
assert 'Camera2VendorRouteFullFactorialMatrix.applyProfile(' in s
assert s.count('Camera2VendorRouteFullFactorialMatrix.applyProfile(') == 1
assert s.index('Camera2PreHalGate.observeSession(') < s.index('Camera2VendorRouteFullFactorialMatrix.applyProfile(')
assert s.index('Camera2VendorRouteFullFactorialMatrix.applyProfile(') < s.index('device.isSessionConfigurationSupported(config)')
assert s.index('persistOriginalRawBuffer(image, stamp)') < s.index('Camera2EnvelopeProbe.observe(image, physical, physicalResult)')
assert s.index('RawSensorRasterAudit.audit(') < s.index('RawPayloadGeometryDecoder.decode(')
assert 'truthraw.camera5-v030-matrix-evidence-bundle.v0.30d' in s

p.write_text(s)
print('patched v0.30e fixed controls + first-missing matrix recovery', p)
print('bytes', p.stat().st_size)
