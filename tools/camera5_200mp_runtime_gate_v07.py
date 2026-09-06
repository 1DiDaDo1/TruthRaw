#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib, json
from pathlib import Path

TARGET=(16320,12288)
EXPECTED_CONTIGUOUS=TARGET[0]*TARGET[1]*2


def sha256_file(path: Path, chunk=8<<20):
    h=hashlib.sha256()
    with path.open('rb') as f:
        while True:
            b=f.read(chunk)
            if not b: break
            h.update(b)
    return h.hexdigest()


def wh(v):
    if not v or len(v)<2: return None
    return int(v[0]),int(v[1])

def rect_wh(v):
    if not v or len(v)<4:return None
    return int(v[2])-int(v[0]),int(v[3])-int(v[1])

def has_size(arr, size):
    return any(tuple(map(int,x))==tuple(size) for x in (arr or []))


def evaluate(m: dict, raw_path: Path|None=None):
    out=m.get('raw_output') or {}
    req=m.get('requested') or {}
    res=m.get('capture_result') or {}
    cc=m.get('camera_characteristics') or {}
    size=(int(out.get('width',0)),int(out.get('height',0)))
    max_pix=wh(m.get('sensor_info_pixel_array_size_maximum_resolution'))
    max_active=rect_wh(m.get('sensor_info_active_array_size_maximum_resolution'))
    max_pre=rect_wh(m.get('sensor_info_pre_correction_active_array_size_maximum_resolution'))
    highres=m.get('maximum_resolution_highres_raw_sizes') or []
    advertised=has_size(highres,TARGET)

    payload_exists = raw_path is not None and raw_path.exists()
    payload_size_ok = None
    payload_hash_ok = None
    actual_hash=None
    if payload_exists:
        actual_size=raw_path.stat().st_size
        payload_size_ok=(actual_size==int(out.get('payload_bytes',-1)))
        actual_hash=sha256_file(raw_path)
        payload_hash_ok=(actual_hash==out.get('payload_sha256')==m.get('source_identity_sha256'))

    checks={
      'device_runtime_capture':m.get('evidence_class')=='DEVICE_RUNTIME_CAPTURE',
      'honor_bkq_n49':str((m.get('device') or {}).get('make','')).upper()=='HONOR' and (m.get('device') or {}).get('model')=='BKQ-N49',
      'camera5_direct':str(m.get('physical_camera_id'))=='5' and str(m.get('opened_camera_id'))=='5',
      'focal_22p48':abs(float(m.get('focal_mm',0))-22.48)<=0.05,
      'raw_sensor_format':out.get('format')=='RAW_SENSOR',
      'exact_200mp_dimensions':size==TARGET,
      'target_is_advertised_highres_raw_sensor':advertised,
      'max_pixel_array_matches':max_pix==TARGET,
      'max_active_or_precorrection_matches':TARGET in {x for x in (max_active,max_pre) if x},
      'requested_maximum_resolution':req.get('sensor_pixel_mode')=='MAXIMUM_RESOLUTION',
      'applied_maximum_resolution':res.get('sensor_pixel_mode')=='MAXIMUM_RESOLUTION',
      'timestamp_bound':out.get('timestamp_matches_capture_result') is True and out.get('image_timestamp_ns')==res.get('sensor_timestamp_ns'),
      'payload_hash_declared':bool(out.get('payload_sha256')) and out.get('payload_sha256')==m.get('source_identity_sha256'),
      'raw_stride_plausible':int(out.get('pixel_stride',-1))==2 and int(out.get('row_stride',0))>=TARGET[0]*2,
    }
    checks['payload_file_provided']=raw_path is not None
    checks['payload_file_exists']=payload_exists
    checks['payload_file_size_matches_manifest']=payload_size_ok is True
    checks['payload_file_sha256_matches_manifest']=payload_hash_ok is True

    core_pass=all(checks.values())
    if m.get('evidence_class')!='DEVICE_RUNTIME_CAPTURE':
        classification='SIMULATION_ONLY'
    elif core_pass:
        classification='FULL_SENSOR_200MP_APP_VISIBLE_RAW_CAPTURE_PROVEN'
    else:
        classification='BLOCKED_INCOMPLETE_200MP_RUNTIME_PROOF'

    topology={
      'sensor_raw_binning_factor_used':res.get('sensor_raw_binning_factor_used'),
      'sensor_info_binning_factor':m.get('sensor_info_binning_factor') or cc.get('sensor_info_binning_factor'),
      'interpretation':(
        'REGULAR_BAYER_RESULT_REPORTED' if res.get('sensor_raw_binning_factor_used') is False else
        'BINNING_FACTOR_CFA_RESULT_REPORTED' if res.get('sensor_raw_binning_factor_used') is True else
        'RESULT_TOPOLOGY_FLAG_NOT_REPORTED'
      )
    }
    lens_shading=cc.get('lens_shading_applied_to_raw')
    untouched = (lens_shading is False)
    if classification=='FULL_SENSOR_200MP_APP_VISIBLE_RAW_CAPTURE_PROVEN' and lens_shading is True:
        purity='APP_VISIBLE_FULL_SENSOR_RAW_WITH_UPSTREAM_LENS_SHADING'
    elif classification=='FULL_SENSOR_200MP_APP_VISIBLE_RAW_CAPTURE_PROVEN':
        purity='APP_VISIBLE_FULL_SENSOR_RAW_UPSTREAM_PURITY_NOT_FULLY_PROVEN'
    else:
        purity='NOT_CLASSIFIED'

    return {
      'schema':'TruthRawCamera5_200MPRuntimeGate/0.7',
      'classification':classification,
      'pass':classification=='FULL_SENSOR_200MP_APP_VISIBLE_RAW_CAPTURE_PROVEN',
      'checks':checks,
      'observed':{
        'raw_dimensions':list(size),'maximum_pixel_array':list(max_pix) if max_pix else None,
        'row_stride':out.get('row_stride'),'pixel_stride':out.get('pixel_stride'),
        'payload_bytes':out.get('payload_bytes'),'expected_contiguous_bytes':EXPECTED_CONTIGUOUS,
        'manifest_payload_sha256':out.get('payload_sha256'),'actual_payload_sha256':actual_hash,
        'canonical_contiguous_rawsensor':out.get('canonical_contiguous_rawsensor'),
      },
      'topology':topology,
      'purity_class':purity,
      'untouched_photodiode_adc_raw_proven':False,
      'boundary':'PASS proves one real app-visible Camera2 RAW_SENSOR frame at 16320x12288, captured on Camera 5 with MAXIMUM_RESOLUTION applied and bound to the same SENSOR_TIMESTAMP and payload hash. It does not prove absence of on-sensor/HAL processing, independent ADC conversion per photodiode, electron calibration, or optical/color truth.'
    }


def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('manifest')
    ap.add_argument('--raw')
    ap.add_argument('--out',required=True)
    ns=ap.parse_args()
    m=json.load(open(ns.manifest,'r',encoding='utf-8'))
    r=evaluate(m,Path(ns.raw) if ns.raw else None)
    Path(ns.out).write_text(json.dumps(r,indent=2),encoding='utf-8')
    print(json.dumps(r,indent=2))

if __name__=='__main__': main()
