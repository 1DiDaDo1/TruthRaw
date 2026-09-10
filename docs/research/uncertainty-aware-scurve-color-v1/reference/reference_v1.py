"""Independent stdlib-only numerical reference for the v1 appearance contract."""

def clamp01(x): return min(1.0,max(0.0,float(x)))
def smoothstep(x):
    t=clamp01(x); return t*t*(3.0-2.0*t)
def scurve(x,s):
    t=clamp01(x); return clamp01(t+s*t*(1.0-t)*(2.0*t-1.0))
def luma(rgb): return 0.2126*rgb[0]+0.7152*rgb[1]+0.0722*rgb[2]

def apply(rgb, ql, qc, censored=False):
    y=luma(rgb)
    strength=0.42+0.12*(1.0-smoothstep(y/0.18))*(1.0-ql)
    yt=scurve(y,strength)
    k=yt/y if y>1e-8 else 0.0
    tone=[v*k for v in rgb]
    gain=0.82+smoothstep(qc)*(1.25-0.82)
    if censored: gain=min(gain,1.0)
    out=[yt+(v-yt)*gain for v in tone]
    return out,yt,strength,gain

def self_test():
    last=-1.0
    for i in range(10001):
        y=scurve(i/10000,0.54)
        assert y+1e-12>=last
        last=y
    low=apply((.12,.08,.05),.1,0.0)
    high=apply((.12,.08,.05),.9,1.0)
    assert low[3] < 1.0 < high[3]
    assert low[2] > high[2]
    c=apply((.12,.08,.05),1.0,1.0,True)
    assert c[3] <= 1.0
    assert abs(luma(low[0])-low[1])<1e-12
    print('TRUTHRAW_UNCERTAINTY_AWARE_SCURVE_COLOR_V1_REFERENCE_PASS')
if __name__=='__main__': self_test()
