#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/cpp/photo_export_bridge.cpp')
s = p.read_text()

inc1 = '#include "presentation_deep_censor_chroma_guard_v0_1.h"'
inc2 = '#include "presentation_deep_censor_chroma_guard_v0_2.h"'
ns1 = 'namespace presentation_deep_censor_chroma = truthraw::presentation_deep_censor_chroma_guard::v0_1;'
ns2 = 'namespace presentation_deep_censor_chroma = truthraw::presentation_deep_censor_chroma_guard::v0_2;'

changed = False
if inc2 not in s:
    if inc1 not in s:
        raise SystemExit('missing deep-censor v0.1/v0.2 include anchor')
    s = s.replace(inc1, inc2, 1)
    changed = True
if ns2 not in s:
    if ns1 not in s:
        raise SystemExit('missing deep-censor v0.1/v0.2 namespace anchor')
    s = s.replace(ns1, ns2, 1)
    changed = True

if s.count(inc2) != 1 or s.count(ns2) != 1:
    raise SystemExit('deep-censor v0.2 wiring must be unique')
if inc1 in s or ns1 in s:
    raise SystemExit('runtime must not retain v0.1 include/namespace after v0.2 wiring')
if s.count('presentation_deep_censor_chroma::apply(') != 1:
    raise SystemExit('deep-censor runtime call count must remain exactly one')

# Preserve the established downstream order. The runtime call alias is intentionally
# unchanged so only the implementation revision changes.
near = s.find('presentation_near_censor_chroma::apply(')
fallback = s.find('presentation_censored_chroma::apply(')
deep = s.find('presentation_deep_censor_chroma::apply(')
warm = s.find('presentation_illuminant_warmth::apply(')
if min(near, fallback, deep, warm) < 0 or not (near < fallback < deep < warm):
    raise SystemExit('invalid Appearance order: near -> fallback -> deep -> warm required')

p.write_text(s)
print('DEEP_CENSOR_CHROMA_GUARD_V02_WIRING_' + ('APPLIED' if changed else 'ALREADY_APPLIED'))
