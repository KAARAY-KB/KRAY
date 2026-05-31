#ifndef AUDIO_FFT_H
#define AUDIO_FFT_H

#include <vector>
#include <cstdint>

// FFT 变换：编译期可选 FFTW3 或自实现基2 Cooley-Tukey
// 定义 USE_FFTW3 宏启用 FFTW3，否则使用自实现
// 输入实部和虚部数组（长度必须为2的幂），原地计算
class AudioFft
{
public:
    // 执行 FFT 变换（原地）
    static void transform(std::vector<float> &real, std::vector<float> &imag);
};

#endif // AUDIO_FFT_H
