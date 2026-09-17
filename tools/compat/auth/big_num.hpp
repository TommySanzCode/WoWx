#pragma once
// Xbox-compatible backend for the pinned WoWee SRP class.
#include <vector>
#include <cstdint>
#include <string>
#include <tommath.h>
namespace wowee::auth {
class BigNum {
public:
    BigNum();
    explicit BigNum(uint32_t value);
    explicit BigNum(const std::vector<uint8_t>& bytes,bool littleEndian=true);
    ~BigNum();
    BigNum(const BigNum& other);
    BigNum& operator=(const BigNum& other);
    BigNum(BigNum&& other) noexcept;
    BigNum& operator=(BigNum&& other) noexcept;
    static BigNum fromRandom(int bytes);
    static BigNum fromHex(const std::string& hex);
    BigNum add(const BigNum& other) const;
    BigNum subtract(const BigNum& other) const;
    BigNum multiply(const BigNum& other) const;
    BigNum mod(const BigNum& modulus) const;
    BigNum modPow(const BigNum& exponent,const BigNum& modulus) const;
    bool equals(const BigNum& other) const;
    bool isZero() const;
    std::vector<uint8_t> toArray(bool littleEndian=true,int minSize=0) const;
    std::string toHex() const;
private:
    mp_int value_{};
};
}
