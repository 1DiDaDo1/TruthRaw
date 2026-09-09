#!/usr/bin/env python3
from __future__ import annotations
from pathlib import Path
import argparse, hashlib, json, math, sys
import numpy as np
import tifffile

V04=Path(__file__).resolve().parents[1]/'..'/'truthrange-real-dng-stage2-v0.4'/'reference'
# In repository layout the v0.4 reference is a sibling research module.
sys.path.insert(0,str(V04.resolve()))
from dng_stage2_reference_v0_4 import read_classic_dng

FEATURE_NAMES=[
 'log1p_abs_prediction_stage2','log1p_sigma_stage2_x1e4','log1p_predicted_snr',
 'log1p_support_std_over_sigma','log1p_support_range_over_sigma',
 'log1p_pair_min_disagreement_over_sigma','log1p_pair_median_disagreement_over_sigma',
 'log1p_green_direction_disagreement_over_sigma','log1p_local_mosaic_range_over_sigma',
 'gain_at_target','neighbor_censor_fraction','radial_position_norm',
 'prediction_negative_flag','prediction_over1_flag','role_R','role_G1','role_G2','role_B']
FEATURE_SCHEMA_SHA256=hashlib.sha256(json.dumps(FEATURE_NAMES,separators=(',',':')).encode()).hexdigest()
ROLE_POS={'B':(0,0),'G1':(0,1),'G2':(1,0),'R':(1,1)}
ROLE_CHANNEL={'R':0,'G1':1,'G2':1,'B':2}
RUNTIME_ROLE={'R':0,'G1':1,'G2':2,'B':3}
TRAIN_ONEHOT={'B':0,'G1':1,'G2':2,'R':3}
C=np.array([0.395601829,1.01616829,-0.14277533,0.735921985,-0.230914241,0.0217581422,-0.100941628,-0.00342952311,0.0824666634,0.0266935733,0.485900593,0.0192627418,0.281847627,0.519084282,0.0425594193,-0.0499333677,-0.0365210334,0.0438949818],np.float32)
I=np.float32(-9.85952204);E=np.float32(2e-5)
Q50=np.array([[1.23186568,1.29081367,1.250442,1.23057982,1.25909627],[1.64072784,1.40598901,1.35229132,1.32266775,1.28092143],[1.64228642,1.39439067,1.35299496,1.32419466,1.23851723],[1.29139635,1.27813684,1.25189713,1.23929701,1.20947405]],np.float32)
Q95=np.array([[3.54679684,3.79396531,3.62211433,3.64434517,3.62210633],[4.76397741,4.27615232,3.96592525,3.83541287,3.83764344],[4.73872316,4.22170938,3.92232166,3.81420021,3.76353938],[3.71949947,3.82415852,3.62994847,3.56435722,3.56799556]],np.float32)

def gain_cpp_semantics(gm,y,x,H,W):
    rf=(((y+0.5)/H)-gm.origin_v)/gm.spacing_v
    cf=(((x+0.5)/W)-gm.origin_h)/gm.spacing_h
    rf=max(0.0,min(float(gm.points_v-1),rf)); cf=max(0.0,min(float(gm.points_h-1),cf))
    r0=int(rf);r1=min(r0+1,gm.points_v-1);c0=int(cf);c1=min(c0+1,gm.points_h-1)
    fr=np.float32(rf-r0)
    a=np.float32(np.float32(gm.entry(r0,c0))*np.float32(1.0-fr)+np.float32(gm.entry(r1,c0))*fr)
    b=np.float32(np.float32(gm.entry(r0,c1))*np.float32(1.0-fr)+np.float32(gm.entry(r1,c1))*fr)
    base=float(a);delta=float(b)-base
    return np.float32(base+delta*(cf-c0))

def make_stage2(path):
    d=read_classic_dng(path)
    raw=tifffile.imread(path).astype(np.uint16)
    cache={}
    def stage(y,x):
        k=(y,x)
        if k in cache:return cache[k]
        gm=next(g for g in d.gain_maps if g.applies(y,x))
        g=gain_cpp_semantics(gm,y,x,d.height,d.width)
        b=np.float32(d.black_phase[d.phase_index(y,x)]);den=np.float32(max(d.white_level-float(b),1.0))
        # Mirror C++: float(raw)-b, float divide, float multiply.
        v=np.float32(np.float32(np.float32(raw[y,x])-b)/den*g)
        cache[k]=v
        return v
    return d,raw,stage

def leak(stage,y,x):
    gl=float(stage(y,x-1));gr=float(stage(y,x+1));gu=float(stage(y-1,x));gd=float(stage(y+1,x))
    dh=abs(gl-gr);dv=abs(gu-gd);wh=1/(dh+1e-5);wv=1/(dv+1e-5)
    return (wh*.5*(gl+gr)+wv*.5*(gu+gd))/(wh+wv),dh,dv

def predict(stage,role,y,x):
    if role in ('R','B'):
        g0,dh,dv=leak(stage,y,x);offs=[(-2,0),(2,0),(0,-2),(0,2),(-2,-2),(-2,2),(2,-2),(2,2)]
        vals=[];gn=[]
        for oy,ox in offs:
            vals.append(float(stage(y+oy,x+ox)));gn.append(leak(stage,y+oy,x+ox)[0])
        vals=np.array(vals,float);gn=np.array(gn,float);w=1/(np.abs(gn-g0)+1e-4);pred=g0+np.sum(w*(vals-gn))/np.sum(w)
        lo,hi=vals.min(),vals.max();rg=max(hi-lo,1e-6);pred=np.clip(pred,lo-.08*rg,hi+.08*rg)
        pair=np.array([abs(vals[0]-vals[1]),abs(vals[2]-vals[3]),abs(vals[4]-vals[7]),abs(vals[5]-vals[6])])
        return float(pred),vals,pair,dh,dv
    vals=np.array([float(stage(y,x-2)),float(stage(y,x+2)),float(stage(y-2,x)),float(stage(y+2,x)),float(stage(y-1,x-1)),float(stage(y+1,x+1)),float(stage(y-1,x+1)),float(stage(y+1,x-1))])
    cand=np.array([.5*(vals[0]+vals[1]),.5*(vals[2]+vals[3]),.5*(vals[4]+vals[5]),.5*(vals[6]+vals[7])])
    pair=np.array([abs(vals[0]-vals[1]),abs(vals[2]-vals[3]),abs(vals[4]-vals[5]),abs(vals[6]-vals[7])]);order=np.argsort(pair);i0,i1=order[:2];w0=1/(pair[i0]+1e-5);w1=1/(pair[i1]+1e-5);pred=(w0*cand[i0]+w1*cand[i1])/(w0+w1)
    lo,hi=vals.min(),vals.max();rg=max(hi-lo,1e-6);pred=np.clip(pred,lo-.06*rg,hi+.06*rg)
    return float(pred),vals,pair,float(pair[0]),float(pair[1])

def feature(path,y,x,role,d=None,raw=None,stage=None):
    if d is None:d,raw,stage=make_stage2(path)
    with tifffile.TiffFile(path) as tf:
        noise=np.array(tf.pages[0].tags['NoiseProfile'].value,float)
    pred,support,pair,dha,dva=predict(stage,role,y,x)
    gm=next(g for g in d.gain_maps if g.applies(y,x));g=float(gain_cpp_semantics(gm,y,x,d.height,d.width));c=ROLE_CHANNEL[role];S,O=noise[2*c:2*c+2]
    sigma=math.sqrt(max(g*S*max(pred,0)+g*g*O,1e-16));snr=abs(pred)/max(sigma,1e-12)
    local=[];cens=0
    for oy in (-2,-1,0,1,2):
        for ox in (-2,-1,0,1,2):
            if oy==0 and ox==0:continue
            local.append(float(stage(y+oy,x+ox)));cens+=int(raw[y+oy,x+ox]>=d.white_level)
    radial=math.sqrt(((x/(d.width-1)-.5)/.5)**2+((y/(d.height-1)-.5)/.5)**2)/math.sqrt(2.)
    f=np.zeros(18,np.float32);den=max(sigma,1e-12)
    f[0]=np.float32(math.log1p(abs(pred)));f[1]=np.float32(math.log1p(sigma*1e4));f[2]=np.float32(math.log1p(max(snr,0)));f[3]=np.float32(math.log1p(np.std(support)/den));f[4]=np.float32(math.log1p(np.ptp(support)/den));f[5]=np.float32(math.log1p(np.min(pair)/den));f[6]=np.float32(math.log1p(np.median(pair)/den));f[7]=np.float32(math.log1p(abs(dha-dva)/den));f[8]=np.float32(math.log1p(np.ptp(local)/den));f[9]=np.float32(g);f[10]=np.float32(cens/24);f[11]=np.float32(radial);f[12]=np.float32(pred<0);f[13]=np.float32(pred>1);f[14+TRAIN_ONEHOT[role]]=1
    r=RUNTIME_ROLE[role];sb=0 if snr<2 else 1 if snr<4 else 2 if snr<8 else 3 if snr<16 else 4
    z=np.float32(I+np.sum(C*f,dtype=np.float32));mu=np.float32(max(float(np.exp(z,dtype=np.float32)-E),1e-6));p50=np.float32(max(float(mu*Q50[r,sb]),1e-8));p95=np.float32(max(float(mu*Q95[r,sb]),float(p50+np.float32(1e-8))))
    return {'y':y,'x':x,'role':role,'runtime_role':r,'pred':np.float32(pred).item(),'sigma':np.float32(sigma).item(),'snr':np.float32(snr).item(),'p50':p50.item(),'p95':p95.item(),'features':[v.item() for v in f]}

def validate(cli_json:Path,dng_path:Path):
    got=json.load(cli_json.open());d,raw,stage=make_stage2(dng_path);refs=[]
    max_feat=max_scalar=max_scalar_rel=0.0
    roles=[]
    for g in got['probe_anchors']:
        r=feature(dng_path,g['y'],g['x'],g['role'],d,raw,stage);refs.append(r);roles.append(g['role'])
        max_feat=max(max_feat,max(abs(a-b) for a,b in zip(g['features'],r['features'])))
        for k in ('pred','sigma','snr','p50','p95'):
            ae=abs(g[k]-r[k]); re=ae/max(abs(g[k]),abs(r[k]),1e-30)
            max_scalar=max(max_scalar,ae); max_scalar_rel=max(max_scalar_rel,re)
    role_counts={role:roles.count(role) for role in ('R','G1','G2','B')}
    role_gate=all(v==2 for v in role_counts.values())
    scalar_gate=(max_scalar<=5e-6 or max_scalar_rel<=1e-6)
    return {'feature_schema_sha256':FEATURE_SCHEMA_SHA256,'probe_count':len(refs),'probe_role_counts':role_counts,'max_feature_abs_error':max_feat,'max_scalar_abs_error':max_scalar,'max_scalar_rel_error':max_scalar_rel,'pass':role_gate and max_feat<=2e-6 and scalar_gate,'reference':refs}

if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('dng',type=Path);ap.add_argument('cli_json',type=Path);a=ap.parse_args();print(json.dumps(validate(a.cli_json,a.dng),indent=2))
