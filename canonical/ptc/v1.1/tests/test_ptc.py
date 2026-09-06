from pathlib import Path
import json, sys, tempfile
from PIL import Image
import numpy as np
sys.path.insert(0,str(Path(__file__).parents[1]/"python"))
import truthraw_ptc as p

def valid_cert(scope="FULL_PHYSICAL"):
    c=p.base_certificate(scope)
    c["source"].update(filename="fixture.dng",sha256="1"*64,source_class="DIRECT_CFA",direct_sensor_cfa=True)
    c["admission"].update(cfa_validated=True,blacklevel_validated=True,whitelevel_validated=True,gainmap_present=True,gainmap_application_count=1,color_transform_bound=True,source_identity_bound=True)
    c["reconstruction"].update(backend="fixture",backend_version="1",backend_sha256="2"*64,backend_bound_to_output=True)
    c["scientific_master"]["sha256"]="3"*64
    c["export"].update(format="JPEG",media_payload_sha256="4"*64,ptc_metadata_embedded=True,copyright_embedded=True)
    c["noise_uncertainty"].update(noise_model_bound=True,backend_bound_uncertainty=True,uncertainty_status="backend-bound")
    c["physical_calibration"].update(per_lens_color_calibrated=True,illuminant_calibrated=True,electron_calibration_bound=True,optics_calibration_bound=True)
    return p.apply_evaluation(c)

def run():
    c=valid_cert(); assert c["status"]=="PURE_TRUTH_CERTIFIED",c["evaluation"]
    d=valid_cert(); d["physical_calibration"]["electron_calibration_bound"]=False; d=p.apply_evaluation(d); assert d["status"]=="PURE_TRUTH_DERIVED"
    n=valid_cert(); n["reconstruction"]["generated_scene_content"]=True; n=p.apply_evaluation(n); assert n["status"]=="NOT_CERTIFIED"
    with tempfile.TemporaryDirectory() as td:
        a=np.zeros((64,96,3),np.uint8); a[...,0]=np.arange(96,dtype=np.uint8)[None,:]; a[...,1]=120; a[...,2]=180
        src=Path(td)/"a.jpg";dst=Path(td)/"b.jpg";Image.fromarray(a).save(src,quality=91)
        h=p.jpeg_media_payload_sha256(src)
        c=valid_cert("CORE_INTEGRITY");c["export"]["media_payload_sha256"]=h;c=p.apply_evaluation(c)
        p.embed_jpeg_xmp(src,dst,c,"certificate.json")
        assert p.jpeg_media_payload_sha256(dst)==h
        x=p.extract_truthraw_xmp(dst); assert x and p.COPYRIGHT_NOTICE in x and c["certificate_json_sha256"] in x
        assert np.array_equal(np.array(Image.open(src)),np.array(Image.open(dst)))
    print("PTC Python self-test: PASS")
if __name__=="__main__":run()
