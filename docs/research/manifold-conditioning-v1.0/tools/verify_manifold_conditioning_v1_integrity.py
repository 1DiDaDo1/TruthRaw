from __future__ import annotations
import hashlib, json
from pathlib import Path

root=Path(__file__).resolve().parents[1]
manifest=json.loads((root/'SHA256_MANIFEST_v1.json').read_text())

def sha(p: Path): return hashlib.sha256(p.read_bytes()).hexdigest()

for rel,expected in manifest['files'].items():
    p=root/rel
    assert p.is_file(), rel
    got=sha(p)
    assert got==expected, (rel,got,expected)
for rel,expected in manifest['architecture_files'].items():
    p=(root/rel).resolve()
    assert p.is_file(), rel
    got=sha(p)
    assert got==expected, (rel,got,expected)

src=(root/'native/manifold_conditioning_v1.cpp').read_text()
hdr=(root/'native/manifold_conditioning_v1.h').read_text()
readme=(root/'README.md').read_text()
assert 'EXACT_REPARAMETERIZATION accepts exposure EV only' in src
assert 'virtual manifold violates the one-frame/one-evidence-root invariant' in src
assert 'candidateFrozenBeforeEvidence' in src
assert 'InsufficientIndependentEvidence' in hdr
assert 'RejectedMetricRegression' in hdr
assert 'decision.status == SelectionStatusV1::Eligible' in src
assert 'virtual EV node count is never an evidence multiplier' in readme
assert 'co-sited missing colour remains uncertified' in readme

real=json.loads((root/'evidence/REAL_094423_EXACT_REPARAMETERIZATION_v1.json').read_text())
agg=real['aggregate']
assert agg['total_roundtrip_value_checks']==451215360
assert agg['total_bit_changed_values']==0
assert agg['max_abs_roundtrip_error']==0.0
assert real['physical_frame_count']==1
assert real['independent_evidence_root_count']==1

dog=json.loads((root/'evidence/DOG_3SCENE_HISTORICAL_MULTI_EV_EVIDENCE_v1.json').read_text())
assert dog['aggregate_interpretation']['qualifying_frozen_scenes_available']==2
assert dog['aggregate_interpretation']['historical_candidate_status']=='NOT_PROMOTABLE'
assert dog['aggregate_interpretation']['co_sited_missing_colour_certified'] is False
assert dog['aggregate_interpretation']['evidence_multiplier_from_virtual_views']==1.0
roles={x['scene_id']:x['role'] for x in dog['sources']}
assert roles['094423']=='DEVELOPMENT_TUNING_EXPOSED'
assert roles['094414']=='FROZEN_PARAMETER_CHECK'
assert roles['094416']=='FROZEN_PARAMETER_CHECK'
# The frozen checks must retain at least one topology regression; do not silently erase falsification evidence.
for x in dog['sources']:
    if not x['candidate_frozen_before_evidence']:
        continue
    assert any((m['ordering_multi_ev'] < m['ordering_baseline'] or
                m['curvature_multi_ev'] < m['curvature_baseline'])
               for m in x['channels'].values())

print('TRUTHRAW_MANIFOLD_CONDITIONING_V1_INTEGRITY PASS')
