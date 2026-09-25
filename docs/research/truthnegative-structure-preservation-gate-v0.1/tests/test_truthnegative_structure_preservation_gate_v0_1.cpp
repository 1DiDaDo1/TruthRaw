#include "truthnegative_structure_preservation_gate_v0_1.h"
#include <iostream>
#include <stdexcept>
namespace g=truthraw::truthnegative_structure_preservation_gate::v0_1;
#define R(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
int main(){g::Result o{};g::Input flat{1,1.01,.99,1.01,.99,.05,true,false,false,true,1,1};R(g::evaluate(flat,o));R(o.decision==g::Decision::EligibleNoiseResidual);R(o.maxSuppressionFraction<=.75);
 g::Input edge=flat;edge.left=.2;edge.right=1.8;R(g::evaluate(edge,o));R(o.decision==g::Decision::Preserve);
 g::Input clip=flat;clip.censored=true;R(g::evaluate(clip,o));R(o.decision==g::Decision::Preserve);
 g::Input unknown=flat;unknown.sigmaKnown=false;R(g::evaluate(unknown,o));R(o.decision==g::Decision::Preserve);
 std::cout<<"TruthNegativeStructurePreservationGate/0.1 PASS\n";}
