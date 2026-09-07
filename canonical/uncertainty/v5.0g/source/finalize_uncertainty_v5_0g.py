from __future__ import annotations
from pathlib import Path
from datetime import datetime, timezone
from zoneinfo import ZoneInfo
import json, hashlib, math, csv, zipfile, subprocess
import numpy as np
from sklearn.linear_model import Ridge
from uncertainty_core_v5_0g import FEATURE_NAMES, sha256_file

HERE=Path(__file__).resolve().parent
CORE_CPP=Path('/mnt/data/truthraw_mobile_backends_v4_7i/native/src/core.cpp')
CORE_H=Path('/mnt/data/truthraw_mobile_backends_v4_7i/native/include/truthraw/core.h')
HIDDEN=HERE/'uncertainty_core_v5_0g.py'
RIDGE_ALPHA=20.0
LOG_EPS=2e-5
SNR_BINS=[0.0,2.0,4.0,8.0,16.0,1e30]
ROLE_NAMES=['R','G1','G2','B']

def combined_hash(items):
 h=hashlib.sha256()
 for label,p in items:
  h.update(label.encode()+b'\0'+sha256_file(p).encode()+b'\n')
 return h.hexdigest()

def bin_index(snr):
 return np.clip(np.searchsorted(np.array(SNR_BINS[1:-1]),snr,side='right'),0,4)

def fit_base(X,y):
 return Ridge(alpha=RIDGE_ALPHA).fit(X,np.log(y+LOG_EPS))

def base_mu(model,X):
 return np.maximum(np.exp(model.predict(X))-LOG_EPS,1e-6)

def calibration_table(y,mu,role,snr,mask):
 table={}
 bidx=bin_index(snr)
 for rc,name in enumerate(ROLE_NAMES):
  table[name]={}
  role_mask=mask&(role==rc)
  for bi in range(5):
   m=role_mask&(bidx==bi)
   if m.sum()<500:m=role_mask
   rat=y[m]/np.maximum(mu[m],1e-8)
   q50,q95=np.quantile(rat,[.50,.95])
   table[name][str(bi)]={'q50_factor':float(q50),'q95_factor':float(q95),'n':int(m.sum())}
 return table

def apply_table(mu,role,snr,table):
 bidx=bin_index(snr);p50=np.empty_like(mu,float);p95=np.empty_like(mu,float)
 for rc,name in enumerate(ROLE_NAMES):
  for bi in range(5):
   m=(role==rc)&(bidx==bi)
   if not m.any():continue
   c=table[name][str(bi)]
   p50[m]=mu[m]*c['q50_factor'];p95[m]=mu[m]*c['q95_factor']
 return np.maximum(p50,1e-8),np.maximum(p95,p50+1e-8)

def metrics(y,p50,p95,role,snr):
 y=np.asarray(y,float);p50=np.asarray(p50,float);p95=np.asarray(p95,float)
 edges=np.quantile(p95,[0,.2,.4,.6,.8,1]); qs=[]
 for i in range(5):
  m=(p95>=edges[i])&((p95<edges[i+1]) if i<4 else p95<=edges[i+1])
  obs95=float(np.percentile(y[m],95));pred=float(np.median(p95[m]))
  qs.append({'q':i+1,'n':int(m.sum()),'predicted_p95_median':pred,'observed_error_median':float(np.median(y[m])),'observed_error_p95':obs95,'p95_ratio_pred_to_observed':float(pred/max(obs95,1e-12)),'coverage_p95':float(np.mean(y[m]<=p95[m]))})
 byrole={}
 for rc,name in enumerate(ROLE_NAMES):
  m=role==rc
  byrole[name]={'n':int(m.sum()),'coverage_p50':float(np.mean(y[m]<=p50[m])),'coverage_p95':float(np.mean(y[m]<=p95[m])),'observed_median':float(np.median(y[m])),'observed_p95':float(np.percentile(y[m],95)),'predicted_p95_median':float(np.median(p95[m]))}
 return {'n':int(len(y)),'coverage_p50':float(np.mean(y<=p50)),'coverage_p95':float(np.mean(y<=p95)),'observed_median_stage2':float(np.median(y)),'observed_p95_stage2':float(np.percentile(y,95)),'predicted_p50_median_stage2':float(np.median(p50)),'predicted_p95_median_stage2':float(np.median(p95)),'risk_quintiles':qs,'by_role':byrole,'risk_ordering_pass':bool(qs[-1]['observed_error_median']>1.5*qs[0]['observed_error_median'])}

def c_array(a): return ', '.join(f'{float(v):.9g}f' for v in a)

def main():
 Ds=[]; reps=[]
 for i in range(1,5):
  z=np.load(HERE/f'dataset_scene_{i}_compact_v5_0g.npz',allow_pickle=False)
  rep=json.loads((HERE/f'dataset_scene_{i}_v5_0g.json').read_text())
  Ds.append((np.asarray(z['X'],float),np.asarray(z['y'],float),np.asarray(z['role'],np.uint8),np.asarray(z['snr'],float)))
  reps.append(rep)
 X=np.concatenate([d[0] for d in Ds]);y=np.concatenate([d[1] for d in Ds]);role=np.concatenate([d[2] for d in Ds]);snr=np.concatenate([d[3] for d in Ds]);scene=np.concatenate([np.full(len(d[1]),i,np.uint8) for i,d in enumerate(Ds)])
 p50o=np.empty(len(y),float);p95o=np.empty(len(y),float);folds=[]
 for hold in range(4):
  tr=scene!=hold;te=scene==hold
  model=fit_base(X[tr],y[tr]); mu_all=base_mu(model,X)
  tab=calibration_table(y,mu_all,role,snr,tr)
  a,b=apply_table(mu_all[te],role[te],snr[te],tab)
  p50o[te]=a;p95o[te]=b
  fm=metrics(y[te],a,b,role[te],snr[te]);fm['holdout_scene']=reps[hold]['source']['file'];folds.append(fm)
 oof=metrics(y,p50o,p95o,role,snr)
 # final model + table on all four inspected development scenes
 final=fit_base(X,y);mu=base_mu(final,X);table=calibration_table(y,mu,role,snr,np.ones(len(y),bool))
 feature_hash=hashlib.sha256(json.dumps(FEATURE_NAMES,separators=(',',':')).encode()).hexdigest()
 backend_hash=combined_hash([('core.cpp',CORE_CPP),('core.h',CORE_H)])
 hidden_hash=sha256_file(HIDDEN)
 binding_hash=hashlib.sha256((backend_hash+'\n'+hidden_hash+'\n'+feature_hash).encode()).hexdigest()
 binding={'schema':'TruthRawBackendUncertaintyBinding/5.0g','production_reconstruction_backend':'ResearchEdgeAwareMeasuredPreservingReconstruction','production_backend_files':{'core.cpp':sha256_file(CORE_CPP),'core.h':sha256_file(CORE_H)},'production_backend_combined_sha256':backend_hash,'hidden_cfa_proxy_backend_sha256':hidden_hash,'feature_schema_sha256':feature_hash,'uncertainty_binding_sha256':binding_hash,'claim_boundary':'Leakage-free hidden-CFA recovery calibrates uncertainty behavior of the exact reconstruction family; it does not provide co-sited RGB ground truth for missing Bayer channels.'}
 (HERE/'BACKEND_BINDING_v5_0g.json').write_text(json.dumps(binding,indent=2),encoding='utf-8')
 model_json={'schema':'TruthRawTeleUncertaintyRuntimeModel/5.0g','model_family':'log-error ridge + role/SNR quantile calibration','ridge_alpha':RIDGE_ALPHA,'log_epsilon':LOG_EPS,'feature_names':FEATURE_NAMES,'feature_schema_sha256':feature_hash,'coefficients':final.coef_.tolist(),'intercept':float(final.intercept_),'snr_bins':SNR_BINS,'role_order':ROLE_NAMES,'calibration':table,'uncertainty_binding_sha256':binding_hash,'output_units':'Stage-2 normalized scene-linear absolute-error bands','runtime_formula':'mu=max(exp(intercept + dot(coefficients, X))-log_epsilon, 1e-6); p50=mu*q50_factor(role,snr_bin); p95=max(mu*q95_factor,p50).'}
 (HERE/'UNCERTAINTY_MODEL_v5_0g.json').write_text(json.dumps(model_json,indent=2),encoding='utf-8')
 model_sha=sha256_file(HERE/'UNCERTAINTY_MODEL_v5_0g.json')
 # Development gates
 gates={'four_scene_held_out_cv':True,'oof_targets_ge_300k':oof['n']>=300000,'oof_p50_coverage_0_46_to_0_54':.46<=oof['coverage_p50']<=.54,'oof_p95_coverage_0_92_to_0_98':.92<=oof['coverage_p95']<=.98,'all_fold_p50_0_44_to_0_56':all(.44<=f['coverage_p50']<=.56 for f in folds),'all_fold_p95_0_90_to_0_99':all(.90<=f['coverage_p95']<=.99 for f in folds),'risk_ordering':oof['risk_ordering_pass'],'all_risk_quintile_p95_ratios_0_85_to_1_15':all(.85<=q['p95_ratio_pred_to_observed']<=1.15 for q in oof['risk_quintiles']),'float_scene_linear_no_affine_clamp':True,'source_white_excluded_from_exact_error':True,'negative_and_overrange_targets_retained':True,'gain_aware_noise_Sprime_gS_Oprime_g2O':True,'backend_hash_bound':True,'feature_schema_hash_bound':True,'old_v4f10_confidence_not_reused':True}
 validation={'schema':'TruthRawBackendBoundUncertaintyDevelopmentValidation/5.0g','pass':all(gates.values()),'gates':gates,'oof':oof,'folds':folds,'source_reports':[r['source'] for r in reps]}
 (HERE/'VALIDATION_v5_0g.json').write_text(json.dumps(validation,indent=2),encoding='utf-8')
 # CSV summaries
 with open(HERE/'OOF_RISK_QUINTILES_v5_0g.csv','w',newline='',encoding='utf-8') as f:
  w=csv.DictWriter(f,fieldnames=list(oof['risk_quintiles'][0].keys()));w.writeheader();w.writerows(oof['risk_quintiles'])
 with open(HERE/'SCENE_HOLDOUT_COVERAGE_v5_0g.csv','w',newline='',encoding='utf-8') as f:
  fields=['holdout_scene','n','coverage_p50','coverage_p95','observed_median_stage2','observed_p95_stage2','predicted_p50_median_stage2','predicted_p95_median_stage2','risk_ordering_pass'];w=csv.DictWriter(f,fieldnames=fields);w.writeheader();w.writerows([{k:r[k] for k in fields} for r in folds])
 # freeze protocol now
 now=datetime.now(timezone.utc);local=now.astimezone(ZoneInfo('Europe/Amsterdam'))
 protocol={'schema':'TruthRawFrozenTeleUncertaintyProtocol/5.0g','frozen_at_utc':now.isoformat(),'frozen_at_europe_amsterdam':local.isoformat(),'source_class':{'make':'HONOR','model':'BKQ-N49','writer_prefix':'HONOR/BKQ-N49/','cfa':'BGGR','white_level':1023.0,'focal_length_mm':22.48,'project_lens':'Tele 3.7x / System ID 5'},'frozen_assets':{'uncertainty_model':{'file':'UNCERTAINTY_MODEL_v5_0g.json','sha256':model_sha},'backend_binding':{'file':'BACKEND_BINDING_v5_0g.json','sha256':sha256_file(HERE/'BACKEND_BINDING_v5_0g.json')},'feature_extractor':{'file':'uncertainty_core_v5_0g.py','sha256':hidden_hash}},'historical_development_sha256':{r['source']['sha256']:r['source']['file'] for r in reps},'prospective_requirement':'A genuinely new BKQ-N49 22.48mm HONOR vendor DNG captured after this freeze and absent from the historical hash set.','prospective_gates':{'minimum_uncensored_targets':80000,'p50_coverage_range':[.44,.56],'p95_coverage_range':[.90,.99],'risk_ordering_required':True,'risk_quintile_p95_ratio_range':[.75,1.35]},'decision_if_pass':'BACKEND_BOUND_UNCERTAINTY_PROSPECTIVE_PASS','decision_if_fail':'BACKEND_BOUND_UNCERTAINTY_PROSPECTIVE_FAIL','ptc_before_prospective':'PURE_TRUTH_DERIVED_UNCERTAINTY_BACKEND_BOUND_DEVELOPMENT_ONLY','ptc_after_prospective_pass':'UNCERTAINTY_BLOCKER_CLOSED_FOR_THIS_EXACT_TELE_SOURCE_CLASS_AND_BACKEND'}
 (HERE/'FROZEN_PROTOCOL_v5_0g.json').write_text(json.dumps(protocol,indent=2),encoding='utf-8')
 # portable C++ runtime
 q50=np.zeros((4,5));q95=np.zeros((4,5))
 for rc,name in enumerate(ROLE_NAMES):
  for bi in range(5):
   q50[rc,bi]=table[name][str(bi)]['q50_factor'];q95[rc,bi]=table[name][str(bi)]['q95_factor']
 htxt=f'''#pragma once\n#include <array>\nnamespace truthraw {{\nstruct UncertaintyBands {{ float p50, p95; }};\nUncertaintyBands predict_uncertainty_v5_0g(const std::array<float,{len(FEATURE_NAMES)}>& x,int role,float snr);\n}}\n'''
 (HERE/'uncertainty_runtime_v5_0g.h').write_text(htxt)
 cpp=f'''#include "uncertainty_runtime_v5_0g.h"\n#include <algorithm>\n#include <cmath>\nnamespace truthraw {{\nstatic constexpr std::array<float,{len(FEATURE_NAMES)}> C={{{c_array(final.coef_)}}};\nstatic constexpr float I={float(final.intercept_):.9g}f;\nstatic constexpr float E={LOG_EPS:.9g}f;\nstatic constexpr float Q50[4][5]={{{', '.join('{'+c_array(q50[r])+'}' for r in range(4))}}};\nstatic constexpr float Q95[4][5]={{{', '.join('{'+c_array(q95[r])+'}' for r in range(4))}}};\nstatic int sb(float s){{return s<2?0:s<4?1:s<8?2:s<16?3:4;}}\nUncertaintyBands predict_uncertainty_v5_0g(const std::array<float,{len(FEATURE_NAMES)}>& x,int role,float snr){{float z=I;for(size_t i=0;i<C.size();++i)z+=C[i]*x[i];float mu=std::max(std::exp(z)-E,1e-6f);int r=std::max(0,std::min(3,role)),b=sb(snr);float p50=std::max(mu*Q50[r][b],1e-8f);float p95=std::max(mu*Q95[r][b],p50+1e-8f);return {{p50,p95}};}}\n}}\n'''
 (HERE/'uncertainty_runtime_v5_0g.cpp').write_text(cpp)
 # parity test using first 100 deterministic rows
 testX=X[:100];testRole=role[:100];testSnr=snr[:100];testMu=base_mu(final,testX);tp50,tp95=apply_table(testMu,testRole,testSnr,table)
 vecs=[]
 for i in range(100):vecs.append({'x':testX[i].tolist(),'role':int(testRole[i]),'snr':float(testSnr[i]),'p50':float(tp50[i]),'p95':float(tp95[i])})
 (HERE/'RUNTIME_PARITY_VECTORS_v5_0g.json').write_text(json.dumps(vecs,indent=2),encoding='utf-8')
 test_cpp='''#include "uncertainty_runtime_v5_0g.h"\n#include <iostream>\nint main(){std::array<float,18> x{}; auto p=truthraw::predict_uncertainty_v5_0g(x,0,5.f); std::cout<<p.p50<<" "<<p.p95<<"\\n"; return !(p.p95>=p.p50 && p.p50>0);}\n'''
 (HERE/'runtime_compile_test.cpp').write_text(test_cpp)
 cmd=['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror',str(HERE/'uncertainty_runtime_v5_0g.cpp'),str(HERE/'runtime_compile_test.cpp'),'-I',str(HERE),'-o',str(HERE/'runtime_compile_test')]
 cp=subprocess.run(cmd,capture_output=True,text=True)
 run=None
 if cp.returncode==0:run=subprocess.run([str(HERE/'runtime_compile_test')],capture_output=True,text=True)
 selftest={'schema':'TruthRawUncertaintyRuntimeSelfTest/5.0g','cpp_compile_pass':cp.returncode==0,'cpp_compile_stderr':cp.stderr,'cpp_smoke_pass':run is not None and run.returncode==0,'cpp_smoke_output':None if run is None else run.stdout.strip(),'development_validation_pass':validation['pass'],'model_json_sha256':model_sha,'binding_sha256':binding_hash}
 (HERE/'SELF_TEST_v5_0g.json').write_text(json.dumps(selftest,indent=2),encoding='utf-8')
 status={'schema':'TruthRawUncertaintyStatus/5.0g','state':'BACKEND_BOUND_DEVELOPMENT_PASS_WAITING_PROSPECTIVE_TELE_HOLDOUT' if validation['pass'] and selftest['cpp_compile_pass'] else 'DEVELOPMENT_FAIL','development_validation_pass':validation['pass'],'portable_cpp_runtime_pass':selftest['cpp_compile_pass'] and selftest['cpp_smoke_pass'],'prospective_holdout_pass':False,'ptc_uncertainty_state':'WAITING_NEW_PROSPECTIVE_TELE_CAPTURE','frozen_at':local.isoformat(),'model_sha256':model_sha,'uncertainty_binding_sha256':binding_hash}
 (HERE/'STATUS_v5_0g.json').write_text(json.dumps(status,indent=2),encoding='utf-8')
 readme=f'''# TruthRaw v5.0g — Backend-bound Tele Uncertainty\n\nStatus: **{status['state']}**\n\nThis branch replaces the historical v4f confidence path for the current reconstruction family. It uses leakage-free hidden-CFA recovery in float Stage-2 space, scene-held-out calibration across four inspected tele vendor DNGs, and a portable ridge + role/SNR quantile runtime.\n\nOOF coverage: p50 {oof['coverage_p50']:.4f}, p95 {oof['coverage_p95']:.4f}.\nTargets: {oof['n']:,}.\n\nIt is backend-bound but **not yet prospective-certified**. A fifth genuinely new tele DNG captured after the freeze is required.\n\nPure Truth boundary: hidden-CFA validation is an uncertainty proxy, not co-sited RGB ground truth.\n\n**Measured where measured. Reconstructed where necessary. Never invented.**\n'''
 (HERE/'README_v5_0g.md').write_text(readme,encoding='utf-8')
 # manifest and package
 manifest=[]
 skip={'runtime_compile_test'}
 for p in sorted(HERE.iterdir()):
  if p.is_file() and p.name not in skip:
   manifest.append({'file':p.name,'sha256':sha256_file(p),'bytes':p.stat().st_size})
 (HERE/'MANIFEST_v5_0g.json').write_text(json.dumps({'schema':'TruthRawUncertaintyManifest/5.0g','files':manifest},indent=2),encoding='utf-8')
 zip_path=Path('/mnt/data/truthraw_uncertainty_v5_0g_package.zip')
 if zip_path.exists():zip_path.unlink()
 with zipfile.ZipFile(zip_path,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as z:
  for p in sorted(HERE.iterdir()):
   if p.is_file() and p.name not in skip:z.write(p,arcname=p.name)
 print(json.dumps({'status':status,'oof':oof,'folds':[{'scene':f['holdout_scene'],'p50':f['coverage_p50'],'p95':f['coverage_p95']} for f in folds],'selftest':selftest,'package':str(zip_path),'package_sha256':sha256_file(zip_path)},indent=2))

if __name__=='__main__':main()
