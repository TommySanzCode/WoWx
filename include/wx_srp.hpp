#pragma once
#include "auth/crypto.hpp"
#include <algorithm>
#include <vector>
#include <string>

// The selected Vanilla server hashes BigNumber values at their natural
// little-endian width (vMaNGOS SRP6.cpp and WorldSocket.cpp). Keep padded wire
// fields separate: dropping an ending zero from a hash input is significant.
inline std::vector<uint8_t> wx_srp_natural(const uint8_t* bytes,size_t count){
    while(count&&!bytes[count-1])count--;
    return {bytes,bytes+count};
}
struct WxSrpProofs {std::vector<uint8_t> client,server;};
inline WxSrpProofs wx_srp_proofs(const uint8_t* modulus,const uint8_t* salt,
    const uint8_t* A,const uint8_t* B,const uint8_t* key,const char* username){
    using wowee::auth::Crypto;
    auto n=Crypto::sha1(wx_srp_natural(modulus,32));
    auto g=Crypto::sha1(std::vector<uint8_t>{7});
    if(n.size()!=20||g.size()!=20)return {};
    for(unsigned i=0;i<20;i++)n[i]^=g[i];
    std::string upper=username;for(char& c:upper)if(c>='a'&&c<='z')c-=32;
    auto user=Crypto::sha1(upper),a=wx_srp_natural(A,32),b=wx_srp_natural(B,32);
    auto s=wx_srp_natural(salt,32),k=wx_srp_natural(key,40);
    std::vector<uint8_t> input=wx_srp_natural(n.data(),n.size());
    for(const auto* part:{&user,&s,&a,&b,&k})input.insert(input.end(),part->begin(),part->end());
    WxSrpProofs result;result.client=Crypto::sha1(input);
    auto m=wx_srp_natural(result.client.data(),result.client.size());
    input=a;input.insert(input.end(),m.begin(),m.end());input.insert(input.end(),k.begin(),k.end());
    result.server=Crypto::sha1(input);return result;
}
