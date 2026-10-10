#!/usr/bin/env python3
from pathlib import Path
import subprocess, tempfile

root = Path(__file__).resolve().parents[1]
h2 = root / 'suite_android/app/src/main/cpp/presentation_deep_censor_chroma_guard_v0_2.h'
h1 = root / 'suite_android/app/src/main/cpp/presentation_deep_censor_chroma_guard_v0_1.h'
bridge = root / 'suite_android/app/src/main/cpp/photo_export_bridge.cpp'
assert h2.exists() and h1.exists() and bridge.exists()

txt = h2.read_text().lower()
# Reject executable/spatial-detail mechanisms, not explanatory comments that state
# that such mechanisms are forbidden.
for token in ['sobel(', 'laplacian(', 'sharpen(', 'blur(', 'object_detector(', 'hue_detector(']:
    assert token not in txt, token
assert 'kmaxdetailrelief = 0.35f' in txt
assert 'kdetailreliefchromaratioend = 0.12f' in txt

b = bridge.read_text()
assert b.count('presentation_deep_censor_chroma_guard_v0_2.h') == 1
assert 'presentation_deep_censor_chroma_guard_v0_1.h' not in b
assert b.count('presentation_deep_censor_chroma::apply(') == 1
assert b.find('presentation_near_censor_chroma::apply(') < b.find('presentation_censored_chroma::apply(') < b.find('presentation_deep_censor_chroma::apply(') < b.find('presentation_illuminant_warmth::apply(')

cpp = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include "presentation_deep_censor_chroma_guard_v0_1.h"
#include "presentation_deep_censor_chroma_guard_v0_2.h"
namespace v1 = truthraw::presentation_deep_censor_chroma_guard::v0_1;
namespace v2 = truthraw::presentation_deep_censor_chroma_guard::v0_2;
static double y(float r,float g,float b){return .2126*r+.7152*g+.0722*b;}
static double c(float r,float g,float b){double yy=y(r,g,b); double a=r-yy,d=g-yy,e=b-yy; return std::sqrt(a*a+d*d+e*e);}
static void apply1(float r,float g,float b,float f,float& R,float& G,float& B){R=r;G=g;B=b;assert(v1::apply(R,G,B,f));}
static void apply2(float r,float g,float b,float f,float& R,float& G,float& B){R=r;G=g;B=b;assert(v2::apply(R,G,B,f));}
int main(){
  for(float f: {0.0f,0.25f,0.50f}){float R,G,B;apply2(.52f,.50f,.48f,f,R,G,B);assert(R==.52f&&G==.50f&&B==.48f);}

  // Strong purple residual: exact v0.1 suppression.
  {float a,b,c1,A,B,C; apply1(.337f,.216f,.855f,.7213f,a,b,c1); apply2(.337f,.216f,.855f,.7213f,A,B,C);
   assert(std::fabs(a-A)<1e-7f&&std::fabs(b-B)<1e-7f&&std::fabs(c1-C)<1e-7f);}

  // Low-amplitude chroma microstructure: bounded relief, never expansion.
  {const float r=.52f,g=.50f,b=.48f,f=.80f; float a,d,e,A,D,E; apply1(r,g,b,f,a,d,e); apply2(r,g,b,f,A,D,E);
   const double cin=c(r,g,b), c1=c(a,d,e), c2=c(A,D,E);
   assert(c2>c1 && c2<cin); assert(std::fabs(y(r,g,b)-y(A,D,E))<2e-7);
   const double yy=y(r,g,b), yy2=y(A,D,E); double sR=(A-yy2)/(r-yy); double sB=(E-yy2)/(b-yy);
   assert(std::fabs(sR-sB)<2e-5);}

  // More CENSOR authority may never restore chroma for the same sample.
  {double prev=1e9; for(float f: {.50f,.60f,.70f,.80f,1.0f}){float R,G,B;apply2(.52f,.50f,.48f,f,R,G,B);double cc=c(R,G,B);assert(cc<=prev+1e-7);prev=cc;}}

  // Clearly chromatic non-purple sample also stays exact v0.1.
  {float a,b,c1,A,B,C;apply1(.60f,.50f,.45f,1.0f,a,b,c1);apply2(.60f,.50f,.45f,1.0f,A,B,C);assert(std::fabs(a-A)<1e-7f&&std::fabs(b-B)<1e-7f&&std::fabs(c1-C)<1e-7f);}
  std::puts("DEEP_CENSOR_CHROMA_GUARD_V02_REGRESSION_PASS");
}
'''
with tempfile.TemporaryDirectory() as td:
    src = Path(td)/'t.cpp'; exe=Path(td)/'t'; src.write_text(cpp)
    inc = root/'suite_android/app/src/main/cpp'
    subprocess.run(['g++','-std=c++17','-O2','-I',str(inc),str(src),'-o',str(exe)],check=True)
    out=subprocess.check_output([str(exe)],text=True).strip()
    assert out=='DEEP_CENSOR_CHROMA_GUARD_V02_REGRESSION_PASS', out
print('DEEP_CENSOR_CHROMA_GUARD_V02_REGRESSION_PASS')
