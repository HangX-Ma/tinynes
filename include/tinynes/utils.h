#ifndef TINYNES_UTILS_H
#define TINYNES_UTILS_H

#include <cstdint>
#include <string>

namespace tn
{

constexpr std::string_view HEX_NUM = "0123456789ABCDEF";
class Utils
{
public:
    static std::string numToHex(uint32_t num, int length)
    {
        std::string s(length, '0');
        for (int i = length - 1; i >= 0; i -= 1, num >>= 4) {
            s[i] = HEX_NUM[num & 0x0F];
        }
        return s;
    }
};

} // namespace tn

#endif