from __future__ import annotations
from pathlib import Path
from datetime import datetime
from zoneinfo import ZoneInfo
import argparse, hashlib, json, sys
import numpy as np
import tifffile

HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE))
from uncertainty_core_v5_0g import extract_dataset, source_admission, sha256_file

ROLE_NAMES=['R','G1','G2','B']
PROTOCOL=json.loads((HERE/'FROZEN_PROTOCOL_v5_0g.json').read_text())
MODEL=json.loads((HERE/'UNCERTAINTY_MODEL_v5_0g.json').read_text())
BINDING=json.loads((HERE/'BACKEND_BINDING_v5_0g.json').read_text())
CORE_CPP=Path('/mnt/data/truthraw_mobile_backends_v4_7i/native/src/core.cpp')
CORE_H=Path('/mnt/data/truthraw_mobile_backends_v4_7i/native/include/truthraw/core.h')


def capture_time_local(path:Path):
 with tifffile.TiffFile(path) as tf:
  t=tf.pages[0].tags
  x=t.get('DateTimeOriginal') or t.get('DateTime')
  if x is None:return None
  s=str(x.value)
 dt=datetime.strptime(s,'%Y:%m:%d %H:%M:%S')
 # BKQ-N49 vendor DNGs in this source class contain local wall-clock without offset.
 return dt.replace(tzinfo=ZoneInfo('Europe/Amsterdam'))

def verify_assets():
 checks={}
 for k,a in PROTOCOL['frozen_assets'].items():
  p=HERE/a['file']; got=sha256_file(p) if p.exists() else None
  checks[k]={'file':a['file'],'expected':a['sha256'],'got':got,'pass':got==a['sha256']}
 # exact production backend must still be the one to which uncertainty is bound
 cur_cpp=sha256_file(CORE_CPP) if CORE_CPP.exists() else None
 cur_h=sha256_file(CORE_H) if CORE_H.exists() else None
 checks['production_core_cpp']={'expected':BINDING['production_backend_files']['core.cpp'],'got':cur_cpp,'pass':cur_cpp==BINDING['production_backend_files']['core.cpp']}
 checks['production_core_h']={'expected':BINDING['production_backend_files']['core.h'],'got':cur_h,'pass':cur_h==BINDING['production_backend_files']['core.h']}
 return all(v['pass'] for v in checks.values()),checks

def predict(X,role,snr):
 coef=np.asarray(MODEL['coefficients'],float);inter=float(MODEL['intercept']);eps=float(MODEL['log_epsilon'])
 mu=np.maximum(np.exp(inter + np.asarray(X,float)@coef)-eps,1e-6)
 bins=np.asarray(MODEL['snr_bins'][1:-1],float); bi=np.searchsorted(bins,snr,side='right')
 p50=np.empty(len(mu),float);p95=np.empty(len(mu),float)
 for rc,name in enumerate(ROLE_NAMES):
  for b in range(5):
   m=(role==rc)&(bi==b)
   if not np.any(m):continue
   c=MODEL['calibration'][name][str(b)]
   p50[m]=mu[m]*float(c['q50_factor']);p95[m]=mu[m]*float(c['q95_factor'])
 p50=np.maximum(p50,1e-8);p95=np.maximum(p95,p50+1e-8)
 return p50,p95

def score(y,p50,p95):
 edges=np.quantile(p95,[0,.2,.4,.6,.8,1]);qs=[]
 for i in range(5):
  m=(p95>=edges[i])&((p95<edges[i+1]) if i<4 else p95<=edges[i+1])
  obs95=float(np.percentile(y[m],95));pred=float(np.median(p95[m]))
  qs.append({'q':i+1,'n':int(m.sum()),'predicted_p95_median':pred,'observed_error_median':float(np.median(y[m])),'observed_error_p95':obs95,'p95_ratio_pred_to_observed':float(pred/max(obs95,1e-12)),'coverage_p95':float(np.mean(y[m]<=p95[m]))})
 return {'n':int(len(y)),'coverage_p50':float(np.mean(y<=p50)),'coverage_p95':float(np.mean(y<=p95)),'observed_median_stage2':float(np.median(y)),'observed_p95_stage2':float(np.percentile(y,95)),'predicted_p50_median_stage2':float(np.median(p50)),'predicted_p95_median_stage2':float(np.median(p95)),'risk_quintiles':qs,'risk_ordering':bool(qs[-1]['observed_error_median']>1.5*qs[0]['observed_error_median'])}

def main():
 ap=argparse.ArgumentParser(description='TruthRaw v5.0g frozen prospective tele uncertainty evaluator')
 ap.add_argument('dng',type=Path);ap.add_argument('--out',type=Path)
 a=ap.parse_args();path=a.dng.resolve()
 asset_ok,assets=verify_assets();digest=sha256_file(path);src=source_admission(path);ct=capture_time_local(path);freeze=datetime.fromisoformat(PROTOCOL['frozen_at_europe_amsterdam'])
 adm_checks={
  'asset_integrity':asset_ok,
  'source_class_exact':bool(src.get('pass')),
  'sha256_not_historical':digest not in PROTOCOL['historical_development_sha256'],
  'capture_timestamp_present':ct is not None,
  'capture_strictly_after_freeze':ct is not None and ct>freeze,
 }
 result={'schema':'TruthRawProspectiveTeleUncertaintyResult/5.0g','file':str(path),'sha256':digest,'frozen_at':PROTOCOL['frozen_at_europe_amsterdam'],'capture_time_assumed_europe_amsterdam':None if ct is None else ct.isoformat(),'asset_integrity':assets,'source_admission':src,'admission_checks':adm_checks,'uncertainty_binding_sha256':MODEL['uncertainty_binding_sha256'],'claim_boundary':BINDING['claim_boundary']}
 if not all(adm_checks.values()):
  result['decision']='REJECT_BEFORE_HIDDEN_CFA_SCORING';result['ptc_uncertainty_state']='UNCHANGED_WAITING_VALID_PROSPECTIVE_HOLDOUT'
  txt=json.dumps(result,indent=2);print(txt);a.out and a.out.write_text(txt);raise SystemExit(2)
 X,y,role,snr,coord,rep=extract_dataset(path)
 if rep['feature_schema_sha256']!=MODEL['feature_schema_sha256']:
  raise RuntimeError('feature schema hash mismatch')
 p50,p95=predict(X,role,snr);sc=score(y,p50,p95);g=PROTOCOL['prospective_gates']
 ratios=[q['p95_ratio_pred_to_observed'] for q in sc['risk_quintiles']]
 checks={
  'minimum_uncensored_targets':sc['n']>=int(g['minimum_uncensored_targets']),
  'p50_coverage':g['p50_coverage_range'][0]<=sc['coverage_p50']<=g['p50_coverage_range'][1],
  'p95_coverage':g['p95_coverage_range'][0]<=sc['coverage_p95']<=g['p95_coverage_range'][1],
  'risk_ordering':sc['risk_ordering'] if g['risk_ordering_required'] else True,
  'all_risk_quintile_p95_ratios':all(g['risk_quintile_p95_ratio_range'][0]<=v<=g['risk_quintile_p95_ratio_range'][1] for v in ratios),
  'float_scene_linear_no_clamp':True,
  'source_white_excluded_from_exact_value_scoring':True,
 }
 passed=all(checks.values())
 result['dataset_summary']={'targets_uncensored':rep['targets_uncensored'],'source_white_censored':rep['targets_source_white_censored'],'stage2_min':rep['source']['stage2_min'],'stage2_max':rep['source']['stage2_max']}
 result['score']=sc;result['prospective_checks']=checks
 result['decision']=PROTOCOL['decision_if_pass'] if passed else PROTOCOL['decision_if_fail']
 result['ptc_uncertainty_state']='UNCERTAINTY_BLOCKER_CLOSED_FOR_THIS_EXACT_TELE_SOURCE_CLASS_AND_BACKEND' if passed else 'PROSPECTIVE_FAIL_REQUIRES_REVIEW_NO_RETUNE_ON_THIS_HOLDOUT'
 txt=json.dumps(result,indent=2);print(txt);a.out and a.out.write_text(txt);raise SystemExit(0 if passed else 3)

if __name__=='__main__':main()
