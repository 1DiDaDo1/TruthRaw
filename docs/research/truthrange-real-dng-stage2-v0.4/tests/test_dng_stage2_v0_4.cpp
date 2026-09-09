#include "dng_stage2_v0_4.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace truthraw_v04;
static void u32(std::vector<std::uint8_t>&b,std::uint32_t v){b.push_back(v>>24);b.push_back(v>>16);b.push_back(v>>8);b.push_back(v);}
static void i32(std::vector<std::uint8_t>&b,std::int32_t v){u32(b,static_cast<std::uint32_t>(v));}
static void f32(std::vector<std::uint8_t>&b,float x){std::uint32_t u;std::memcpy(&u,&x,4);u32(b,u);}
static void f64(std::vector<std::uint8_t>&b,double x){std::uint64_t u;std::memcpy(&u,&x,8);for(int i=7;i>=0;--i)b.push_back(std::uint8_t(u>>(8*i)));}
static std::vector<std::uint8_t> makeList(){
 std::vector<std::vector<std::uint8_t>> pay;
 const int pos[4][2]={{1,1},{0,1},{1,0},{0,0}};
 for(int k=0;k<4;++k){std::vector<std::uint8_t> p;i32(p,pos[k][0]);i32(p,pos[k][1]);i32(p,4);i32(p,4);u32(p,0);u32(p,1);u32(p,2);u32(p,2);u32(p,2);u32(p,2);f64(p,1.0);f64(p,1.0);f64(p,0.0);f64(p,0.0);u32(p,1);f32(p,1.0f+k);f32(p,2.0f+k);f32(p,3.0f+k);f32(p,4.0f+k);pay.push_back(p);}std::vector<std::uint8_t>b;u32(b,4);for(auto&p:pay){u32(b,9);u32(b,0x01030000);u32(b,1);u32(b,std::uint32_t(p.size()));b.insert(b.end(),p.begin(),p.end());}return b;}
int main(){
 auto bytes=makeList(); auto g=parseOpcodeList2(bytes); if(g.size()!=4) throw std::runtime_error("count");
 if(!g[0].applies(1,1)||g[0].applies(0,0))throw std::runtime_error("phase apply");
 float v=g[0].interpolate(1,1,4,4); if(!(v>1.0f&&v<4.0f))throw std::runtime_error("interp");
 ClassicDng d;d.width=4;d.height=4;d.blackPhase={10,20,30,40};d.whiteLevel=100;d.gainMaps=g;d.raw.assign(16,50);float s=d.stage2At(1,1);float expected=((50.0f-40.0f)/(100.0f-40.0f))*d.gainAt(1,1);if(std::abs(s-expected)>1e-7f)throw std::runtime_error("stage2");
 auto bad=bytes;bad[4+0]=0;bad[4+1]=0;bad[4+2]=0;bad[4+3]=8;bool rejected=false;try{(void)parseOpcodeList2(bad);}catch(...){rejected=true;}if(!rejected)throw std::runtime_error("non-gainmap not rejected");
 bad=bytes;bad.pop_back();rejected=false;try{(void)parseOpcodeList2(bad);}catch(...){rejected=true;}if(!rejected)throw std::runtime_error("truncation not rejected");
 std::cout<<"TruthRaw v0.4 native GainMap/Stage-2 contract: PASS\n";
 return 0;
}
