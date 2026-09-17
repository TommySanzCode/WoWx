#include "auth/crypto.hpp"
extern "C" {
#include <tomcrypt.h>
}
namespace wowee::auth {
static std::vector<uint8_t> digest(const void* input,size_t size){
    hash_state state{};std::vector<uint8_t> result(20);
    if(sha1_init(&state)!=CRYPT_OK||sha1_process(&state,(const unsigned char*)input,(unsigned long)size)!=CRYPT_OK||sha1_done(&state,result.data())!=CRYPT_OK)return {};
    return result;
}
std::vector<uint8_t> Crypto::sha1(const std::vector<uint8_t>& data){return digest(data.data(),data.size());}
std::vector<uint8_t> Crypto::sha1(const std::string& data){return digest(data.data(),data.size());}
}
