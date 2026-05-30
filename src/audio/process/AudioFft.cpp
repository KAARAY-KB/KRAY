#include "AudioFft.h"
#include <cmath>
#include <algorithm>
#include <cstdint>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 基2 Cooley-Tukey FFT
void AudioFft::transform(std::vector<float> &real, std::vector<float> &imag)
{
    uint32_t n = static_cast<uint32_t>(real.size());

    // 位反转排列
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        while (j & bit) {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;
        if (i < j) {
            std::swap(real[i], real[j]);
            std::swap(imag[i], imag[j]);
        }
    }

    // 蝶形运算
    for (uint32_t len = 2; len <= n; len <<= 1) {
        float angle = -2.0f * static_cast<float>(M_PI) / len;
        float w_real = cosf(angle);
        float w_imag = sinf(angle);

        for (uint32_t i = 0; i < n; i += len) {
            float cur_real = 1.0f;
            float cur_imag = 0.0f;

            for (uint32_t j = 0; j < len / 2; j++) {
                uint32_t even = i + j;
                uint32_t odd = i + j + len / 2;

                float t_real = cur_real * real[odd] - cur_imag * imag[odd];
                float t_imag = cur_real * imag[odd] + cur_imag * real[odd];

                real[odd] = real[even] - t_real;
                imag[odd] = imag[even] - t_imag;
                real[even] += t_real;
                imag[even] += t_imag;

                float new_real = cur_real * w_real - cur_imag * w_imag;
                float new_imag = cur_real * w_imag + cur_imag * w_real;
                cur_real = new_real;
                cur_imag = new_imag;
            }
        }
    }
}
