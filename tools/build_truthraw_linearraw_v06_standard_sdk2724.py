from __future__ import annotations

from pathlib import Path

BASE = Path(__file__).with_name("build_truthraw_linearraw_v06_standard.py")
text = BASE.read_text(encoding="utf-8")

replacements = [
    (
        "'DNGVersion','DNGBackwardVersion','UniqueCameraModel','Make','Model','DefaultScale','DefaultCropOrigin','DefaultCropSize','ActiveArea',\n            'ColorMatrix1'",
        "'DNGVersion','DNGBackwardVersion','UniqueCameraModel','Make','Model',\n            'ColorMatrix1'",
    ),
    (
        "'candidate_status':'RESEARCH_CANDIDATE_REQUIRES_DNG_SDK_1_7_1_2724_VALIDATION'",
        "'candidate_status':'RESEARCH_CANDIDATE_REQUIRES_DNG_SDK_1_7_1_2724_REVALIDATION_AFTER_IFD_TAG_FIX'",
    ),
    (
        "raw_extra=[\n      (274,DT.SHORT,1,int(src_meta['Orientation']),False),(50713,DT.SHORT,2,(1,1),False),",
        "raw_extra=[\n      (50713,DT.SHORT,2,(1,1),False),",
    ),
    (
        "'raw_orientation':int(sub.tags['Orientation'].value),'raw_black_level'",
        "'raw_has_orientation':'Orientation' in sub.tags,'raw_black_level'",
    ),
]

for old, new in replacements:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"expected exactly one replacement match, got {count}: {old[:80]!r}")
    text = text.replace(old, new)

# Execute the corrected historical candidate in-process so command-line behavior and
# output hashes remain reproducible while preserving the failed pre-fix writer in Git.
code = compile(text, str(BASE) + "[sdk2724-ifd-fix]", "exec")
namespace = {"__name__": "__main__", "__file__": str(BASE)}
exec(code, namespace, namespace)
