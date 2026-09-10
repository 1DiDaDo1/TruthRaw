from pathlib import Path
import hashlib,json
R=Path(__file__).resolve().parents[1]
m=json.loads((R/'SHA256_MANIFEST_v1_0.json').read_text())
for section in ('files','architecture_files'):
    for rel,want in m[section].items():
        p=(R/rel).resolve()
        if not p.exists(): raise SystemExit(f'MISSING {rel}')
        got=hashlib.sha256(p.read_bytes()).hexdigest()
        if got!=want: raise SystemExit(f'HASH_FAIL {rel} {got} != {want}')
readme=(R/'README.md').read_text()
state=json.loads((R/'state/UNCERTAINTY_AWARE_SCURVE_V1_STATE.json').read_text())
metrics=json.loads((R/'real_dog/094423_UNCERTAINTY_AWARE_SCURVE_METRICS.json').read_text())
assert state['appearance_only'] is True
assert state['scientific_master_mutation_allowed'] is False
assert state['independent_rgb_curves_admitted'] is False
assert state['unknown_chroma_confidence_policy']=='NO_POSITIVE_CHROMA_BOOST'
assert 'Unknown covariance is never replaced by independence' in readme
assert metrics['scientific_master_modified'] is False
assert metrics['low_confidence_shadow_sigma_out_over_in']['median'] < 1.0
assert metrics['high_confidence_midtone_slope']['median'] > 1.0
assert metrics['safe_color_hue_shift_vs_tone_only_deg']['p95'] < 0.01
assert metrics['unsafe_per_channel_curve_hue_shift_vs_tone_only_deg']['p95'] > 0.5
src=(R/'native/uncertainty_aware_scurve_v1.cpp').read_text()
assert 'sourceHighCensored' in src
assert 'chromaConfidenceKnown' in src
arch=(R/'../../CORE_VISION_UNCERTAINTY_AWARE_APPEARANCE.md').resolve().read_text()
assert 'appearance policy, not a reconstruction prior' in arch
print('TRUTHRAW_UNCERTAINTY_AWARE_SCURVE_V1_INTEGRITY PASS')
