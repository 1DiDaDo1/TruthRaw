from __future__ import annotations
from pathlib import Path
import hashlib, json, math, re
import numpy as np
import tifffile

LIB=Path('/mnt/data/truthraw_v4f8_sources/lib')
import sys
sys.path.insert(0,str(LIB))
from spatial_color_calibration import parse_dng_gainmaps, bilinear_grid, CFA_PATTERNS

ROLES=('B','G1','G2','R')
ROLE_POS={'B':(0,0),'G1':(0,1),'G2':(1,0),'R':(1,1)}
ROLE_CHANNEL={'R':0,'G1':1,'G2':1,'B':2}
ROLE_CODE={'R':0,'G1':1,'G2':2,'B':3}
STRIDE=16
MARGIN=8
EPS=1e-12

FEATURE_NAMES=[
 'log1p_abs_prediction_stage2',
 'log1p_sigma_stage2_x1e4',
 'log1p_predicted_snr',
 'log1p_support_std_over_sigma',
 'log1p_support_range_over_sigma',
 'log1p_pair_min_disagreement_over_sigma',
 'log1p_pair_median_disagreement_over_sigma',
 'log1p_green_direction_disagreement_over_sigma',
 'log1p_local_mosaic_range_over_sigma',
 'gain_at_target',
 'neighbor_censor_fraction',
 'radial_position_norm',
 'prediction_negative_flag',
 'prediction_over1_flag',
 'role_R','role_G1','role_G2','role_B',
]


def sha256_file(p:Path)->str:
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1<<20),b''): h.update(b)
 return h.hexdigest()

def rats(v):
 a=list(v)
 return np.array([a[i]/a[i+1] for i in range(0,len(a),2)],np.float64)

def _black4(v):
 b=rats(v)
 if len(b)==1:b=np.repeat(b,4)
 if len(b)!=4:raise RuntimeError(f'four BlackLevel values required, got {b}')
 return b.reshape(2,2)

def load_scene(path:Path):
 with tifffile.TiffFile(path) as tf:
  p=tf.pages[0];t=p.tags
  raw=p.asarray().astype(np.float32)
  cfa=bytes(t['CFAPattern'].value)
  if cfa!=bytes([2,1,1,0]): raise RuntimeError(f'BGGR required, got {list(cfa)}')
  make=str(t['Make'].value).strip('\x00 ');model=str(t['Model'].value).strip('\x00 ');software=str(t['Software'].value).strip('\x00 ')
  white=float(np.asarray(t['WhiteLevel'].value).ravel()[0])
  black=_black4(t['BlackLevel'].value)
  noise=np.asarray(t['NoiseProfile'].value,np.float64).ravel()
  if noise.shape!=(6,) or np.any(noise<=0): raise RuntimeError('positive six-value NoiseProfile required')
  gm=parse_dng_gainmaps(t['OpcodeList2'].value,'BGGR')
  focal=t.get('FocalLength'); focal=float(focal.value[0]/focal.value[1]) if focal is not None else None
  iso=t.get('ISOSpeedRatings'); iso=int(np.asarray(iso.value).ravel()[0]) if iso is not None else None
 denom=np.empty_like(raw,np.float32); bfull=np.empty_like(raw,np.float32); stage2=np.empty_like(raw,np.float32); gain=np.empty_like(raw,np.float32)
 for role,(dy,dx) in ROLE_POS.items():
  b=float(black[dy,dx]); den=white-b
  rp=raw[dy::2,dx::2]
  g=bilinear_grid(gm[role].gains,*rp.shape).astype(np.float32)
  bfull[dy::2,dx::2]=b;denom[dy::2,dx::2]=den;gain[dy::2,dx::2]=g
  stage2[dy::2,dx::2]=((rp-b)/den)*g
 meta={'file':path.name,'sha256':sha256_file(path),'make':make,'model':model,'software':software,'shape':list(raw.shape),'cfa':'BGGR','white_level':white,'black_levels':black.ravel().tolist(),'noise_profile':noise.tolist(),'focal_length_mm':focal,'iso':iso,'stage2_min':float(stage2.min()),'stage2_max':float(stage2.max()),'gain_min':float(gain.min()),'gain_max':float(gain.max())}
 return raw,stage2,gain,bfull,denom,white,noise,meta

def target_coords(H,W,role,role_index):
 dy,dx=ROLE_POS[role]
 sy=MARGIN+((dy-MARGIN)&1); sx=MARGIN+((dx-MARGIN)&1)
 ys=np.arange(sy,H-MARGIN,STRIDE,dtype=np.int32); xs=np.arange(sx,W-MARGIN,STRIDE,dtype=np.int32)
 yy,xx=np.meshgrid(ys,xs,indexing='ij')
 # deterministic quarter-density checker within each role grid to keep dataset compact but spatially broad
 iy,ix=np.meshgrid(np.arange(len(ys)),np.arange(len(xs)),indexing='ij')
 keep=((iy+2*ix+role_index)%2)==0
 return yy[keep],xx[keep]

def _avg4(a,y,x):
 return .25*(a[y-1,x]+a[y+1,x]+a[y,x-1]+a[y,x+1])

def _leakfree_green_at_rb(stage2,y,x):
 gl=stage2[y,x-1].astype(np.float64);gr=stage2[y,x+1].astype(np.float64);gu=stage2[y-1,x].astype(np.float64);gd=stage2[y+1,x].astype(np.float64)
 dh=np.abs(gl-gr);dv=np.abs(gu-gd)
 wh=1/(dh+1e-5);wv=1/(dv+1e-5)
 gh=.5*(gl+gr);gv=.5*(gu+gd)
 g=(wh*gh+wv*gv)/(wh+wv)
 return g,dh,dv

def _green_for_coords(stage2, yy, xx):
 # target coordinates here are R/B positions only; use adjacent measured greens, never central R/B target.
 return _leakfree_green_at_rb(stage2,yy,xx)[0]

def predict_hidden(stage2,role,y,x):
 """Leakage-free hidden-CFA proxy predictor in Stage-2 float space.
 The central measured target is never read here.
 """
 if role in ('R','B'):
  g0,dh,dv=_leakfree_green_at_rb(stage2,y,x)
  offs=[(-2,0),(2,0),(0,-2),(0,2),(-2,-2),(-2,2),(2,-2),(2,2)]
  vals=[]; diffs=[]; gn=[]
  for oy,ox in offs:
   ny=y+oy;nx=x+ox
   cn=stage2[ny,nx].astype(np.float64)
   gni=_green_for_coords(stage2,ny,nx)
   vals.append(cn);gn.append(gni);diffs.append(cn-gni)
  vals=np.stack(vals,0);gn=np.stack(gn,0);diffs=np.stack(diffs,0)
  w=1/(np.abs(gn-g0[None,:])+1e-4)
  pred=g0+np.sum(w*diffs,axis=0)/np.sum(w,axis=0)
  # conservative support limiter similar in spirit to production backend
  lo=np.min(vals,axis=0);hi=np.max(vals,axis=0);rg=np.maximum(hi-lo,1e-6)
  pred=np.clip(pred,lo-.08*rg,hi+.08*rg)
  pair=np.stack([np.abs(vals[0]-vals[1]),np.abs(vals[2]-vals[3]),np.abs(vals[4]-vals[7]),np.abs(vals[5]-vals[6])],0)
  return pred,vals,pair,dh,dv
 # Green sites: same-phase axis neighbors plus opposite-green diagonals, all measured and target-independent.
 vals=np.stack([
  stage2[y,x-2],stage2[y,x+2],stage2[y-2,x],stage2[y+2,x],
  stage2[y-1,x-1],stage2[y+1,x+1],stage2[y-1,x+1],stage2[y+1,x-1]
 ],0).astype(np.float64)
 cand=np.stack([.5*(vals[0]+vals[1]),.5*(vals[2]+vals[3]),.5*(vals[4]+vals[5]),.5*(vals[6]+vals[7])],0)
 pair=np.stack([np.abs(vals[0]-vals[1]),np.abs(vals[2]-vals[3]),np.abs(vals[4]-vals[5]),np.abs(vals[6]-vals[7])],0)
 inv=1/(pair+1e-5)
 # use two best directional pairs softly rather than a hard semantic choice
 order=np.argsort(pair,axis=0)
 i0=order[0];i1=order[1]
 c0=np.take_along_axis(cand,i0[None,:],axis=0)[0];c1=np.take_along_axis(cand,i1[None,:],axis=0)[0]
 d0=np.take_along_axis(pair,i0[None,:],axis=0)[0];d1=np.take_along_axis(pair,i1[None,:],axis=0)[0]
 w0=1/(d0+1e-5);w1=1/(d1+1e-5)
 pred=(w0*c0+w1*c1)/(w0+w1)
 lo=np.min(vals,axis=0);hi=np.max(vals,axis=0);rg=np.maximum(hi-lo,1e-6)
 pred=np.clip(pred,lo-.06*rg,hi+.06*rg)
 return pred,vals,pair,pair[0],pair[1]

def extract_dataset(path:Path):
 raw,s2,gain,bfull,denom,white,noise,meta=load_scene(path)
 H,W=raw.shape
 Xs=[];ys=[];roles=[];snrs=[];coords=[];censored=0;counts={}
 # Fixed 5x5 target-independent sample offsets for local range and neighbor censor feature.
 local_off=[(oy,ox) for oy in (-2,-1,0,1,2) for ox in (-2,-1,0,1,2) if not (oy==0 and ox==0)]
 for ri,role in enumerate(ROLES):
  yy,xx=target_coords(H,W,role,ri)
  pred,support,pair,dha,dva=predict_hidden(s2,role,yy,xx)
  g=gain[yy,xx].astype(np.float64)
  c=ROLE_CHANNEL[role];S=float(noise[2*c]);O=float(noise[2*c+1])
  sigma=np.sqrt(np.maximum(g*S*np.maximum(pred,0)+g*g*O,1e-16))
  snr=np.abs(pred)/np.maximum(sigma,1e-12)
  sstd=np.std(support,axis=0);srange=np.ptp(support,axis=0)
  pairmin=np.min(pair,axis=0);pairmed=np.median(pair,axis=0)
  gd=np.abs(dha-dva)
  local=np.stack([s2[yy+oy,xx+ox] for oy,ox in local_off],0).astype(np.float64)
  lrange=np.ptp(local,axis=0)
  neigh_raw=np.stack([raw[yy+oy,xx+ox] for oy,ox in local_off],0)
  neigh_cens=np.mean(neigh_raw>=white,axis=0)
  radial=np.sqrt(((xx/(W-1)-.5)/.5)**2+((yy/(H-1)-.5)/.5)**2)/np.sqrt(2.)
  onehot=np.zeros((len(yy),4),np.float64); onehot[:,ri]=1
  X=np.column_stack([
   np.log1p(np.abs(pred)),
   np.log1p(sigma*1e4),
   np.log1p(np.maximum(snr,0)),
   np.log1p(sstd/np.maximum(sigma,1e-12)),
   np.log1p(srange/np.maximum(sigma,1e-12)),
   np.log1p(pairmin/np.maximum(sigma,1e-12)),
   np.log1p(pairmed/np.maximum(sigma,1e-12)),
   np.log1p(gd/np.maximum(sigma,1e-12)),
   np.log1p(lrange/np.maximum(sigma,1e-12)),
   g,neigh_cens,radial,(pred<0).astype(float),(pred>1).astype(float),onehot
  ]).astype(np.float32)
  # Hidden target is read only after all X features are complete.
  target=s2[yy,xx].astype(np.float64)
  target_raw=raw[yy,xx].astype(np.float64)
  err=np.abs(pred-target).astype(np.float32)
  keep=target_raw<white
  censored+=int(np.sum(~keep))
  counts[role]={'all':int(len(yy)),'kept_uncensored':int(np.sum(keep)),'source_white_censored':int(np.sum(~keep))}
  Xs.append(X[keep]);ys.append(err[keep]);roles.append(np.full(np.sum(keep),ROLE_CODE[role],np.uint8));snrs.append(snr[keep].astype(np.float32));coords.append(np.column_stack([yy[keep],xx[keep]]).astype(np.int32))
 X=np.concatenate(Xs); y=np.concatenate(ys); role=np.concatenate(roles); snr=np.concatenate(snrs); coord=np.concatenate(coords)
 fhash=hashlib.sha256(json.dumps(FEATURE_NAMES,separators=(',',':')).encode()).hexdigest()
 report={'schema':'TruthRawBackendBoundHiddenCFAFeatures/5.0g','source':meta,'targets_uncensored':int(len(y)),'targets_source_white_censored':int(censored),'role_counts':counts,'feature_names':FEATURE_NAMES,'feature_schema_sha256':fhash,'anti_leak':'All features and predictions are computed without reading the central hidden measured sample. Only after X is complete is the target read for error. Source-white targets are excluded from exact-value scoring.','claim_boundary':'Hidden-CFA error is a proxy calibration for reconstruction uncertainty; it is not co-sited RGB ground truth for missing Bayer channels.'}
 return X,y,role,snr,coord,report

def source_admission(path:Path):
 try:
  _,_,_,_,_,white,noise,m=load_scene(path)
 except Exception as e:
  return {'pass':False,'error':str(e)}
 checks={
  'make_HONOR':m['make']=='HONOR',
  'model_BKQ_N49':m['model']=='BKQ-N49',
  'firmware_writer_prefix':m['software'].startswith('HONOR/BKQ-N49/'),
  'BGGR':m['cfa']=='BGGR',
  'white_1023':m['white_level']==1023.0,
  'tele_focal_22_48mm':m['focal_length_mm'] is not None and abs(m['focal_length_mm']-22.48)<0.01,
  'noise_profile_valid':len(m['noise_profile'])==6 and all(float(v)>0 for v in m['noise_profile']),
 }
 return {'pass':all(checks.values()),'checks':checks,'metadata':m}
