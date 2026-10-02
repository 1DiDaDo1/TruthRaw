#include "truthraw_sha256_v0_69.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string>
#include <vector>

namespace sha = truthraw::sha256_v0_69;

namespace {

void require_active(bool ok,const char* expr,int line){
    if(!ok){
        std::cerr<<"REQUIRE_FAIL line="<<line<<" expr="<<expr<<"\n";
        std::exit(2);
    }
}
#define REQUIRE(expr) require_active(static_cast<bool>(expr),#expr,__LINE__)

sha::Digest digest_one_shot(std::span<const std::uint8_t> data){
    sha::Hasher h;
    h.update(data);
    return h.finalize();
}

sha::Digest digest_chunks(
    std::span<const std::uint8_t> data,
    std::span<const std::size_t> pattern,
    std::uint64_t* directBlocks=nullptr,
    std::uint64_t* bufferedBlocks=nullptr){
    sha::Hasher h;
    std::size_t offset=0u;
    std::size_t pi=0u;
    while(offset<data.size()){
        const std::size_t requested=
            pattern.empty()?data.size()-offset:pattern[pi%pattern.size()];
        REQUIRE(requested>0u);
        const std::size_t take=
            std::min(requested,data.size()-offset);
        h.update(data.data()+offset,take);
        offset+=take;
        ++pi;
    }
    if(directBlocks) *directBlocks=h.directInputBlockTransformCount();
    if(bufferedBlocks) *bufferedBlocks=h.bufferedInputBlockTransformCount();
    return h.finalize();
}

std::vector<std::uint8_t> bytes(const std::string& s){
    return std::vector<std::uint8_t>(s.begin(),s.end());
}

void test_known_vectors(){
    {
        sha::Hasher h;
        REQUIRE(
            sha::hex(h.finalize())==
            "e3b0c44298fc1c149afbf4c8996fb924"
            "27ae41e4649b934ca495991b7852b855");
    }
    {
        const auto v=bytes("abc");
        REQUIRE(
            sha::hex(digest_one_shot(v))==
            "ba7816bf8f01cfea414140de5dae2223"
            "b00361a396177a9cb410ff61f20015ad");
    }
    {
        const auto v=bytes(
            "abcdbcdecdefdefgefghfghighijhijk"
            "ijkljklmklmnlmnomnopnopq");
        REQUIRE(
            sha::hex(digest_one_shot(v))==
            "248d6a61d20638b8e5c026930c3e6039"
            "a33ce45964ff2167f6ecedd419db06c1");
    }
    {
        std::vector<std::uint8_t> v(1'000'000u,static_cast<std::uint8_t>('a'));
        sha::Hasher h;
        h.update(v);
        REQUIRE(h.directInputBlockTransformCount()>0u);
        REQUIRE(
            sha::hex(h.finalize())==
            "cdc76e5c9914fb9281a1c7e284d73e67"
            "f1809a48a497200e046d39ccc7112cd0");
    }
}

void test_chunk_boundary_parity(){
    std::vector<std::uint8_t> data(131071u);
    for(std::size_t i=0u;i<data.size();++i){
        data[i]=static_cast<std::uint8_t>(
            (i*131u + (i>>3u)*17u + 29u)&0xffu);
    }
    const auto expected=digest_one_shot(data);

    const std::array<std::size_t,1> one{{1u}};
    const std::array<std::size_t,1> seven{{7u}};
    const std::array<std::size_t,1> sixtyThree{{63u}};
    const std::array<std::size_t,1> sixtyFour{{64u}};
    const std::array<std::size_t,1> sixtyFive{{65u}};
    const std::array<std::size_t,1> authorityBatch{{2400u}};
    const std::array<std::size_t,8> mixed{{1u,63u,64u,65u,127u,1024u,2400u,17u}};

    REQUIRE(digest_chunks(data,one)==expected);
    REQUIRE(digest_chunks(data,seven)==expected);
    REQUIRE(digest_chunks(data,sixtyThree)==expected);
    REQUIRE(digest_chunks(data,sixtyFour)==expected);
    REQUIRE(digest_chunks(data,sixtyFive)==expected);
    REQUIRE(digest_chunks(data,authorityBatch)==expected);
    REQUIRE(digest_chunks(data,mixed)==expected);
}

void test_transport_counters(){
    std::vector<std::uint8_t> data(4096u);
    for(std::size_t i=0u;i<data.size();++i){
        data[i]=static_cast<std::uint8_t>((i*19u+7u)&0xffu);
    }

    {
        sha::Hasher h;
        h.update(data);
        REQUIRE(h.directInputBlockTransformCount()==64u);
        REQUIRE(h.bufferedInputBlockTransformCount()==0u);
        const auto d=h.finalize();
        REQUIRE(d==digest_one_shot(data));
    }

    {
        std::uint64_t direct=0u;
        std::uint64_t buffered=0u;
        const std::array<std::size_t,1> one{{1u}};
        const auto d=digest_chunks(data,one,&direct,&buffered);
        REQUIRE(d==digest_one_shot(data));
        REQUIRE(direct==0u);
        REQUIRE(buffered==64u);
    }

    {
        std::uint64_t direct=0u;
        std::uint64_t buffered=0u;
        const std::array<std::size_t,1> batch{{2400u}};
        const auto d=digest_chunks(data,batch,&direct,&buffered);
        REQUIRE(d==digest_one_shot(data));
        REQUIRE(direct>0u);
        REQUIRE(direct+buffered==64u);
    }
}

} // namespace

int main(){
    test_known_vectors();
    test_chunk_boundary_parity();
    test_transport_counters();
    std::cout<<"SHA-256 direct-block transport parity PASS\n";
    return 0;
}
