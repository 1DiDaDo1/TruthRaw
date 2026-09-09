from pathlib import Path
import numpy as np, tifffile, json, math, hashlib

ROOT=Path('/mnt/data/truthraw_topology_v08')
SOURCES=[
 ('094414',Path('/mnt/data/IMG_BNC_TRUTHRAW20260907_094414_423.dng')),
 ('094416',Path('/mnt/data/IMG_BNC_TRUTHRAW20260907_094416_197.dng')),
 ('094423',Path('/mnt/data/IMG_BNC_TRUTHRAW20260907_094423_122.dng')),
]
SEED=0x5452555448524157
rng=np.random.default_rng(SEED)

def load_dump(path):
    with open(path,'rb') as f:
        magic=np.fromfile(f,'<u4',1)[0]; w=int(np.fromfile(f,'<u4',1)[0]); h=int(np.fromfile(f,'<u4',1)[0])
        assert magic==0x54325338
        n=w*h
        s=np.fromfile(f,'<f4',n).reshape(h,w)
        g=np.fromfile(f,'<f4',n).reshape(h,w)
        c=np.fromfile(f,'u1',n).reshape(h,w)
        raw=np.fromfile(f,'<u2',n).reshape(h,w)
    return s,g,c,raw

def noise_profile(path):
    with tifffile.TiffFile(path) as tf:
        p=tf.pages[0]
        npv=np.asarray(p.tags['NoiseProfile'].value,dtype=np.float64).reshape(3,2)
        wl=float(p.tags['WhiteLevel'].value)
    return npv,wl

def gproxy(stage,y,x):
    # Target-independent guide for R/B: average physically measured adjacent green sites.
    return 0.25*(stage[y,x-1]+stage[y,x+1]+stage[y-1,x]+stage[y+1,x])

def fold_id(y,x):
    return ((x//2)&1) + 2*((y//2)&1)

def region_id(y,x,h,w):
    xn=(x-(w-1)/2)/((w-1)/2); yn=(y-(h-1)/2)/((h-1)/2)
    r=np.sqrt(xn*xn+yn*yn)/np.sqrt(2.0)
    return np.where(r<0.35,0,np.where(r<0.70,1,2))

def offsets_for(c):
    if c in (0,2):
        return [(-2,0),(2,0),(0,-2),(0,2),(-2,-2),(2,2),(-2,2),(2,-2)]
    return [(-2,0),(2,0),(0,-2),(0,2),(-1,-1),(1,1),(-1,1),(1,-1)]

def pairs_for(c):
    if c in (0,2):
        return [((-2,0),(2,0)),((0,-2),(0,2)),((-2,-2),(2,2)),((-2,2),(2,-2))]
    return [((-2,0),(2,0)),((0,-2),(0,2)),((-1,-1),(1,1)),((-1,1),(1,-1))]

def aggregate_group(stage,gain,color,raw,noise,wl,c,fold,max_samples=60000):
    h,w=stage.shape
    yy,xx=np.nonzero((color==c) & (fold_id(*np.indices((h,w)))==fold))
    # valid interior + uncensored target
    m=(yy>=3)&(yy<h-3)&(xx>=3)&(xx<w-3)&(raw[yy,xx]<wl)
    yy,xx=yy[m],xx[m]
    if yy.size>max_samples:
        idx=rng.choice(yy.size,max_samples,replace=False); yy,xx=yy[idx],xx[idx]
    n=yy.size
    truth=stage[yy,xx].astype(np.float64)
    sig_t=np.sqrt(np.maximum(gain[yy,xx]*noise[c,0]*np.maximum(truth,0.0)+(gain[yy,xx]**2)*noise[c,1],0.0))
    offs=offsets_for(c)
    vals=[]; sigs=[]; valids=[]; dists=[]; guides=[]
    gtg=gproxy(stage,yy,xx).astype(np.float64) if c in (0,2) else None
    for dx,dy in offs:
        ny=yy+dy; nx=xx+dx
        v=stage[ny,nx].astype(np.float64)
        ok=(color[ny,nx]==c)&(fold_id(ny,nx)!=fold)&(raw[ny,nx]<wl)
        sg=np.sqrt(np.maximum(gain[ny,nx]*noise[c,0]*np.maximum(v,0.0)+(gain[ny,nx]**2)*noise[c,1],0.0))
        vals.append(v); sigs.append(sg); valids.append(ok); dists.append(math.hypot(dx,dy))
        if c in (0,2): guides.append(gproxy(stage,ny,nx).astype(np.float64))
    V=np.stack(vals,1); SG=np.stack(sigs,1); OK=np.stack(valids,1); D=np.asarray(dists)[None,:]
    # spatial baseline
    W0=np.where(OK,1.0/D,0.0); denom=W0.sum(1)
    good=denom>0
    pred_base=np.sum(W0*V,1)/np.maximum(denom,1e-30)
    if c in (0,2):
        GG=np.stack(guides,1)
        W=np.where(OK,(1.0/D)/(1e-4+np.abs(GG-gtg[:,None])),0.0)
        ws=W.sum(1)
        pred=gtg+np.sum(W*(V-GG),1)/np.maximum(ws,1e-30)
    else:
        pred=pred_base.copy()
    # production-like support limiting for evaluated predictor
    lo=np.min(np.where(OK,V,np.inf),1); hi=np.max(np.where(OK,V,-np.inf),1)
    span=np.maximum(hi-lo,0.0); margin=0.125*span+1e-5
    pred_limited=np.minimum(np.maximum(pred,lo-margin),hi+margin)
    # require >=4 supports
    support_count=OK.sum(1); good &= support_count>=4 & np.isfinite(lo)&np.isfinite(hi)
    yy=yy[good];xx=xx[good];truth=truth[good];sig_t=sig_t[good];V=V[good];SG=SG[good];OK=OK[good];pred=pred[good];pred_limited=pred_limited[good];pred_base=pred_base[good];lo=lo[good];hi=hi[good];span=span[good]
    # value metrics
    ae=np.abs(pred_limited-truth); ae_base=np.abs(pred_base-truth)
    sigma_units=ae/np.maximum(sig_t,1e-12)
    rel_span=ae/np.maximum(span,4*sig_t+1e-6)
    # pairwise ordering against supports, significant only
    td=truth[:,None]-V; pd=pred_limited[:,None]-V
    sig_pair=np.sqrt(sig_t[:,None]**2+SG**2)
    significant=OK & (np.abs(td)>2.0*sig_pair)
    agree=significant & (np.signbit(td)==np.signbit(pd))
    order_total=int(significant.sum()); order_agree=int(agree.sum())
    # local rank discrepancy (all valid supports)
    rank_true=np.sum(OK & (V<truth[:,None]),1)/np.maximum(OK.sum(1),1)
    rank_pred=np.sum(OK & (V<pred_limited[:,None]),1)/np.maximum(OK.sum(1),1)
    rank_err=np.abs(rank_true-rank_pred)
    # curvature sign on opposite support pairs
    pair_map={o:i for i,o in enumerate(offs)}
    curv_total=curv_agree=0
    for a,b in pairs_for(c):
        ia,ib=pair_map[a],pair_map[b]
        ok=OK[:,ia]&OK[:,ib]
        mean=.5*(V[:,ia]+V[:,ib])
        tc=truth-mean; pc=pred_limited-mean
        sigc=np.sqrt(sig_t**2+0.25*(SG[:,ia]**2+SG[:,ib]**2))
        sig=ok&(np.abs(tc)>2.0*sigc)
        curv_total+=int(sig.sum());curv_agree+=int(np.sum(sig&(np.signbit(tc)==np.signbit(pc))))
    reg=region_id(yy,xx,h,w)
    out={
      'channel':'RGB'[c], 'fold':int(fold), 'n':int(truth.size),
      'mae':float(np.mean(ae)), 'median_abs':float(np.median(ae)), 'p95_abs':float(np.quantile(ae,.95)),
      'baseline_mae':float(np.mean(ae_base)), 'mae_ratio_vs_spatial_baseline':float(np.mean(ae)/np.mean(ae_base)),
      'median_target_sigma_units':float(np.median(sigma_units)), 'p95_target_sigma_units':float(np.quantile(sigma_units,.95)),
      'median_rel_local_span':float(np.median(rel_span)), 'p95_rel_local_span':float(np.quantile(rel_span,.95)),
      'ordering_significant_pairs':order_total, 'ordering_agreement':float(order_agree/order_total) if order_total else None,
      'curvature_significant_pairs':curv_total, 'curvature_agreement':float(curv_agree/curv_total) if curv_total else None,
      'mean_local_rank_error':float(np.mean(rank_err)), 'p95_local_rank_error':float(np.quantile(rank_err,.95)),
      'raw_unlimited_support_violation_fraction':float(np.mean((pred<lo)|(pred>hi))),
      'region':{}
    }
    for rid,name in enumerate(['center','mid','edge']):
        z=(reg==rid)
        if not np.any(z): continue
        out['region'][name]={
          'n':int(z.sum()), 'mae':float(np.mean(ae[z])), 'p95_abs':float(np.quantile(ae[z],.95)),
          'median_rel_local_span':float(np.median(rel_span[z])), 'mean_local_rank_error':float(np.mean(rank_err[z]))
        }
    return out

def main():
    allres=[]
    for key,src in SOURCES:
        dump=ROOT/(src.stem+'.stage2.bin')
        stage,gain,color,raw=load_dump(dump); noise,wl=noise_profile(src)
        scene={'key':key,'source':src.name,'noise_profile':noise.tolist(),'white_level':wl,'groups':[]}
        for c in range(3):
            for f in range(4):
                r=aggregate_group(stage,gain,color,raw,noise,wl,c,f)
                scene['groups'].append(r)
                print(key,r['channel'],f,'n',r['n'],'mae',r['mae'],'order',r['ordering_agreement'],'curv',r['curvature_agreement'],'rank',r['mean_local_rank_error'])
        allres.append(scene)
    # summaries
    summary={}
    for c,ch in enumerate('RGB'):
        gs=[g for s in allres for g in s['groups'] if g['channel']==ch]
        wt=np.array([g['n'] for g in gs],float)
        def wavg(k): return float(np.average([g[k] for g in gs],weights=wt))
        # aggregate pair-weighted ordering/curvature
        oa=sum(g['ordering_agreement']*g['ordering_significant_pairs'] for g in gs if g['ordering_agreement'] is not None); on=sum(g['ordering_significant_pairs'] for g in gs)
        ca=sum(g['curvature_agreement']*g['curvature_significant_pairs'] for g in gs if g['curvature_agreement'] is not None); cn=sum(g['curvature_significant_pairs'] for g in gs)
        summary[ch]={
          'samples':int(wt.sum()), 'weighted_mae':wavg('mae'), 'weighted_p95_abs_mean':wavg('p95_abs'),
          'weighted_mae_ratio_vs_spatial_baseline':wavg('mae_ratio_vs_spatial_baseline'),
          'ordering_agreement':float(oa/on), 'ordering_pairs':int(on),
          'curvature_agreement':float(ca/cn), 'curvature_pairs':int(cn),
          'weighted_mean_local_rank_error':wavg('mean_local_rank_error'),
          'weighted_raw_support_violation_fraction':wavg('raw_unlimited_support_violation_fraction'),
          'worst_fold_ordering':float(min(g['ordering_agreement'] for g in gs if g['ordering_agreement'] is not None)),
          'worst_fold_curvature':float(min(g['curvature_agreement'] for g in gs if g['curvature_agreement'] is not None)),
          'worst_fold_p95_rel_local_span':float(max(g['p95_rel_local_span'] for g in gs)),
        }
    result={
      'study':'TruthRaw missing-channel topology held-out CFA proxy v0.8',
      'main_sha':'10b2927d0b631f5fc22e5399b39860f38763efad',
      'method':'4-fold same-channel CFA holdout; target value never used by predictor; R/B use target-independent adjacent-green color-difference guide; G uses same-channel spatial support; source-clipped targets/support excluded',
      'claim_boundary':'Necessary-condition topology proxy only. It does not provide co-sited independent ground truth for channels that were physically absent at a Bayer pixel and therefore cannot set topologyCertified=true by itself.',
      'scenes':allres,'summary':summary
    }
    p=ROOT/'TOPOLOGY_HELDOUT_RESULTS_v0_8.json'; p.write_text(json.dumps(result,indent=2),encoding='utf-8')
    print(json.dumps(summary,indent=2))
if __name__=='__main__':main()
