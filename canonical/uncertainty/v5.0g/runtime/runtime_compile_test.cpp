#include "uncertainty_runtime_v5_0g.h"
#include <iostream>
int main(){std::array<float,18> x{}; auto p=truthraw::predict_uncertainty_v5_0g(x,0,5.f); std::cout<<p.p50<<" "<<p.p95<<"\n"; return !(p.p95>=p.p50 && p.p50>0);}
