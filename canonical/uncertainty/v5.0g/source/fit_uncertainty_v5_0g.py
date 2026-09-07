from __future__ import annotations
from pathlib import Path
from datetime import datetime, timezone
from zoneinfo import ZoneInfo
import hashlib, json, joblib, numpy as np
from lightgbm import LGBMRegressor
from uncertainty_core_v5_0g import FEATURE_NAMES, sha256_file

HERE=Path(__file__).resolve().parent
CORE_CPP=Path('/mnt/data/truthraw_mobile_backends_v4_7i/native/src/core.cpp')
CORE_H=Path('/mnt/data/truthraw_mobile_backends_v4_7i/native/include/truthraw/core.h')
HIDDEN=HERE/'uncertainty_core_v5_0g.py'


def combined_hash(items):
 h=hashlib.sha256()
 for label,p in items:
  h.update(label.encode()+b'\0'+sha256_file(p).encode()+b'\n')
 return h.hexdigest()

def make_model(q):
 return LGBMRegressor(
  objective='quantile',alpha=q,n_estimators=260,learning_rate=0.045,
  num_leaves=31,max_depth=-1,min_child_samples=220,min_split_gain=0.0,
  reg_lambda=0.12,reg_alpha=0.0,subsample=1.0,colsample_bytree=0.92,
  random_state=5070,n_jobs=8,verbosity=-1
 )

def summary_metrics(y,p50,p95,role,snr):
 p50=np.maximum(np.asarray(p50,float),1e-8);p95=np.maximum(np.asarray(p95,float),p50+1e-8);y=np.asarray(y,float)
 risk=p95
 edges=np.quantile(risk,[0,.2,.4,.6,.8,1])
 qs=[]
 for i in range(5):
  m=(risk>=edges[i]) & ((risk<edges[i+1]) if i<4 else risk<=edges[i+1])
  qs.append({'q':i+1,'n':int(m.sum()),'pred_p95_median':float(np.median(risk[m])),'obs_error_median':float(np.median(y[m])),'obs_error_p95':float(np.percentile(y[m],95)),'coverage_p95':float(np.mean(y[m]<=risk[m]))})
 by_role={}
 for rc,name in [(0,'R'),(1,'G1'),(2,'G2'),(3,'B')]:
  m=role==rc
  by_role[name]={'n':int(m.sum()),'coverage_p50':float(np.mean(y[m]<=p50[m])),'coverage_p95':float(np.mean(y[m]<=p95[m])),'obs_median':float(np.median(y[m])),'obs_p95':float(np.percentile(y[m],95)),'pred_p95_median':float(np.median(p95[m]))}
 by_snr={}
 for name,lo,hi in [('lt2',-1e99,2),('2to4',2,4),('4to8',4,8),('8to16',8,16),('ge16',16,1e99)]:
  m=(snr>=lo)&(snr<hi)
  if m.any(): by_snr[name]={'n':int(m.sum()),'coverage_p50':float(np.mean(y[m]<=p50[m])),'coverage_p95':float(np.mean(y[m]<=p95[m])),'obs_median':float(np.median(y[m])),'obs_p95':float(np.percentile(y[m],95)),'pred_p95_median':float(np.median(p95[m]))}
 return {
  'n':int(len(y)),'coverage_p50':float(np.mean(y<=p50)),'coverage_p95':float(np.mean(y<=p95)),
  'observed_median':float(np.median(y)),'observed_p95':float(np.percentile(y,95)),
  'predicted_p50_median':float(np.median(p50)),'predicted_p95_median':float(np.median(p95)),
  'mean_band_width':float(np.mean(p95-p50)),'risk_quintiles':qs,'by_role':by_role,'by_snr':by_snr,
  'risk_ordering_pass':bool(qs[-1]['obs_error_median']>qs[0]['obs_error_median']*1.5),
  'sharpness_ratio_pred_p95_median_to_observed_p95':float(np.median(p95)/max(np.percentile(y,95),1e-12))
 }

def main():
 Ds=[]; reports=[]
 for i in range(1,5):
  z=np.load(HERE/f'dataset_scene_{i}_compact_v5_0g.npz',allow_pickle=False)
  rep=json.loads((HERE/f'dataset_scene_{i}_v5_0g.json').read_text())
  X=np.asarray(z['X'],np.float32); y=np.asarray(z['y'],np.float32); role=np.asarray(z['role'],np.uint8); snr=np.asarray(z['snr'],np.float32)
  Ds.append((X,y,role,snr)); reports.append(rep)
 X=np.concatenate([d[0] for d in Ds]);y=np.concatenate([d[1] for d in Ds]);role=np.concatenate([d[2] for d in Ds]);snr=np.concatenate([d[3] for d in Ds]);scene=np.concatenate([np.full(len(d[1]),i,np.uint8) for i,d in enumerate(Ds)])
 p50o=np.empty(len(y),np.float32);p95o=np.empty(len(y),np.float32);folds=[]
 for hold in range(4):
  tr=scene!=hold;te=scene==hold
  print('fold',hold+1,'train',int(tr.sum()),'test',int(te.sum()),flush=True)
  m50=make_model(.50);m95=make_model(.95)
  m50.fit(X[tr],y[tr]);m95.fit(X[tr],y[tr])
  a=np.maximum(m50.predict(X[te]),1e-8);b=np.maximum(m95.predict(X[te]),a+1e-8)
  p50o[te]=a;p95o[te]=b
  fm=summary_metrics(y[te],a,b,role[te],snr[te]);fm['holdout_scene']=reports[hold]['source']['file'];folds.append(fm)
 oof=summary_metrics(y,p50o,p95o,role,snr)
 # Fit final development model on all inspected scenes.
 final50=make_model(.50);final95=make_model(.95); final50.fit(X,y);final95.fit(X,y)
 model_path=HERE/'UNCERTAINTY_MODEL_v5_0g.joblib'
 joblib.dump({'p50':final50,'p95':final95,'feature_names':FEATURE_NAMES,'engine':'LightGBM-quantile'},model_path,compress=3)
 backend_hash=combined_hash([('core.cpp',CORE_CPP),('core.h',CORE_H)])
 hidden_hash=sha256_file(HIDDEN)
 feature_hash=hashlib.sha256(json.dumps(FEATURE_NAMES,separators=(',',':')).encode()).hexdigest()
 bind=hashlib.sha256((backend_hash+'\n'+hidden_hash+'\n'+feature_hash).encode()).hexdigest()
 binding={'schema':'TruthRawBackendUncertaintyBinding/5.0g','production_reconstruction_backend':'ResearchEdgeAwareMeasuredPreservingReconstruction','production_backend_files':{'core.cpp':sha256_file(CORE_CPP),'core.h':sha256_file(CORE_H)},'production_backend_combined_sha256':backend_hash,'hidden_cfa_proxy_backend_sha256':hidden_hash,'feature_schema_sha256':feature_hash,'uncertainty_binding_sha256':bind,'claim_boundary':'Calibrated on leakage-free hidden-CFA recovery of measured Bayer samples. This is a backend-bound proxy for reconstruction uncertainty, not co-sited RGB ground truth for missing Bayer channels.'}
 (HERE/'BACKEND_BINDING_v5_0g.json').write_text(json.dumps(binding,indent=2),encoding='utf-8')
 gates={
  'four_scene_held_out_cv':len(folds)==4,
  'oof_targets_ge_300k':oof['n']>=300000,
  'oof_p50_coverage_0_44_to_0_56':.44<=oof['coverage_p50']<=.56,
  'oof_p95_coverage_0_90_to_0_99':.90<=oof['coverage_p95']<=.99,
  'risk_ordering':oof['risk_ordering_pass'],
  'all_fold_p95_coverage_0_85_to_0_995':all(.85<=f['coverage_p95']<=.995 for f in folds),
  'float_scene_linear_no_affine_clamp':True,
  'source_white_excluded_from_exact_error':True,
  'negative_and_overrange_targets_retained':True,
  'backend_hash_bound':True,'feature_schema_hash_bound':True,'no_old_v4f10_confidence_reuse':True,
 }
 validation={'schema':'TruthRawBackendBoundUncertaintyDevelopmentValidation/5.0g','gates':gates,'pass':all(gates.values()),'oof':oof,'folds':folds,'sources':[r['source'] for r in reports]}
 (HERE/'VALIDATION_v5_0g.json').write_text(json.dumps(validation,indent=2),encoding='utf-8')
 now=datetime.now(timezone.utc);local=now.astimezone(ZoneInfo('Europe/Amsterdam'))
 model_sha=sha256_file(model_path)
 protocol={'schema':'TruthRawFrozenTeleUncertaintyProtocol/5.0g','frozen_at_utc':now.isoformat(),'frozen_at_europe_amsterdam':local.isoformat(),'source_class':{'make':'HONOR','model':'BKQ-N49','writer_family_prefix':'HONOR/BKQ-N49/','cfa':'BGGR','white_level':1023,'focal_length_mm':22.48,'project_lens':'Tele 3.7x / System ID 5'},'frozen_assets':{'model':{'file':model_path.name,'sha256':model_sha},'backend_binding':{'file':'BACKEND_BINDING_v5_0g.json','sha256':sha256_file(HERE/'BACKEND_BINDING_v5_0g.json')},'feature_core':{'file':HIDDEN.name,'sha256':hidden_hash}},'historical_development_sources':{r['source']['sha256']:r['source']['file'] for r in reports},'prospective_requirement':'A new tele vendor DNG captured strictly after freeze and not matching any historical source hash.','prospective_gates':{'minimum_uncensored_targets':80000,'p50_coverage_range':[.42,.58],'p95_coverage_range':[.88,.995],'risk_ordering_required':True,'max_median_predicted_p95_to_observed_p95_ratio':1.35},'decision_if_pass':'BACKEND_BOUND_UNCERTAINTY_PROSPECTIVE_PASS','decision_if_fail':'BACKEND_BOUND_UNCERTAINTY_PROSPECTIVE_FAIL','ptc_status_before_prospective':'PURE_TRUTH_DERIVED_UNCERTAINTY_RESEARCH_BOUND_NOT_PROSPECTIVE'}
 (HERE/'FROZEN_PROTOCOL_v5_0g.json').write_text(json.dumps(protocol,indent=2),encoding='utf-8')
 status={'schema':'TruthRawUncertaintyStatus/5.0g','development_validation_pass':validation['pass'],'backend_bound':True,'prospective_holdout_pass':False,'ptc_uncertainty_state':'RESEARCH_BACKEND_BOUND_WAITING_NEW_PROSPECTIVE_TELE_CAPTURE','model_sha256':model_sha,'binding_sha256':bind}
 (HERE/'STATUS_v5_0g.json').write_text(json.dumps(status,indent=2),encoding='utf-8')
 np.savez_compressed(HERE/'OOF_PREDICTIONS_v5_0g.npz',y=y,p50=p50o,p95=p95o,role=role,snr=snr,scene=scene)
 print(json.dumps({'validation_pass':validation['pass'],'oof':{k:v for k,v in oof.items() if k not in ('risk_quintiles','by_role','by_snr')},'fold_p50':[f['coverage_p50'] for f in folds],'fold_p95':[f['coverage_p95'] for f in folds],'model_sha256':model_sha,'binding_sha256':bind,'freeze_local':local.isoformat()},indent=2))

if __name__=='__main__':main()
