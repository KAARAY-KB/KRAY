#ifndef AUDIO_FFT_H
#define AUDIO_FFT_H

#include <vector>

// FFT 变换：基2 Cooley-Tukey 算法
// 输入实部和虚部数组（长度必须为2的幂），原地计算
class AudioFft
{
public:
    // 执行 FFT 变换（原地）
    static void transform(std::vector<float> &real, std::vector<float> &imag);
};

#endif // AUDIO_FFT_H
