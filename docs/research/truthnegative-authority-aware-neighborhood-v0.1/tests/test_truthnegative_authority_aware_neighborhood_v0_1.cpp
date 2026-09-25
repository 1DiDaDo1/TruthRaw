#include "truthnegative_authority_aware_neighborhood_v0_1.h"
#include <iostream>
#include <stdexcept>
namespace n=truthraw::truthnegative_authority_aware_neighborhood::v0_1;
#define R(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
int main(){n::Input i{};i.center=1;i.centerVariance=.01;i.centerVarianceKnown=true;i.neighbors={{1.02,.01,1,n::SampleAuthority::Measured,true,true,true,false},{.98,.01,1,n::SampleAuthority::Measured,true,true,true,false},{2,.01,1,n::SampleAuthority::Measured,true,true,true,false},{1,.01,1,n::SampleAuthority::Censored,true,true,true,true}};n::Result o{};R(n::estimate(i,o));R(o.valid);R(o.contributors==2);R(o.estimate>.98&&o.estimate<1.02);std::cout<<"TruthNegativeAuthorityAwareNeighborhood/0.1 PASS\n";}
