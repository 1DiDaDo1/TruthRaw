#include "output_acutance.h"
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>
using namespace truthraw_v47k;
int main(){int w=1280,h=964;std::size_t N=std::size_t(w)*h;std::vector<float>a(3*N),b(3*N);for(int y=0;y<h;++y)for(int x=0;x<w;++x){float v=0.05f+0.5f*float(x)/w+0.03f*std::sin(0.05f*x+0.03f*y);std::size_t i=std::size_t(y)*w+x;a[3*i]=1.05f*v;a[3*i+1]=v;a[3*i+2]=0.93f*v;}auto p=choose_output_acutance_plan(0.00135f,1.6f,OutputProfile::AdaptiveDetail);for(int i=0;i<2;++i)apply_output_acutance(a.data(),w,h,p,b.data());using C=std::chrono::steady_clock;auto t0=C::now();int it=8;for(int i=0;i<it;++i)apply_output_acutance(a.data(),w,h,p,b.data());auto t1=C::now();double ms=std::chrono::duration<double,std::milli>(t1-t0).count()/it;std::cout<<"width="<<w<<" height="<<h<<" ms="<<ms<<" mpix_s="<<(double(N)/1e6)/(ms/1000.0)<<"\n";return 0;}
