#include "audio_cap_thread.h"

#include <QDebug>


//#include <QIODevice>
#include <QAudioFormat>
#include <QAudioInput>
#include <QElapsedTimer>





AudioCapThread::AudioCapThread(QObject *parent, const QString audio_name)
    : QThread(parent)
    , m_audio_name(audio_name)
{
    connect(this, &AudioCapThread::sg_quit, this, &AudioCapThread::on_quit);
    connect(this, &AudioCapThread::sg_start, this, &AudioCapThread::on_start);

    for (uint16_t i = 0; i < NUM_FFT_SAMPLES_AVG; i++) {
        dir[i] = 1;
    }

    qDebug() << "Hello FFMPEG, av_version_info is" << av_version_info();
    qDebug() << "avutil_configuration is" << avutil_configuration();


    foreach(const QAudioDeviceInfo &deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
    {
         qDebug() << "Device name: " << deviceInfo.deviceName();
    }

}
AudioCapThread::~AudioCapThread(void)
{
    fftw_free(m_pFftIn);
    fftw_free(m_pFftOut);
    fftw_destroy_plan(m_fftPlan);
    end();
}


const QString AudioCapThread::get_audio_name(void)
{
    QString url = "audio=";
    if (m_audio_name != nullptr)
    {
        url.append(m_audio_name);
    }
    else
    {
        url.append("virtual-audio-capturer");
    //    url.append("麦克风阵列 (Realtek(R) Audio)");
    //    url.append("扬声器 (Realtek(R) Audio)");
        // url.append("立体声混音 (Realtek(R) Audio)");
    //    url.append("VG259QM (2- NVIDIA High Definition Audio)");
    }
    return url;
}

void AudioCapThread::close_capture(void)
{
    avformat_close_input(&m_pAudioFmtCtx);
    avcodec_free_context(&m_pCodecContext);
    avformat_free_context(m_pAudioFmtCtx);
    m_pCodecContext = nullptr;
    m_pAudioFmtCtx  = nullptr;
    
    if (m_pAudioOption) {
        av_dict_free(&m_pAudioOption);
        m_pAudioOption = nullptr;
    }
}

/**
* @brief open audio device
* @return succ: AVFormatContext*, fail: NULL
*/
bool AudioCapThread::open_capture(const char* audio_name)
{
    int ret = -1;
    char errors[1024]   = { 0x00 };
    //    av_register_all();
    avdevice_register_all();
    
    // 初始化选项字典
    m_pAudioOption = nullptr;
//    av_dict_set(&m_pAudioOption, "sample_rate", "48000", 0);  // 设置采样率
//    av_dict_set(&m_pAudioOption, "sample_size", "16", 0);     // 设置采样大小
//    av_dict_set(&m_pAudioOption, "channels", "2", 0);         // 设置声道数
//    av_dict_set(&m_pAudioOption, "volume", "1.0", 0);         // 设置初始音量为1.0 (0.0-1.0)

    m_pAudioFmtCtx = avformat_alloc_context();
    m_pAudioInputFormat = av_find_input_format("dshow"); // windows input format

    // 使用设置好的选项打开输入设备
    ret = avformat_open_input(&m_pAudioFmtCtx, audio_name, m_pAudioInputFormat, &m_pAudioOption);
    if (ret != 0) {
        av_strerror(ret, errors, 1024);
        qDebug() << "Couldn't open input stream. (打开音频流设备失败)" << ret << errors;
        avformat_close_input(&m_pAudioFmtCtx);
        avformat_free_context(m_pAudioFmtCtx);
        m_pAudioFmtCtx = nullptr;
        return false;
    }

    // 添加音量控制回调函数
    m_pAudioFmtCtx->interrupt_callback.callback = [](void* ctx) -> int {
        return 0; // 返回0表示继续，1表示中断
    };
    m_pAudioFmtCtx->interrupt_callback.opaque = this;

    for (uint i = 0; i < m_pAudioFmtCtx->nb_streams; i++) {
        if (m_pAudioFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            m_audioIdx = i;
            break;
        }
    }
    if (m_audioIdx == -1) {
        qDebug() << "Couldn't find video stream information. (无法获取音频流信息)";
        return false;
    }

    m_pCodecParamter = m_pAudioFmtCtx->streams[m_audioIdx]->codecpar;
    m_pCodec         = avcodec_find_decoder(m_pCodecParamter->codec_id);
    m_pCodecContext  = avcodec_alloc_context3(m_pCodec);

     // 设置音频参数
     m_pCodecContext->sample_rate = 48000;            // 采样率
     m_pCodecContext->ch_layout.nb_channels = 2;      // 声道数
     m_pCodecContext->sample_fmt = AV_SAMPLE_FMT_S16; // 采样格式

    /* 填充上下文 */
    avcodec_parameters_to_context(m_pCodecContext, m_pCodecParamter);

    ret = avcodec_open2(m_pCodecContext, m_pCodec, nullptr);
    if (ret != 0) {
        qDebug() << "can not find or open audio decoder! (打开音频解码器失败)";
        avcodec_free_context(&m_pCodecContext);
        m_pCodecContext = nullptr;
        return false;
    }

    m_pFrame = av_frame_alloc();
    m_pPacket= av_packet_alloc();

    m_inputBuffer.open(QBuffer::ReadWrite);

    qDebug()<<"audio info:"
            << m_pCodecParamter->format
            << m_pCodecParamter->bit_rate
            << m_pCodecParamter->sample_rate
            << m_pCodecParamter->ch_layout.nb_channels;


    // m_plot 向量，预分配6个向量用于存储不同的数据
    while (m_plot.size() < PLOT_MAX) {
        m_plot.append(QVector<double>());
    }

    // 初始化采样缓冲区
    m_plot[PLOT_INDICES].resize(0);
    m_plot[PLOT_SAMPLES].resize(0);
    for (uint32_t i = 0; i < NUM_SAMPLES; i++) {
        m_plot[PLOT_INDICES].append(i);    // 时域索引
        m_plot[PLOT_SAMPLES].append(0);    // 时域采样数据
    }

    return true;
}
bool AudioCapThread::decode_frame(QVector<double> &out)
{
    int pcm_len = 0;
    uint32_t sample_len = 0;
    bool succeed = false;
    
    if (av_read_frame(m_pAudioFmtCtx, m_pPacket) >= 0) {
        if (m_pPacket->stream_index == m_audioIdx) {
            if (avcodec_send_packet(m_pCodecContext, m_pPacket) == 0) {
                if (avcodec_receive_frame(m_pCodecContext, m_pFrame) == 0) {
                    succeed = true;
                    pcm_len = m_pPacket->size; //av_samples_get_buffer_size(nullptr, m_pFrame->ch_layout.nb_channels, m_pFrame->nb_samples, (enum AVSampleFormat)m_pFrame->format, 1);
//                    sample_len = pcm_len / sizeof(int16_t);
                    av_packet_unref(m_pPacket);
                }
            }
        }
        av_packet_unref(m_pPacket);
    }
    
    if (succeed == false) {
        return false;
    }

    /* fill */
    sample_len = m_pFrame->nb_samples * m_pFrame->ch_layout.nb_channels;
    switch (m_pFrame->format) {
        case AV_SAMPLE_FMT_S16:
        case AV_SAMPLE_FMT_S16P: {
            const int16_t *data = (const int16_t *)m_pFrame->data[0];
            for (uint16_t i = 0; i < sample_len; i++) {
                out.append((double)data[i] / (double)INT16_MAX);
            }
        }
        break;

        case AV_SAMPLE_FMT_S32:
        case AV_SAMPLE_FMT_S32P: {
            const int32_t *data = (const int32_t *)m_pFrame->data[0];
            for (uint32_t i = 0; i < sample_len; i++) {
                out.append((double)data[i] / (double)INT32_MAX);
            }
        }
        break;
    }

    sample_len = out.length();
    if (sample_len > NUM_SAMPLES) {
        out = out.mid(sample_len - NUM_SAMPLES, -1);
    }

//    m_currentVolume = getVolume();
//    qDebug()<< "Current volume:" << m_currentVolume;

    static uint8_t cnt = 0;
    if (cnt == 1)
    {
        cnt = 6;
        qDebug()<< "current volume:" << m_currentVolume
                << "sample_rate:" << m_pFrame->sample_rate
                << "samples number:" << m_pFrame->nb_samples
                << "channels:" << m_pFrame->ch_layout.nb_channels
                << "sample_fmt:" << m_pFrame->format
                << "sample_fmt:" << m_pCodecContext->sample_fmt
                << "m_pPacket size:" << pcm_len
                << "indices:" << m_plot[PLOT_INDICES].length()
                << "samples:" << m_plot[PLOT_SAMPLES].length()
                << "fftIndices:" << m_plot[PLOT_FFT_INDICES].length()
                << "fftSamples:" << m_plot[PLOT_FFT_SAMPLES].length()
                << "fftSamplesAvg:" << m_plot[PLOT_FFT_SAMPLES_AVG].length();
    }
    else if (cnt == 0)
    {
        cnt++;
    }

    return true;
}
void AudioCapThread::fft_init(void)
{
    /* Set up FFT plan */
#if 0
    if (m_pFftIn == nullptr || m_pFftOut == nullptr) {
        m_pFftIn  = fftw_alloc_real(sizeof( double) * NUM_FFT_SAMPLES);
        m_pFftOut = fftw_alloc_real(sizeof( double) * NUM_FFT_SAMPLES);
        m_fftPlan = fftw_plan_r2r_1d(NUM_FFT_SAMPLES, m_pFftIn, m_pFftOut, FFTW_R2HC, FFTW_ESTIMATE);
    }
#else
    if (m_pFftIn == nullptr || m_ffto == nullptr) {
        m_pFftIn  = (double *)fftw_malloc(sizeof( double) * NUM_FFT_SAMPLES);
        m_ffto    = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * (NUM_FFT_SAMPLES));
        m_fftPlan = fftw_plan_dft_r2c_1d(NUM_FFT_SAMPLES, m_pFftIn, m_ffto, FFTW_ESTIMATE);
    }
#endif

    double freq_step = (double)SAMPLE_FREQ / (double)NUM_SAMPLES;
    uint32_t str = AUDIBLE_FREQ_RANGE_STR / freq_step;
    uint32_t end = AUDIBLE_FREQ_RANGE_END / freq_step;

    // 初始化 FFT 数据向量
    m_plot[PLOT_FFT_INDICES].resize(0);
    m_plot[PLOT_FFT_SAMPLES].resize(0);
    for (uint32_t i = str; i < end; i++) {
        m_plot[PLOT_FFT_INDICES].append(i * freq_step); // FFT索引
        m_plot[PLOT_FFT_SAMPLES].append(0); // FFT数据
    }

    // 初始化 FFT 平均值数据向量
    m_plot[PLOT_FFT_INDICES_AVG].resize(0);
    m_plot[PLOT_FFT_SAMPLES_AVG].resize(0);
    for (int i = 0; i < NUM_FFT_SAMPLES_AVG; i++) {
        m_plot[PLOT_FFT_INDICES_AVG].append(i);// FFT平均值索引
        m_plot[PLOT_FFT_SAMPLES_AVG].append(0); // FFT平均值数据
    }
}
void AudioCapThread::fft_calc(QVector<double> &in, QVector<double> &out)
{
//    out.clear();  // 清空之前的数据
//    memcpy(m_pFftIn, in.data(), in.length() * sizeof(double));

    // 应用汉宁窗并填充FFT输入
    for (int i = 0; i < in.length(); i++) {
        double multiplier = 0.5 * (1 - cos(2 * M_PI * i / (in.length() - 1)));
        m_pFftIn[i] = in[i] * multiplier;
    }

    fftw_execute(m_fftPlan);
    
    double freq_step = (double)SAMPLE_FREQ / (double)NUM_SAMPLES;
    uint32_t str = AUDIBLE_FREQ_RANGE_STR / freq_step;
    uint32_t end = AUDIBLE_FREQ_RANGE_END / freq_step;
    uint32_t len = end - str;
    for (uint32_t i = str; i < end; i++) {
        double sample = sqrt(m_ffto[i][0] * m_ffto[i][0] + m_ffto[i][1] * m_ffto[i][1]); //幅度谱
        sample *= (qLn(i) * qLn(i)); // Apply logarithmic scaling
        sample /= static_cast<double>(len);

        // double sample = atan2(m_ffto[i][1], m_ffto[i][0]); //相位谱

        // double db = log(sample);
        // qDebug("db = %f", db);

        out.append(sample);
    }

    uint32_t n = out.length();
    if (n > len) {
        out = out.mid(n - len, -1);
    }
}
void AudioCapThread::weighted_avg(QVector<double> &in, QVector<double> &out)
{
    int points_per_band = in.length() / NUM_FFT_SAMPLES_AVG;

//    int pos = NUM_FFT_SAMPLES_AVG / 2;
//    int pos_l = pos;
//    int pos_r = pos;
//    int dir_l = true;

    double max = 10;
    for (int i = 0; i < NUM_FFT_SAMPLES_AVG; i++) {
        double sum = 0.0;
        for (int j = 0; j < points_per_band; j++) {
            int index = i * points_per_band + j;
            sum += in.at(index);
        }
        sum /= points_per_band;

        uint32_t idx = i; //abs(i-(NUM_FFT_SAMPLES_AVG-1));
        sum *= dir[idx];

        if (i < 20)                         sum *= 0.5f;
        else if (i < 40)                    sum *= 0.8f;
        else if ((i >= 70) && (i < 90))     sum *= 1.2f;
        else if (i < NUM_FFT_SAMPLES_AVG)   sum *= 1.5f;
        // if (sum > max) sum = max;

        out.replace(idx, sum); // 计算算术平均值

//        if (i == 0 || i == num_bands-1) {
//            sum /= 3;
//        }
//        out.replace(pos, sum); // 计算算术平均值
//        if (dir_l) {
//            dir_l = false;
//            pos_l -= 1;
//            pos = pos_l;
//        }
//        else {
//            dir_l = true;
//            pos_r += 1;
//            pos = pos_r;
//        }
    }
}

void AudioCapThread::run()
{
//    QFile file("output1.pcm");
//    if(!file.open(QFile::WriteOnly)){
//        qDebug()<<"can`t open file";
////            return -1;
//    }

    const char *name = QByteArray(get_audio_name().toUtf8()).data();
    if (open_capture(name) == false) {
        return;
    }
    qDebug()<<"open_capture ok";

    fft_init();
    
    // 添加计时器来控制刷新率
    QElapsedTimer refreshTimer;
    refreshTimer.start();
    const int minRefreshInterval = 1000 / FPS;

    for (;;) {
        if (is_abort) {
            continue;
        }
        switch (run_ty) {
            case RUN_TYPE_QUIT:
                goto QUIT;
                break;

            case RUN_TYPE_NONE:
                break;

            case RUN_TYPE_EVT:
                if (decode_frame(m_plot[PLOT_SAMPLES])) {
                    fft_calc(m_plot[PLOT_SAMPLES], m_plot[PLOT_FFT_SAMPLES]);
                    weighted_avg(m_plot[PLOT_FFT_SAMPLES], m_plot[PLOT_FFT_SAMPLES_AVG]);
                    
                    // 限制刷新率
                    if (refreshTimer.elapsed() >= minRefreshInterval) {
                        m_pPlot[0]->graph(0)->setData(m_plot[PLOT_INDICES], m_plot[PLOT_SAMPLES]);
                        m_pPlot[0]->xAxis->rescale();

                        m_pPlot[1]->graph(0)->setData(m_plot[PLOT_FFT_INDICES], m_plot[PLOT_FFT_SAMPLES]);
                        m_pPlot[1]->xAxis->rescale();

                        m_bar[0]->setData(m_plot[PLOT_FFT_INDICES_AVG], m_plot[PLOT_FFT_SAMPLES_AVG]);
                        m_pPlot[2]->xAxis->rescale();

                        emit sg_captrue_refresh(m_plot);
                        refreshTimer.restart();
                    }
                }
                break;
        }
    }
QUIT:
    av_frame_free(&m_pFrame);
    av_packet_free(&m_pPacket);
    close_capture();
//    file.close();
    qDebug("thread end");
}

void AudioCapThread::add_bar(QCPBars *bar)
{
    m_bar.append(bar);
}
void AudioCapThread::add_plot(QCustomPlot *plot)
{
    m_pPlot.append(plot);
}
void AudioCapThread::end(void)
{
    emit sg_quit();
}
void AudioCapThread::begin(run_type type)
{
    emit sg_start(type);
}
void AudioCapThread::change_run(run_type type)
{
    run_ty = type;
}

AudioCapThread::run_type AudioCapThread::get_type()
{
    return run_ty;
}
void AudioCapThread::abort(void)
{
    is_abort = true;
}
void AudioCapThread::keep(void)
{
    is_abort = false;
}

void AudioCapThread::on_start(run_type type) {
    run_ty = type;
    if (isRunning() != true) {
        start();
    }
}
void AudioCapThread::on_quit(void) {
    run_ty = RUN_TYPE_QUIT;
    if (isRunning() == true) {
        requestInterruption();
        quit();
        wait();
    }
}

float AudioCapThread::getVolumeRealtime(const AVFrame* frame)
{
    if (!frame || !frame->data[0]) {
        return 0.0;
    }

    float sum = 0.0;
    int samples = frame->nb_samples * frame->ch_layout.nb_channels;
    
    // 根据采样格式计算音量
    switch (frame->format) {
        case AV_SAMPLE_FMT_S16:
        case AV_SAMPLE_FMT_S16P: {
            const int16_t* data = (const int16_t*)frame->data[0];
            for (int i = 0; i < samples; i++) {
                sum += abs(data[i]) / (double)INT16_MAX;
            }
            break;
        }
        case AV_SAMPLE_FMT_FLT:
        case AV_SAMPLE_FMT_FLTP: {
            const float* data = (const float*)frame->data[0];
            for (int i = 0; i < samples; i++) {
                sum += fabs(data[i]);
            }
            break;
        }
        // 可以添加其他采样格式的支持
        default:
            return 0.0;
    }

    // 计算平均音量
    return sum / samples;
}


// 添加设置音量的方法
bool AudioCapThread::setVolume(float volume) {
    if (!m_pAudioFmtCtx) return false;
    
    // 确保音量在0.0-1.0范围内
    volume = std::max(0.0f, std::min(1.0f, volume));
    
    char vol_str[32];
    snprintf(vol_str, sizeof(vol_str), "%.2f", volume);
    
    // 设置新的音量
    int ret = av_dict_set(&m_pAudioOption, "volume", vol_str, 0);
    if (ret < 0) {
        char errors[1024] = { 0x00 };
        av_strerror(ret, errors, 1024);
        qDebug() << "Failed to set volume:" << errors;
        return false;
    }
    
    // 更新当前音量值
    m_currentVolume = volume;
    return true;
}

// 添加获取音量的方法
float AudioCapThread::getVolume() {
    if (!m_pAudioFmtCtx) return 0.0f;
    
    char* volume_str = nullptr;
    if (av_opt_get(m_pAudioFmtCtx, "volume", AV_OPT_SEARCH_CHILDREN, (uint8_t**)&volume_str) >= 0) {
        if (volume_str) {
            float volume = atof(volume_str);
            av_free(volume_str);
            return volume;
        }
    }
    
    // 获取音频设备属性
//        AVDictionaryEntry* volume = av_dict_get(m_pAudioOption, "volume", nullptr, 0);
//        if (volume) {
//            m_currentVolume = atof(volume->value) / 100.0;
//            qDebug() << "System volume level (from options):" << m_currentVolume;
//        }
    return m_currentVolume;
}
