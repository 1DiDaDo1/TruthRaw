"""Stdlib-only numerical reference for TruthRaw S-Curve v1.0."""

def clamp01(x): return min(1.0,max(0.0,float(x)))
def smoothstep(x):
    x=clamp01(x); return x*x*(3.0-2.0*x)
def power_s(x,p=1.38):
    x=clamp01(x)
    if x<=0.0 or x>=1.0: return x
    a=x**p; b=(1.0-x)**p
    return a/(a+b)
def shadow_safe(x,floor=.58,end=.18):
    x=clamp01(x); t=smoothstep(x/end); return x*(floor+(1.0-floor)*t)
def tone(x,sigma):
    q=0.0 if sigma is None else smoothstep(((x/max(sigma,1e-30))-2.0)/6.0)
    return shadow_safe(x)*(1-q)+power_s(x)*q

def slope(x,sigma,h=1e-6):
    lo=max(0.0,x-h); hi=min(1.0,x+h)
    return (tone(hi,sigma)-tone(lo,sigma))/(hi-lo)

def chroma_gain(q):
    q2=smoothstep((clamp01(q)-.20)/(.85-.20))
    return .88+(1.14-.88)*q2

def self_test():
    assert slope(.02,.02) < 1.0
    assert slope(.35,.001) > 1.0
    assert chroma_gain(0.0) < 1.0
    assert chroma_gain(1.0) > 1.0
    assert chroma_gain(.2) == .88
    assert abs(tone(.05,None)-shadow_safe(.05)) < 1e-15
    print('TRUTHRAW_SCURVE_REFERENCE_V1 PASS')
if __name__=='__main__': self_test()
