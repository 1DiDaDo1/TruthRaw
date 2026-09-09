#include "dng_stage2_v0_4.h"
#include <fstream>
#include <iostream>
#include <cstdint>
#include <vector>
#include <cmath>
int main(int argc,char**argv){
  if(argc!=3){std::cerr<<"usage: dump_stage2 input.dng out.bin\n";return 2;}
  try{
    auto d=truthraw_v04::readClassicDng(argv[1]);
    const std::size_t N=std::size_t(d.width)*d.height;
    std::vector<float> s(N),g(N);
    std::vector<std::uint8_t> c(N);
    static const int BGGR[4]={2,1,1,0};
    for(int y=0;y<d.height;++y)for(int x=0;x<d.width;++x){auto i=std::size_t(y)*d.width+x;s[i]=d.stage2At(y,x);g[i]=d.gainAt(y,x);c[i]=std::uint8_t(BGGR[d.phaseIndex(y,x)]);}
    std::ofstream o(argv[2],std::ios::binary); if(!o){return 3;}
    const std::uint32_t magic=0x54325338u,w=d.width,h=d.height;
    o.write((char*)&magic,4);o.write((char*)&w,4);o.write((char*)&h,4);
    o.write((char*)s.data(),std::streamsize(s.size()*4));
    o.write((char*)g.data(),std::streamsize(g.size()*4));
    o.write((char*)c.data(),std::streamsize(c.size()));
    o.write((char*)d.raw.data(),std::streamsize(d.raw.size()*2));
    std::cout<<"width="<<d.width<<" height="<<d.height<<" white="<<d.whiteLevel<<" black="<<d.blackPhase[0]<<","<<d.blackPhase[1]<<","<<d.blackPhase[2]<<","<<d.blackPhase[3]<<"\n";
    return 0;
  }catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 4;}
}
