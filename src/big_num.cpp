#include "auth/big_num.hpp"
#include "wx_auth.h"
#include <algorithm>
#include <utility>
static bool healthy=true;
static void check(mp_err result){if(result!=MP_OKAY)healthy=false;}
extern "C" int wx_crypto_healthy(void){return healthy;}
namespace wowee::auth {
BigNum::BigNum(){check(mp_init(&value_));}
BigNum::BigNum(uint32_t n):BigNum(){if(healthy)mp_set_u32(&value_,n);}
BigNum::BigNum(const std::vector<uint8_t>& bytes,bool little):BigNum(){
    if(bytes.size()>256){healthy=false;return;}
    auto data=bytes;if(little)std::reverse(data.begin(),data.end());
    if(healthy)check(mp_from_ubin(&value_,data.data(),data.size()));
}
BigNum::~BigNum(){if(value_.dp)mp_clear(&value_);}
BigNum::BigNum(const BigNum& other):BigNum(){if(healthy)check(mp_copy(&other.value_,&value_));}
BigNum& BigNum::operator=(const BigNum& other){if(this!=&other&&healthy)check(mp_copy(&other.value_,&value_));return *this;}
BigNum::BigNum(BigNum&& other) noexcept {std::swap(value_,other.value_);}
BigNum& BigNum::operator=(BigNum&& other) noexcept{if(this!=&other)std::swap(value_,other.value_);return *this;}
BigNum BigNum::fromRandom(int count){
    if(count<1||count>64){healthy=false;return BigNum();}
    std::vector<uint8_t> data(count);if(!wx_random_bytes(data.data(),data.size()))healthy=false;
    BigNum value(data);std::fill(data.begin(),data.end(),0);return value;
}
BigNum BigNum::fromHex(const std::string& text){BigNum r;if(text.size()>512)healthy=false;if(healthy)check(mp_read_radix(&r.value_,text.c_str(),16));return r;}
BigNum BigNum::add(const BigNum& b) const{BigNum r;if(healthy)check(mp_add(&value_,&b.value_,&r.value_));return r;}
BigNum BigNum::subtract(const BigNum& b) const{BigNum r;if(healthy)check(mp_sub(&value_,&b.value_,&r.value_));return r;}
BigNum BigNum::multiply(const BigNum& b) const{BigNum r;if(healthy)check(mp_mul(&value_,&b.value_,&r.value_));return r;}
BigNum BigNum::mod(const BigNum& b) const{BigNum r;if(healthy)check(mp_mod(&value_,&b.value_,&r.value_));return r;}
BigNum BigNum::modPow(const BigNum& e,const BigNum& n) const{BigNum r;if(healthy)check(mp_exptmod(&value_,&e.value_,&n.value_,&r.value_));return r;}
bool BigNum::equals(const BigNum& b) const{return healthy&&mp_cmp(&value_,&b.value_)==MP_EQ;}
bool BigNum::isZero() const{return !healthy||mp_iszero(&value_);}
std::vector<uint8_t> BigNum::toArray(bool little,int minimum) const{
    if(!healthy||minimum<0||minimum>256)return {};
    size_t count=mp_ubin_size(&value_);if(count>256){healthy=false;return {};}
    std::vector<uint8_t> r(std::max(count,size_t(minimum)),0);
    if(count)check(mp_to_ubin(&value_,r.data()+r.size()-count,count,nullptr));
    if(little)std::reverse(r.begin(),r.end());return r;
}
std::string BigNum::toHex() const{char text[513]={};if(healthy)check(mp_to_radix(&value_,text,sizeof text,nullptr,16));return text;}
}
