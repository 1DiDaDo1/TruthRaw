#include "output_acutance.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <cstdlib>
using namespace truthraw_v47k;
int main(int argc,char**argv){
 if(argc!=8){std::cerr<<"usage: in.bin w h noise resize profile out.bin\n";return 2;}
 std::string in=argv[1],outp=argv[7];int w=std::atoi(argv[2]),h=std::atoi(argv[3]);float n=std::strtof(argv[4],nullptr),rr=std::strtof(argv[5],nullptr);int pr=std::atoi(argv[6]);
 std::vector<float>a(std::size_t(w)*h*3),b(a.size());std::ifstream f(in,std::ios::binary);f.read(reinterpret_cast<char*>(a.data()),std::streamsize(a.size()*sizeof(float)));if(!f){return 3;}
 auto p=choose_output_acutance_plan(n,rr,static_cast<OutputProfile>(pr));if(!apply_output_acutance(a.data(),w,h,p,b.data()))return 4;std::ofstream o(outp,std::ios::binary);o.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size()*sizeof(float)));return o?0:5;
}
