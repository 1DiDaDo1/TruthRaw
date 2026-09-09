from __future__ import annotations
import argparse, json, hashlib, struct, math
from pathlib import Path
import numpy as np
import tifffile
from PIL import Image

DT=tifffile.DATATYPE
ENC_BLACK=4096
WINDOWS=(1.25,2.0,4.0,8.0,16.0,32.0,64.0)


def sha256_file(path: Path):
    h=hashlib.sha256()
    with open(path,'rb') as f:
        for b in iter(lambda:f.read(16*1024*1024),b''): h.update(b)
    return h.hexdigest()

def sha256_array(a):
    h=hashlib.sha256(); mv=memoryview(np.ascontiguousarray(a)).cast('B')
    for i in range(0,len(mv),16*1024*1024): h.update(mv[i:i+16*1024*1024])
    return h.hexdigest()

def val(t,name,default=None): return t[name].value if name in t else default

def tag_copy(t,name,writeonce=True):
    if name not in t:return None
    x=t[name]; return (int(x.code),int(x.dtype),int(x.count),x.value,writeonce)

def rat_float(x):
    a=np.asarray(x)
    if a.ndim==1 and a.size==2 and float(a[1])!=0: return float(a[0])/float(a[1])
    try:return float(a.ravel()[0])
    except:return 0.0

def build(src:Path,scene_path:Path,preview_path:Path,out:Path,key:str):
    side=out.with_suffix(out.suffix+'.truthraw.json')
    src_sha=sha256_file(src)
    with tifffile.TiffFile(src) as tf:
        sp=tf.pages[0]; st=sp.tags
        src_meta={n:val(st,n) for n in [
            'DNGVersion','DNGBackwardVersion','UniqueCameraModel','Make','Model','Orientation',
            'DefaultScale','DefaultCropOrigin','DefaultCropSize','ActiveArea','ColorMatrix1','ColorMatrix2',
            'CameraCalibration1','CameraCalibration2','AnalogBalance','AsShotNeutral','BaselineExposure',
            'BaselineNoise','BaselineSharpness','ShadowScale','CalibrationIlluminant1','CalibrationIlluminant2',
            'ForwardMatrix1','ForwardMatrix2','DateTime','ExposureTime','FNumber','FocalLength']}
        source_raw=np.asarray(sp.asarray(),dtype=np.uint16)
        source_white=float(np.asarray(val(st,'WhiteLevel')).ravel()[0])
        source_clip_count=int(np.count_nonzero(source_raw>=source_white))
        source_raw_sha=sha256_array(source_raw)
        copy_tags={n:tag_copy(st,n,True) for n in [
            'DNGVersion','DNGBackwardVersion','UniqueCameraModel','Make','Model','DefaultScale','DefaultCropOrigin','DefaultCropSize','ActiveArea',
            'ColorMatrix1','ColorMatrix2','CameraCalibration1','CameraCalibration2','AnalogBalance','AsShotNeutral','BaselineNoise','BaselineSharpness','ShadowScale',
            'CalibrationIlluminant1','CalibrationIlluminant2','ForwardMatrix1','ForwardMatrix2','ExposureTime','FNumber','FocalLength']}

    with open(scene_path,'rb') as f: magic,w,h=struct.unpack('<III',f.read(12))
    if magic!=0x54525343 or (w,h)!=(4080,3072): raise RuntimeError('bad scene header')
    scene=np.memmap(scene_path,dtype='<f4',mode='r',offset=12,shape=(h,w,3),order='C')
    if not np.isfinite(scene).all(): raise RuntimeError('non-finite scene')
    scene_min=float(scene.min()); scene_max=float(scene.max())
    window=next((x for x in WINDOWS if scene_max <= x),None)
    if window is None: raise RuntimeError(f'scene max {scene_max} exceeds supported DNG compatibility windows')
    enc_scale=(65535-ENC_BLACK)/window
    enc_float=ENC_BLACK+np.asarray(scene,dtype=np.float32)*enc_scale
    below=int(np.count_nonzero(enc_float<0)); above=int(np.count_nonzero(enc_float>65535))
    if below or above: raise RuntimeError(f'compatibility storage clips below={below}, above={above}')
    raw16=np.rint(enc_float).astype(np.uint16)
    decoded=(raw16.astype(np.float32)-ENC_BLACK)/enc_scale
    quant_err=float(np.max(np.abs(decoded-np.asarray(scene,dtype=np.float32))))
    raw_sha=sha256_array(raw16)

    preview=np.asarray(Image.open(preview_path).convert('RGB'),dtype=np.uint8)
    preview_sha=sha256_array(preview)
    source_baseline=rat_float(src_meta.get('BaselineExposure',(0,1)))
    compat_ev=math.log2(window)
    out_baseline=source_baseline+compat_ev
    be_num=int(round(out_baseline*1_000_000)); be_den=1_000_000

    provenance={
      'schema':'TruthRawLinearColorPreviewCandidate/0.6',
      'classification':'DERIVED_RECONSTRUCTED_LINEAR_RAW_DNG_COMPATIBILITY_PROJECTION',
      'candidate_status':'RESEARCH_CANDIDATE_REQUIRES_DNG_SDK_1_7_1_2724_VALIDATION',
      'source':{'file':src.name,'sha256':src_sha,'raw_sample_sha256':source_raw_sha,
        'source_white_level':source_white,'source_white_censored_count':source_clip_count,
        'capture_orientation':int(src_meta['Orientation']) if src_meta['Orientation'] is not None else None},
      'scientific_master':{'representation':'full-resolution signed scene-linear camera RGB',
        'reconstruction':'current TruthRaw v4.7i measured-preserving scene master; measured CFA component reinjected',
        'gainmap_application':'exactly once before/within reconstruction path','measured_component_max_abs_error_before_export':0.0,
        'tone_curve_baked':False,'sharpening_baked':False,'output_acutance_baked':False,
        'scene_min':scene_min,'scene_max':scene_max,
        'master_boundary':'The unrestricted Scene Master is authoritative; this DNG is a finite compatibility projection.'},
      'linearraw_storage':{'photometric_interpretation':34892,'bits_per_sample':16,'compression':1,
        'black_level':[ENC_BLACK]*3,'white_level':[65535]*3,'truthraw_scene_window_white':window,
        'truthraw_scene_codes_per_unit':enc_scale,
        'formula':f'stored_code = round({ENC_BLACK} + scene_linear * {enc_scale:.12g})',
        'dng_normalized_relation':f'DNG_linear_reference ~= TruthRaw_scene_linear / {window}',
        'baseline_exposure_compensation_ev':compat_ev,'output_baseline_exposure_ev':out_baseline,
        'negative_raw_codes_below_black_preserved_in_container':True,
        'truthraw_over_one_values_within_window_preserved_without_exceeding_DNG_WhiteLevel':True,
        'storage_clip_count':below+above,'max_quantization_abs_scene_error':quant_err,
        'opcode_list2_copied':False,'noise_profile_copied':False,
        'standards_reason':'DNG 1.7.1 Chapter 5 says rescaled raw values above 1.0 should be clipped; therefore v0.6 never stores scientific overrange above the DNG WhiteLevel. A virtual LinearRaw headroom window plus BaselineExposure carries the finite compatibility projection.'},
      'preview':{'ifd':'IFD0 reduced RGB preview','orientation':1,'shape':list(preview.shape),
        'pipeline':'Colorimetric V3 L1: AsShotNeutral + CameraCalibration + normalized ForwardMatrix -> XYZ D50 -> Bradford D65 -> linear sRGB -> scalar display exposure -> hue-preserving shoulder -> sRGB transfer',
        'appearance_only':True,'l1_source_bound_color':True,'l2_target_calibrated_color':False,'spectral_truth_claimed':False},
      'claim_boundary':'Source CFA remains evidence; LinearRaw RGB is derived. DNG is standards-oriented finite compatibility output, not the unrestricted TruthRaw master.'}

    root_extra=[]
    for n,q in copy_tags.items():
        if q is not None: root_extra.append(q)
    root_extra += [
      (274,DT.SHORT,1,1,False),
      (50730,DT.SRATIONAL,1,(be_num,be_den),False),
      (50966,DT.ASCII,0,'TruthRaw',False),(50967,DT.ASCII,0,'0.6-standard-compat',False),
      (50968,DT.ASCII,0,'TruthRaw Colorimetric V3 L1 Preview',False),(50970,DT.LONG,1,2,False)]
    raw_extra=[
      (274,DT.SHORT,1,int(src_meta['Orientation']),False),(50713,DT.SHORT,2,(1,1),False),
      (50714,DT.RATIONAL,3,(ENC_BLACK,1,ENC_BLACK,1,ENC_BLACK,1),False),
      (50717,DT.LONG,3,(65535,65535,65535),False),
      (50718,DT.RATIONAL,2,src_meta['DefaultScale'],False),(50719,DT.LONG,2,src_meta['DefaultCropOrigin'],False),
      (50720,DT.LONG,2,src_meta['DefaultCropSize'],False),(50829,DT.LONG,4,src_meta['ActiveArea'],False),
      (50935,DT.RATIONAL,1,(1,1),False)]
    desc='TruthRaw provenance: '+json.dumps(provenance,separators=(',',':'),sort_keys=True)
    with tifffile.TiffWriter(out,byteorder='<',bigtiff=False) as tw:
        tw.write(preview,photometric='rgb',planarconfig='contig',bitspersample=8,compression=None,metadata=None,
          subfiletype=1,subifds=1,rowsperstrip=min(128,preview.shape[0]),software='TruthRaw 0.6 Standard Compatibility Candidate',description=desc,extratags=root_extra)
        tw.write(raw16,photometric=34892,planarconfig='contig',bitspersample=16,compression=None,metadata=None,
          subfiletype=0,rowsperstrip=64,description='TruthRaw derived LinearRaw finite compatibility projection',extratags=raw_extra)

    with tifffile.TiffFile(out) as tf:
        root=tf.pages[0]; sub=root.pages[0]; gotprev=np.asarray(root.asarray(),dtype=np.uint8); gotraw=np.asarray(sub.asarray(),dtype=np.uint16)
        checks={
          'preview_pixel_exact':bool(np.array_equal(gotprev,preview)),'raw_payload_exact':bool(np.array_equal(gotraw,raw16)),
          'root_preview_shape':list(gotprev.shape),'root_orientation':int(root.tags['Orientation'].value),
          'root_baseline_exposure':root.tags['BaselineExposure'].value,
          'raw_shape':list(gotraw.shape),'raw_photometric':int(sub.photometric),'raw_compression':int(sub.compression),
          'raw_orientation':int(sub.tags['Orientation'].value),'raw_black_level':sub.tags['BlackLevel'].value,
          'raw_white_level':sub.tags['WhiteLevel'].value,'raw_has_opcode_list2':'OpcodeList2' in sub.tags,'raw_has_noise_profile':'NoiseProfile' in sub.tags,
          'max_stored_code':int(gotraw.max()),'min_stored_code':int(gotraw.min()),
          'all_codes_at_or_below_whitelevel':bool(int(gotraw.max())<=65535)}
        if not checks['preview_pixel_exact'] or not checks['raw_payload_exact']: raise RuntimeError('roundtrip failed')
        if checks['raw_photometric']!=34892 or checks['raw_compression']!=1: raise RuntimeError('structure failed')
        if checks['raw_has_opcode_list2'] or checks['raw_has_noise_profile']: raise RuntimeError('forbidden tag copied')
    report={**provenance,'output':{'file':out.name,'bytes':out.stat().st_size,'sha256':sha256_file(out),'linearraw_payload_sha256':raw_sha,'embedded_preview_rgb_sha256':preview_sha},
      'validation':{'tifffile_structure_and_payload_roundtrip':'PASS','checks':checks,'scene_decode_max_abs_error':quant_err,'storage_clipping_count':below+above,
      'dng_sdk_1_7_1_2724_for_this_exact_hash':'NOT_RUN','older_2611_validation_inheritance':'FORBIDDEN_NEW_HASH'}}
    side.write_text(json.dumps(report,indent=2,default=lambda x:int(x) if isinstance(x,np.integer) else float(x) if isinstance(x,np.floating) else str(x)),encoding='utf-8')
    return report

if __name__=='__main__':
    ap=argparse.ArgumentParser(); ap.add_argument('--key',required=True); ap.add_argument('--suffix',required=True); a=ap.parse_args()
    src=Path(f'/mnt/data/IMG_BNC_TRUTHRAW20260907_{a.key}_{a.suffix}.dng')
    scene=Path(f'/mnt/data/truthraw_dng_build/{a.key}.scene.bin'); preview=Path(f'/mnt/data/truthraw_dng_build/{a.key}_V3_PREVIEW.jpg')
    out=Path(f'/mnt/data/IMG_BNC_TRUTHRAW20260907_{a.key}_{a.suffix}__TRUTHRAW_LINEAR_V06_STANDARD_COLOR_PREVIEW.dng')
    r=build(src,scene,preview,out,a.key); print(json.dumps({'file':r['output']['file'],'sha256':r['output']['sha256'],'bytes':r['output']['bytes'],'scene_min':r['scientific_master']['scene_min'],'scene_max':r['scientific_master']['scene_max'],'window':r['linearraw_storage']['truthraw_scene_window_white'],'quant_err':r['linearraw_storage']['max_quantization_abs_scene_error'],'source_clip':r['source']['source_white_censored_count'],'checks':r['validation']['checks']},indent=2,default=str))