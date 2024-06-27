#include "stdafx.h"
#include <iostream>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <cstring>
#include <tuple>
using namespace std;

class Blowfish {
public:
    const int N = 16;
    std::vector<uint32_t> P;
    std::vector<std::vector<uint32_t>> S;

    uint32_t swap32(uint32_t i) {
        return ((i & 0xFF000000) >> 24) |
               ((i & 0x00FF0000) >> 8) |
               ((i & 0x0000FF00) << 8) |
               ((i & 0x000000FF) << 24);
    }

    int32_t to_clong(uint32_t v) {
        return static_cast<int32_t>(v);
    }

    uint32_t F(uint32_t x) {
        uint8_t a = (x >> 24) & 0x00FF;
        uint8_t b = (x >> 16) & 0x00FF;
        uint8_t c = (x >> 8) & 0x00FF;
        uint8_t d = x & 0x00FF;

        uint32_t y = (S[0][a] + S[1][b]) & 0xFFFFFFFF;
        y = y ^ S[2][c] & 0xFFFFFFFF;
        y = (y + S[3][d]) & 0xFFFFFFFF;
        return to_clong(y);
    }

    void blowfish_encipher(uint32_t& xl, uint32_t& xr) {
        for (int i = 0; i < N; i++) {
            xl = xl ^ P[i];
            xr = F(xl) ^ xr;
            std::swap(xl, xr);
        }

        std::swap(xl, xr);
        xr = xr ^ P[N];
        xl = xl ^ P[N + 1];
    }

    void blowfish_decipher(uint32_t& xl, uint32_t& xr) {
        for (int i = N + 1; i > 1; i--) {
            xl = xl ^ P[i];
            xr = F(xl) ^ xr;
            std::swap(xl, xr);
        }

        std::swap(xl, xr);
        xr = xr ^ P[1];
        xl = xl ^ P[0];
    }

    Blowfish() {
        P.reserve(N + 2);
        S.resize(4, std::vector<uint32_t>(256));

        for (int num = 0; num < 4; num++) {
            for (int sub_num = 0; sub_num < 256; sub_num++) {
                S[num][sub_num] = to_clong(swap32(PI_S_BOXES[num][sub_num]));
            }
        }
    }

    void initialize(const std::vector<uint8_t>& key) {
        int j = 0;
        for (int i = 0; i < N + 2; i++) {
            uint32_t data = 0x00000000;
            for (int k = 0; k < 4; k++) {
                data = data << 8;
                uint32_t x = key[j] & 0x000000FF;
                data = data | x;
                data = to_clong(data);
                j = j + 1;
                if (j >= key.size()) {
                    j = 0;
                }
                uint32_t p = swap32(PI_P_ARRAY[i]);
                uint32_t v = to_clong(p ^ data);
                P.push_back(v);
            }
        }

        uint32_t datal = 0x00000000;
        uint32_t datar = 0x00000000;

        for (int i = 0; i < N + 2; i += 2) {
            blowfish_encipher(datal, datar);
            P[i] = datal;
            P[i + 1] = datar;
        }

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 256; j += 2) {
                blowfish_encipher(datal, datar);
                S[i][j] = datal;
                S[i][j + 1] = datar;
            }
        }
    }
};

//int main() {
//    Blowfish blowfish;
//    std::vector<uint8_t> key = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
//    blowfish.initialize(key);
//    return 0;
//}
