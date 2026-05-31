#include "AudioFft.h"

#ifdef USE_FFTW3
// ==================== FFTW3 实现 ====================
#include <fftw3.h>
// 不直接 include fftw3.h，因为其中硬编码 #define FFTW_DLL
// 导致 __declspec(dllimport) 与 MinGW .a 导入库符号不匹配
// 自行声明所需的 fftwf 函数，避免 dllimport 修饰

// // fftwf 复数类型
// typedef float fftwf_complex_t[2];
// // fftwf 计划类型（不透明指针）
// typedef struct fftwf_plan_s *fftwf_plan;
// // FFTW 标志
// #define FFTW_ESTIMATE 64

// extern "C" {
// float *fftwf_alloc_real(int n);
// fftwf_complex_t *fftwf_alloc_complex(int n);
// void fftwf_free(void *p);
// fftwf_plan fftwf_plan_dft_r2c_1d(int n, float *in, fftwf_complex_t *out, unsigned flags);
// void fftwf_execute(fftwf_plan p);
// void fftwf_destroy_plan(fftwf_plan p);
// }

void AudioFft::transform(std::vector<float> &real, std::vector<float> &imag)
{
    uint32_t n = static_cast<uint32_t>(real.size());

    // 分配 FFTW 输入输出缓冲区
    float *in = fftwf_alloc_real(n);
    fftwf_complex *out = fftwf_alloc_complex(n / 2 + 1);

    // 填充输入
    for (uint32_t i = 0; i < n; i++) {
        in[i] = real[i];
    }

    // 创建并执行 FFTW 计划（单精度 r2c）
    fftwf_plan plan = fftwf_plan_dft_r2c_1d(
        static_cast<int>(n), in, out, FFTW_ESTIMATE);
    fftwf_execute(plan);

    // 提取结果：r2c 输出 n/2+1 个复数，对应 0~奈奎斯特频率
    // 填充前 n/2+1 个 bin
    for (uint32_t i = 0; i <= n / 2; i++) {
        real[i] = out[i][0];
        imag[i] = out[i][1];
    }
    // 利用共轭对称性填充后半部分：X[k] = conj(X[N-k])
    for (uint32_t i = 1; i < n / 2; i++) {
        real[n - i] = out[i][0];
        imag[n - i] = -out[i][1];
    }

    // 释放资源
    fftwf_destroy_plan(plan);
    fftwf_free(in);
    fftwf_free(out);
}

#else
// ==================== 自实现基2 Cooley-Tukey ====================
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

#endif // USE_FFTW3
