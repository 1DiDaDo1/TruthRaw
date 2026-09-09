"""Independent numerical reference for TruthRaw Virtual Observation Manifold v0.9."""
from __future__ import annotations
import math
import numpy as np

DEFAULT_EV_NODES = (-6,-4,-2,0,2,4,6,8,10)

def exposure_scale(ev: float) -> float:
    return 2.0 ** float(ev)

def project_truthrange_ev(t: float, exposure_ev: float) -> float:
    return float(t) + float(exposure_ev)

def project_signed_linear(x, exposure_ev: float, gain_ev: float=0.0):
    return np.asarray(x, np.float64) * exposure_scale(exposure_ev + gain_ev)

def project_covariance(cov, exposure_ev: float, gain_ev: float=0.0):
    s = exposure_scale(exposure_ev + gain_ev)
    return np.asarray(cov, np.float64) * s * s

def normalized_ev_weights(signal: float, ev_nodes=DEFAULT_EV_NODES, target: float=.18, width_stops: float=2.0):
    x=[]
    for ev in ev_nodes:
        d=abs(math.log2(signal*exposure_scale(ev)/target))
        x.append(math.exp(-.5*(d/width_stops)**2))
    x=np.asarray(x,np.float64)
    return x/x.sum()

def self_test():
    assert exposure_scale(4)==16
    assert project_truthrange_ev(-3,4)==1
    c=np.array([[1,.2,.1],[.2,4,.3],[.1,.3,9]],np.float64)
    assert np.array_equal(project_covariance(c,4),c*256)
    w=normalized_ev_weights(.01)
    assert abs(w.sum()-1)<1e-15
    # View count is deliberately absent from the evidence equations.
    print('TRUTHRAW_VIRTUAL_OBSERVATION_REFERENCE_V0_9 PASS')

if __name__=='__main__': self_test()
