"""Stdlib-only reference for TruthRaw Manifold Conditioning v1."""
from __future__ import annotations
import json
import math
from pathlib import Path


def condition(x: float, sigma: float, ev: float):
    s = math.ldexp(1.0, int(ev)) if float(ev).is_integer() else 2.0 ** ev
    return x * s, sigma * s, s


def decondition(y: float, sy: float, s: float):
    return y / s, sy / s


def qualify_channel(records, channel, min_scenes=3, max_mae_ratio=.999,
                    max_p95_ratio=1.0, min_ordering_delta=0.0,
                    min_curvature_delta=0.0):
    qualifying=[]
    failures=[]
    for r in records:
        if not r['candidate_frozen_before_evidence']:
            continue
        m=r['channels'][channel]
        qualifying.append(r['scene_id'])
        od=m['ordering_multi_ev']-m['ordering_baseline']
        cd=m['curvature_multi_ev']-m['curvature_baseline']
        if (m['mae_ratio'] > max_mae_ratio or m['p95_ratio'] > max_p95_ratio or
            od < min_ordering_delta or cd < min_curvature_delta):
            failures.append(r['scene_id'])
    if len(set(qualifying)) < min_scenes:
        return 'INSUFFICIENT_INDEPENDENT_EVIDENCE', qualifying, failures
    if failures:
        return 'REJECTED_METRIC_REGRESSION', qualifying, failures
    return 'ELIGIBLE', qualifying, failures


def self_test():
    for ev in (-20,-10,-6,-4,-2,0,2,4,6,8,10,20):
        y,sy,s=condition(-0.00325,0.0075,ev)
        x,sx=decondition(y,sy,s)
        assert x == -0.00325
        assert sx == 0.0075

    root=Path(__file__).resolve().parents[1]
    evidence=json.loads((root/'evidence/DOG_3SCENE_HISTORICAL_MULTI_EV_EVIDENCE_v1.json').read_text())
    for c in ('R','G','B'):
        status, scenes, failures=qualify_channel(evidence['sources'],c)
        assert status == 'INSUFFICIENT_INDEPENDENT_EVIDENCE'
        assert set(scenes)=={'094414','094416'}
        assert failures, 'historical transplanted candidate must retain its regression evidence'
    assert evidence['aggregate_interpretation']['evidence_multiplier_from_virtual_views'] == 1.0
    print('TRUTHRAW_MANIFOLD_CONDITIONING_REFERENCE_V1 PASS')


if __name__=='__main__':
    self_test()
