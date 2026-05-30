#ifndef AUDIOCAPTHREAD_H
#define AUDIOCAPTHREAD_H

#include <QObject>
#include <QThread>
#include <QFile>
#include <QBuffer>
#include <QVector>
#include <fftw3.h>

extern "C" {
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
    #include <libpostproc/postprocess.h>
    #include <libavutil/imgutils.h>
    #include <libavformat/avformat.h>
    #include <libavformat/avio.h>
    #include <libavfilter/avfilter.h>
    #include <libavdevice/avdevice.h>
    #include <libavcodec/avcodec.h>
}
#include "qcustomplot.h"

class AudioCapThread : public QThread
{
    Q_OBJECT

public:
    AudioCapThread(QObject *parent = nullptr, const QString audio_name = nullptr);

    virtual ~AudioCapThread();

    enum run_type {
        RUN_TYPE_NONE = 0x00,
        RUN_TYPE_QUIT,
        RUN_TYPE_EVT,
    };

    enum audio_para {
        FPS = 60,
        AUDIBLE_FREQ_RANGE_STR = 20,
        AUDIBLE_FREQ_RANGE_END = 20000, /* very optimistic */
        SAMPLE_FREQ = 48000,
        NUM_SAMPLES = 960*25, //xs buff
        NUM_FFT_SAMPLES = NUM_SAMPLES,
        FFT_DRV = 2,
        NUM_FFT_SAMPLES_AVG = 128,
    };

    enum plot_index {
        PLOT_INDICES = 0,        // 时域索引
        PLOT_SAMPLES = 1,           // 时域采样数据
        PLOT_FFT_INDICES = 2,      // FFT频域索引
        PLOT_FFT_SAMPLES = 3,      // FFT频域数据
        PLOT_FFT_INDICES_AVG = 4,  // FFT平均值索引
        PLOT_FFT_SAMPLES_AVG = 5,  // FFT平均值数据
        PLOT_MAX = 6,           // 最大索引
    };

    void begin(run_type type);
    void change_run(run_type type);
    run_type get_type(void);
    void abort(void);
    void keep(void);
    void end(void);
    void add_plot(QCustomPlot *plot);
    void add_bar(QCPBars *bar);

    int dir_idx = 0;
    double dir[NUM_FFT_SAMPLES_AVG];


    bool setVolume(float volume);
    float getVolume();
    float getVolumeRealtime(const AVFrame* frame);

private:
    run_type run_ty = RUN_TYPE_NONE;
    bool is_abort = false;

    const QString m_audio_name = nullptr;
    int m_audioIdx = -1;
    AVFormatContext *m_pAudioFmtCtx = nullptr;
    const AVInputFormat *m_pAudioInputFormat = nullptr;
    AVDictionary *m_pAudioOption = nullptr;

    AVCodecParameters *m_pCodecParamter = nullptr;
    const AVCodec *m_pCodec = nullptr;
    AVCodecContext *m_pCodecContext = nullptr;

    AVFrame *m_pFrame = nullptr;
    AVPacket *m_pPacket= nullptr;

    QBuffer  m_inputBuffer;

    QVector<QVector<double>> m_plot;

    double m_currentVolume = 1.0f;

    /*  */
    fftw_plan m_fftPlan;
    double *m_pFftIn = nullptr;
    double *m_pFftOut = nullptr;
    fftw_complex *m_ffti = nullptr;
    fftw_complex *m_ffto = nullptr;

    QVector<QCPBars *> m_bar;
    QVector<QCustomPlot *> m_pPlot;


    const QString get_audio_name(void);
    void fft_init();
    bool open_capture(const char* audio_name);
    void close_capture(void);
    bool captrue_output(QFile &file);
    bool captrue_output_1(QFile &file);

    bool decode_frame(QVector<double> &out);
    void fft_calc(QVector<double> &in, QVector<double> &out);
    void weighted_avg(QVector<double> &in, QVector<double> &out);
private slots:
    void on_start(run_type type);
    void on_quit(void);

protected:
    void run();

signals:
    void sg_start(run_type type);
    void sg_quit(void);
    void sg_captrue_refresh(QVector<QVector<double>> xy);
};

#endif // AUDIOCAPTHREAD_H
