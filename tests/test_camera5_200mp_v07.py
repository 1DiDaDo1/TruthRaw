from pathlib import Path
import importlib.util, json

ROOT=Path('/mnt/data/truthraw_fullraw_lab')
SRC=ROOT/'android_probe/camera5-200mp-probe-v07/app/src/main/java/com/truthraw/fullsensorprobe/MainActivity.kt'
CAP=ROOT/'android_probe/HONOR_BKQ_N49_CAMERA2_CAPABILITY_EVIDENCE_v06.json'
GATE=ROOT/'bin/camera5_200mp_runtime_gate_v07.py'

def load(path,name):
    spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def test_device_capability_has_exact_200mp_highres_raw_sensor():
    d=json.loads(CAP.read_text())
    assert d['camera_id']=='5'
    assert d['focal_length_mm']==22.48
    assert d['pixel_array_size_maximum_resolution']==[16320,12288]
    assert any(x['format']=='RAW_SENSOR' and x['width']==16320 and x['height']==12288 for x in d['maximum_resolution_high_res_outputs'])

def test_probe_queries_high_resolution_output_sizes_and_exact_target():
    s=SRC.read_text()
    assert 'getHighResolutionOutputSizes' in s
    assert 'w != 16320 || h != 12288' in s
    assert 'SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION' in s
    assert 'addSensorPixelModeUsed' in s

def test_probe_has_no_full_frame_heap_copy():
    s=SRC.read_text()
    assert 'ByteArray(buf.remaining())' not in s
    assert 'ByteArray(sourceBuffer.remaining())' not in s
    assert 'MessageDigest.getInstance("SHA-256")' in s
    assert 'FileOutputStream(raw).channel' in s

def test_runtime_gate_never_closes_without_payload():
    g=load(GATE,'gatev07')
    m={
      'evidence_class':'DEVICE_RUNTIME_CAPTURE','device':{'make':'HONOR','model':'BKQ-N49'},
      'physical_camera_id':'5','opened_camera_id':'5','focal_mm':22.48,
      'source_identity_sha256':'abc','maximum_resolution_highres_raw_sizes':[[16320,12288]],
      'sensor_info_pixel_array_size_maximum_resolution':[16320,12288],
      'sensor_info_active_array_size_maximum_resolution':[0,0,16320,12288],
      'requested':{'sensor_pixel_mode':'MAXIMUM_RESOLUTION'},
      'capture_result':{'sensor_pixel_mode':'MAXIMUM_RESOLUTION','sensor_timestamp_ns':123,'sensor_raw_binning_factor_used':False},
      'raw_output':{'format':'RAW_SENSOR','width':16320,'height':12288,'row_stride':32640,'pixel_stride':2,
                    'payload_bytes':401080320,'payload_sha256':'abc','image_timestamp_ns':123,'timestamp_matches_capture_result':True},
      'camera_characteristics':{'lens_shading_applied_to_raw':True}
    }
    r=g.evaluate(m,None)
    assert not r['pass']
    assert r['checks']['payload_file_provided'] is False

def test_simulation_cannot_close_gate():
    g=load(GATE,'gatev07b')
    m={'evidence_class':'SIMULATION_ONLY','raw_output':{},'capture_result':{}}
    r=g.evaluate(m,None)
    assert r['classification']=='SIMULATION_ONLY'
    assert not r['pass']
