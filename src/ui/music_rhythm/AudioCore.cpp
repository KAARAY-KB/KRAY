#include "AudioCore.h"
#include <cmath>
#include <algorithm>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ==================== Windows: WASAPI PKEY 补充定义 ====================
#ifdef _WIN32
#ifndef _PKEY_Device_FriendlyName
static const PROPERTYKEY _PKEY_Device_FriendlyName = {
    {0xa45c254e, 0xdf1c, 0x4efd, {0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}},
    14
};
#define PKEY_Device_FriendlyName _PKEY_Device_FriendlyName
#endif
#endif

// ==================== 构造/析构 ====================

// 构造函数
AudioCore::AudioCore()
    : m_running(false)
    , m_waveform_points(512)
    , m_bar_count(16)
    , m_fft_size(1024)
    , m_sample_rate(44100)
    , m_dev_dir(AUDIO_DEV_OUT)
#ifdef __linux__
    , m_pa_loop(nullptr)
    , m_pa_ctx(nullptr)
    , m_pa_stream(nullptr)
#endif
#ifdef __APPLE__
    , m_au(nullptr)
#endif
{
}

// 析构函数
AudioCore::~AudioCore()
{
    stop();
}

// ==================== 属性 getter/setter ====================

// 获取波形数据点数
uint32_t AudioCore::get_waveform_points() const
{
    return m_waveform_points;
}
// 设置波形数据点数
void AudioCore::set_waveform_points(uint32_t points)
{
    m_waveform_points = points;
}

// 获取频谱柱子数量
uint32_t AudioCore::get_bar_count() const
{
    return m_bar_count;
}
// 设置频谱柱子数量
void AudioCore::set_bar_count(uint32_t count)
{
    m_bar_count = count;
}

// ==================== 设备枚举 ====================

// 枚举指定方向的音频设备
std::vector<AudioDevInfo> AudioCore::enum_devices(AudioDevDir dir)
{
    std::vector<AudioDevInfo> devices;

#ifdef _WIN32
    // Windows: WASAPI 枚举
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool need_uninit = (hr == S_OK);

    IMMDeviceEnumerator *p_enumerator = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                          CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                          (void **)&p_enumerator);
    if (FAILED(hr)) {
        if (need_uninit) CoUninitialize();
        return devices;
    }

    EDataFlow data_flow = (dir == AUDIO_DEV_OUT) ? eRender : eCapture;
    IMMDeviceCollection *p_collection = nullptr;
    hr = p_enumerator->EnumAudioEndpoints(data_flow, DEVICE_STATE_ACTIVE, &p_collection);
    p_enumerator->Release();
    if (FAILED(hr)) {
        if (need_uninit) CoUninitialize();
        return devices;
    }

    UINT count = 0;
    p_collection->GetCount(&count);

    for (UINT i = 0; i < count; i++) {
        IMMDevice *p_device = nullptr;
        hr = p_collection->Item(i, &p_device);
        if (FAILED(hr)) continue;

        LPWSTR p_id = nullptr;
        hr = p_device->GetId(&p_id);
        if (FAILED(hr)) {
            p_device->Release();
            continue;
        }

        AudioDevInfo info;
        info.dir = dir;
        int len = WideCharToMultiByte(CP_UTF8, 0, p_id, -1, nullptr, 0, nullptr, nullptr);
        if (len > 0) {
            info.id.resize(len - 1);
            WideCharToMultiByte(CP_UTF8, 0, p_id, -1, &info.id[0], len, nullptr, nullptr);
        }
        CoTaskMemFree(p_id);

        IPropertyStore *p_props = nullptr;
        hr = p_device->OpenPropertyStore(STGM_READ, &p_props);
        if (SUCCEEDED(hr)) {
            PROPVARIANT prop;
            PropVariantInit(&prop);
            hr = p_props->GetValue(PKEY_Device_FriendlyName, &prop);
            if (SUCCEEDED(hr) && prop.pwszVal != nullptr) {
                int name_len = WideCharToMultiByte(CP_UTF8, 0, prop.pwszVal, -1, nullptr, 0, nullptr, nullptr);
                if (name_len > 0) {
                    info.name.resize(name_len - 1);
                    WideCharToMultiByte(CP_UTF8, 0, prop.pwszVal, -1, &info.name[0], name_len, nullptr, nullptr);
                }
            }
            PropVariantClear(&prop);
            p_props->Release();
        }

        p_device->Release();
        devices.push_back(info);
    }

    p_collection->Release();
    if (need_uninit) CoUninitialize();

#elif defined(__linux__)
    // Linux: PulseAudio 枚举
    pa_mainloop *loop = pa_mainloop_new();
    pa_context *ctx = pa_context_new(pa_mainloop_get_api(loop), "KRAY_enum");

    pa_context_connect(ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr);

    // 等待连接就绪
    while (pa_context_get_state(ctx) < PA_CONTEXT_READY) {
        pa_mainloop_iterate(loop, 1, nullptr);
    }
    if (pa_context_get_state(ctx) != PA_CONTEXT_READY) {
        pa_context_unref(ctx);
        pa_mainloop_free(loop);
        return devices;
    }

    // 获取服务器信息以确定默认采样率
    pa_server_info *server_info = nullptr;
    pa_operation *op = pa_context_get_server_info(ctx,
        [](pa_context *c, const pa_server_info *i, void *userdata) {
            *static_cast<pa_server_info **>(userdata) = const_cast<pa_server_info *>(i);
        }, &server_info);
    while (pa_operation_get_state(op) == PA_OPERATION_RUNNING) {
        pa_mainloop_iterate(loop, 1, nullptr);
    }
    pa_operation_unref(op);

    // 枚举设备
    auto enum_cb = [](pa_context *c, const pa_sink_info *i, int eol, void *userdata) {
        if (eol || !i) return;
        auto *devs = static_cast<std::vector<AudioDevInfo> *>(userdata);
        AudioDevInfo info;
        info.id = i->name;
        info.name = i->description;
        info.dir = AUDIO_DEV_OUT;
        devs->push_back(info);
    };
    auto enum_src_cb = [](pa_context *c, const pa_source_info *i, int eol, void *userdata) {
        if (eol || !i) return;
        auto *devs = static_cast<std::vector<AudioDevInfo> *>(userdata);
        AudioDevInfo info;
        info.id = i->name;
        info.name = i->description;
        info.dir = AUDIO_DEV_IN;
        devs->push_back(info);
    };

    if (dir == AUDIO_DEV_OUT) {
        op = pa_context_get_sink_info_list(ctx, enum_cb, &devices);
    } else {
        op = pa_context_get_source_info_list(ctx, enum_src_cb, &devices);
    }
    while (pa_operation_get_state(op) == PA_OPERATION_RUNNING) {
        pa_mainloop_iterate(loop, 1, nullptr);
    }
    pa_operation_unref(op);

    pa_context_disconnect(ctx);
    pa_context_unref(ctx);
    pa_mainloop_free(loop);

#elif defined(__APPLE__)
    // macOS: CoreAudio 枚举
    UInt32 prop_size = 0;
    AudioObjectPropertyAddress prop_addr = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &prop_addr, 0, nullptr, &prop_size);
    int device_count = prop_size / sizeof(AudioDeviceID);
    if (device_count <= 0) return devices;

    std::vector<AudioDeviceID> dev_ids(device_count);
    AudioObjectGetPropertyData(kAudioObjectSystemObject, &prop_addr, 0, nullptr, &prop_size, dev_ids.data());

    for (int i = 0; i < device_count; i++) {
        AudioDeviceID dev_id = dev_ids[i];

        // 获取设备方向
        prop_addr.mSelector = kAudioDevicePropertyStreamConfiguration;
        prop_addr.mScope = (dir == AUDIO_DEV_OUT) ? kAudioDevicePropertyScopeOutput : kAudioDevicePropertyScopeInput;
        prop_addr.mElement = kAudioObjectPropertyElementMaster;

        prop_size = 0;
        OSStatus err = AudioObjectGetPropertyDataSize(dev_id, &prop_addr, 0, nullptr, &prop_size);
        if (err != noErr || prop_size == 0) continue;

        AudioBufferList *buf_list = (AudioBufferList *)malloc(prop_size);
        err = AudioObjectGetPropertyData(dev_id, &prop_addr, 0, nullptr, &prop_size, buf_list);
        bool has_channels = false;
        if (err == noErr) {
            for (UInt32 b = 0; b < buf_list->mNumberBuffers; b++) {
                if (buf_list->mBuffers[b].mNumberChannels > 0) {
                    has_channels = true;
                    break;
                }
            }
        }
        free(buf_list);
        if (!has_channels) continue;

        // 获取设备名称
        prop_addr.mSelector = kAudioDevicePropertyDeviceNameCFString;
        prop_addr.mScope = kAudioObjectPropertyScopeGlobal;
        prop_addr.mElement = kAudioObjectPropertyElementMaster;
        CFStringRef cf_name = nullptr;
        prop_size = sizeof(cf_name);
        err = AudioObjectGetPropertyData(dev_id, &prop_addr, 0, nullptr, &prop_size, &cf_name);

        AudioDevInfo info;
        info.dir = dir;
        info.id = std::to_string(dev_id);
        if (err == noErr && cf_name) {
            char name_buf[256] = {0};
            CFStringGetCString(cf_name, name_buf, sizeof(name_buf), kCFStringEncodingUTF8);
            info.name = name_buf;
            CFRelease(cf_name);
        } else {
            info.name = "Audio Device " + std::to_string(dev_id);
        }

        devices.push_back(info);
    }
#endif

    return devices;
}

// ==================== 设备设置 ====================

// 设置采集设备 ID 和方向
void AudioCore::set_device(const std::string &device_id, AudioDevDir dir)
{
    m_device_id = device_id;
    m_dev_dir = dir;
}

// ==================== 启动/停止 ====================

// 启动采集
bool AudioCore::start()
{
    if (m_running) return true;

    m_running = true;
    m_thread = std::thread(&AudioCore::capture_loop, this);
    return true;
}

// 停止采集
void AudioCore::stop()
{
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

// 是否正在采集
bool AudioCore::is_running() const
{
    return m_running;
}

// ==================== 回调设置 ====================

// 设置频谱回调
void AudioCore::on_spectrum(SpectrumCb cb)
{
    m_spectrum_cb = cb;
}

// 设置波形回调
void AudioCore::on_waveform(WaveformCb cb)
{
    m_waveform_cb = cb;
}

// 设置能量回调
void AudioCore::on_energy(EnergyCb cb)
{
    m_energy_cb = cb;
}

// 设置错误回调
void AudioCore::on_error(ErrorCb cb)
{
    m_error_cb = cb;
}

// ==================== 采集线程 ====================

// 采集线程主循环
void AudioCore::capture_loop()
{
#ifdef _WIN32
    // ==================== Windows WASAPI ====================
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        if (m_error_cb) m_error_cb("COM init failed");
        m_running = false;
        return;
    }

    IMMDeviceEnumerator *p_enumerator = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                          CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                          (void **)&p_enumerator);
    if (FAILED(hr)) {
        if (m_error_cb) m_error_cb("Cannot create device enumerator");
        CoUninitialize();
        m_running = false;
        return;
    }

    EDataFlow data_flow = (m_dev_dir == AUDIO_DEV_OUT) ? eRender : eCapture;
    ERole role = eConsole;

    IMMDevice *p_device = nullptr;
    if (!m_device_id.empty()) {
        int wlen = MultiByteToWideChar(CP_UTF8, 0, m_device_id.c_str(), -1, nullptr, 0);
        if (wlen > 0) {
            std::vector<wchar_t> wbuf(wlen);
            MultiByteToWideChar(CP_UTF8, 0, m_device_id.c_str(), -1, wbuf.data(), wlen);
            hr = p_enumerator->GetDevice(wbuf.data(), &p_device);
        }
        if (FAILED(hr) || p_device == nullptr) {
            p_enumerator->GetDefaultAudioEndpoint(data_flow, role, &p_device);
        }
    }
    if (p_device == nullptr) {
        hr = p_enumerator->GetDefaultAudioEndpoint(data_flow, role, &p_device);
    }
    p_enumerator->Release();
    if (FAILED(hr) || p_device == nullptr) {
        if (m_error_cb) m_error_cb("Cannot get audio device");
        CoUninitialize();
        m_running = false;
        return;
    }

    IAudioClient *p_audio_client = nullptr;
    hr = p_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
                            nullptr, (void **)&p_audio_client);
    p_device->Release();
    if (FAILED(hr)) {
        if (m_error_cb) m_error_cb("Cannot activate audio client");
        CoUninitialize();
        m_running = false;
        return;
    }

    WAVEFORMATEX *p_format = nullptr;
    hr = p_audio_client->GetMixFormat(&p_format);
    if (FAILED(hr)) {
        p_audio_client->Release();
        if (m_error_cb) m_error_cb("Cannot get audio format");
        CoUninitialize();
        m_running = false;
        return;
    }

    m_sample_rate = p_format->nSamplesPerSec;
    UINT32 channels = p_format->nChannels;
    UINT32 bits_per_sample = p_format->wBitsPerSample;

    DWORD stream_flags = AUDCLNT_SHAREMODE_SHARED;
    if (m_dev_dir == AUDIO_DEV_OUT) {
        stream_flags |= AUDCLNT_STREAMFLAGS_LOOPBACK;
    }

    REFERENCE_TIME hns_duration = 10000000;
    hr = p_audio_client->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        stream_flags,
        hns_duration, 0, p_format, nullptr);
    if (FAILED(hr)) {
        CoTaskMemFree(p_format);
        p_audio_client->Release();
        if (m_error_cb) m_error_cb("Audio client init failed");
        CoUninitialize();
        m_running = false;
        return;
    }

    IAudioCaptureClient *p_capture = nullptr;
    hr = p_audio_client->GetService(__uuidof(IAudioCaptureClient),
                                    (void **)&p_capture);
    if (FAILED(hr)) {
        CoTaskMemFree(p_format);
        p_audio_client->Release();
        if (m_error_cb) m_error_cb("Cannot get capture client");
        CoUninitialize();
        m_running = false;
        return;
    }

    hr = p_audio_client->Start();
    if (FAILED(hr)) {
        p_capture->Release();
        CoTaskMemFree(p_format);
        p_audio_client->Release();
        if (m_error_cb) m_error_cb("Cannot start capture");
        CoUninitialize();
        m_running = false;
        return;
    }

    UINT32 packet_len = 0;
    UINT32 buffer_frames = 0;
    BYTE *p_data = nullptr;
    DWORD flags = 0;

    while (m_running) {
        hr = p_capture->GetNextPacketSize(&packet_len);
        if (FAILED(hr)) break;

        if (packet_len == 0) {
            Sleep(1);
            continue;
        }

        hr = p_capture->GetBuffer(&p_data, &buffer_frames, &flags, nullptr, nullptr);
        if (FAILED(hr)) break;

        if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT) && p_data != nullptr) {
            std::vector<float> samples;
            convert_to_float(p_data, buffer_frames, channels, bits_per_sample, samples);
            if (!samples.empty()) {
                process_audio(samples.data(), buffer_frames);
            }
        }

        p_capture->ReleaseBuffer(buffer_frames);
    }

    p_audio_client->Stop();
    p_capture->Release();
    CoTaskMemFree(p_format);
    p_audio_client->Release();
    CoUninitialize();

#elif defined(__linux__)
    // ==================== Linux PulseAudio ====================
    m_pa_loop = pa_mainloop_new();
    pa_mainloop_api *api = pa_mainloop_get_api(m_pa_loop);
    m_pa_ctx = pa_context_new(api, "KRAY_capture");

    // 连接 PulseAudio 服务器
    if (pa_context_connect(m_pa_ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0) {
        if (m_error_cb) m_error_cb("PulseAudio connect failed");
        pa_context_unref(m_pa_ctx);
        pa_mainloop_free(m_pa_loop);
        m_pa_loop = nullptr;
        m_pa_ctx = nullptr;
        m_running = false;
        return;
    }

    // 等待连接就绪
    while (pa_context_get_state(m_pa_ctx) < PA_CONTEXT_READY) {
        pa_mainloop_iterate(m_pa_loop, 1, nullptr);
    }
    if (pa_context_get_state(m_pa_ctx) != PA_CONTEXT_READY) {
        if (m_error_cb) m_error_cb("PulseAudio context not ready");
        pa_context_disconnect(m_pa_ctx);
        pa_context_unref(m_pa_ctx);
        pa_mainloop_free(m_pa_loop);
        m_pa_loop = nullptr;
        m_pa_ctx = nullptr;
        m_running = false;
        return;
    }

    // 创建流采样规格
    pa_sample_spec ss;
    ss.format = PA_SAMPLE_FLOAT32LE;
    ss.rate = m_sample_rate;
    ss.channels = 1;

    m_pa_stream = pa_stream_new(m_pa_ctx, "KRAY_capture", &ss, nullptr);
    if (!m_pa_stream) {
        if (m_error_cb) m_error_cb("PulseAudio stream create failed");
        pa_context_disconnect(m_pa_ctx);
        pa_context_unref(m_pa_ctx);
        pa_mainloop_free(m_pa_loop);
        m_pa_loop = nullptr;
        m_pa_ctx = nullptr;
        m_running = false;
        return;
    }

    // 流数据回调
    pa_stream_set_read_callback(m_pa_stream,
        [](pa_stream *s, size_t length, void *userdata) {
            auto *self = static_cast<AudioCore *>(userdata);
            const void *data = nullptr;
            if (pa_stream_peek(s, &data, &length) < 0 || !data) {
                pa_stream_drop(s);
                return;
            }
            size_t frames = length / sizeof(float);
            const float *samples = static_cast<const float *>(data);
            self->process_audio(samples, static_cast<uint32_t>(frames));
            pa_stream_drop(s);
        }, this);

    // 连接流到源
    pa_buffer_attr buf_attr;
    buf_attr.maxlength = (uint32_t)-1;
    buf_attr.tlength = (uint32_t)-1;
    buf_attr.prebuf = (uint32_t)-1;
    buf_attr.minreq = (uint32_t)-1;
    buf_attr.fragsize = 1024 * sizeof(float);

    // 输出设备用 monitor 源（loopback），输入设备用普通源
    std::string source_name = m_device_id;
    if (m_dev_dir == AUDIO_DEV_OUT && !source_name.empty()) {
        // PulseAudio sink 的 monitor 源名 = sink 名 + ".monitor"
        if (source_name.find(".monitor") == std::string::npos) {
            source_name += ".monitor";
        }
    }

    pa_stream_flags_t flags = PA_STREAM_NOFLAGS;
    int ret;
    if (!source_name.empty()) {
        ret = pa_stream_connect_record(m_pa_stream, source_name.c_str(), &buf_attr, flags);
    } else {
        // 不指定设备：输出模式用默认 monitor，输入模式用默认源
        if (m_dev_dir == AUDIO_DEV_OUT) {
            ret = pa_stream_connect_record(m_pa_stream, nullptr, &buf_attr, flags);
        } else {
            ret = pa_stream_connect_record(m_pa_stream, nullptr, &buf_attr, flags);
        }
    }

    if (ret < 0) {
        if (m_error_cb) m_error_cb("PulseAudio stream connect failed");
        pa_stream_unref(m_pa_stream);
        pa_context_disconnect(m_pa_ctx);
        pa_context_unref(m_pa_ctx);
        pa_mainloop_free(m_pa_loop);
        m_pa_stream = nullptr;
        m_pa_ctx = nullptr;
        m_pa_loop = nullptr;
        m_running = false;
        return;
    }

    // 运行 PulseAudio 主循环
    while (m_running) {
        int ret_val = pa_mainloop_iterate(m_pa_loop, 1, nullptr);
        if (ret_val < 0) break;
    }

    // 清理
    if (m_pa_stream) {
        pa_stream_disconnect(m_pa_stream);
        pa_stream_unref(m_pa_stream);
        m_pa_stream = nullptr;
    }
    if (m_pa_ctx) {
        pa_context_disconnect(m_pa_ctx);
        pa_context_unref(m_pa_ctx);
        m_pa_ctx = nullptr;
    }
    if (m_pa_loop) {
        pa_mainloop_free(m_pa_loop);
        m_pa_loop = nullptr;
    }

#elif defined(__APPLE__)
    // ==================== macOS CoreAudio ====================
    AudioComponentDescription desc;
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_HALOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    AudioComponent comp = AudioComponentFindNext(nullptr, &desc);
    OSStatus err = AudioComponentInstanceNew(comp, &m_au);
    if (err != noErr) {
        if (m_error_cb) m_error_cb("Cannot create AudioUnit");
        m_running = false;
        return;
    }

    // 输出设备用启用输入侧 + loopback（通过 kAudioOutputUnitProperty_EnableOutput）
    if (m_dev_dir == AUDIO_DEV_OUT) {
        // 禁用输出，启用输入（用于 loopback）
        UInt32 disable = 0;
        AudioUnitSetProperty(m_au, kAudioOutputUnitProperty_EnableOutput,
                             kAudioUnitScope_Output, 0, &disable, sizeof(disable));
        UInt32 enable = 1;
        AudioUnitSetProperty(m_au, kAudioOutputUnitProperty_EnableInput,
                             kAudioUnitScope_Input, 1, &enable, sizeof(enable));
    } else {
        // 输入设备：启用输入
        UInt32 enable = 1;
        AudioUnitSetProperty(m_au, kAudioOutputUnitProperty_EnableInput,
                             kAudioUnitScope_Input, 1, &enable, sizeof(enable));
        UInt32 disable = 0;
        AudioUnitSetProperty(m_au, kAudioOutputUnitProperty_EnableOutput,
                             kAudioUnitScope_Output, 0, &disable, sizeof(disable));
    }

    // 设置设备
    if (!m_device_id.empty()) {
        AudioDeviceID dev_id = (AudioDeviceID)std::stoi(m_device_id);
        UInt32 prop_size = sizeof(dev_id);
        AudioUnitSetProperty(m_au, kAudioOutputUnitProperty_CurrentDevice,
                             kAudioUnitScope_Global, 0, &dev_id, prop_size);
    }

    // 设置流格式
    AudioStreamBasicDescription fmt;
    UInt32 fmt_size = sizeof(fmt);
    err = AudioUnitGetProperty(m_au, kAudioUnitProperty_StreamFormat,
                               kAudioUnitScope_Input, 1, &fmt, &fmt_size);
    if (err != noErr) {
        if (m_error_cb) m_error_cb("Cannot get audio format");
        AudioComponentInstanceDispose(m_au);
        m_au = nullptr;
        m_running = false;
        return;
    }

    m_sample_rate = (uint32_t)fmt.mSampleRate;

    // 设置回调
    AURenderCallbackStruct cb_struct;
    cb_struct.inputProc = au_callback;
    cb_struct.inputProcRefCon = this;
    AudioUnitSetProperty(m_au, kAudioOutputUnitProperty_SetInputCallback,
                         kAudioUnitScope_Global, 0, &cb_struct, sizeof(cb_struct));

    // 初始化并启动
    err = AudioUnitInitialize(m_au);
    if (err != noErr) {
        if (m_error_cb) m_error_cb("AudioUnit init failed");
        AudioComponentInstanceDispose(m_au);
        m_au = nullptr;
        m_running = false;
        return;
    }

    err = AudioOutputUnitStart(m_au);
    if (err != noErr) {
        if (m_error_cb) m_error_cb("AudioUnit start failed");
        AudioUnitUninitialize(m_au);
        AudioComponentInstanceDispose(m_au);
        m_au = nullptr;
        m_running = false;
        return;
    }

    // 等待停止
    while (m_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 清理
    AudioOutputUnitStop(m_au);
    AudioUnitUninitialize(m_au);
    AudioComponentInstanceDispose(m_au);
    m_au = nullptr;

#else
    // ==================== 无平台支持 ====================
    if (m_error_cb) m_error_cb("Audio capture not supported on this platform");
    m_running = false;
#endif

    m_running = false;
}

// ==================== macOS AudioUnit 回调 ====================
#ifdef __APPLE__
OSStatus AudioCore::au_callback(void *inRefCon,
                                AudioUnitRenderActionFlags *ioActionFlags,
                                const AudioTimeStamp *inTimeStamp,
                                UInt32 inBusNumber,
                                UInt32 inNumberFrames,
                                AudioBufferList *ioData)
{
    auto *self = static_cast<AudioCore *>(inRefCon);
    if (!self->m_running) return noErr;

    AudioBufferList buf_list;
    buf_list.mNumberBuffers = 1;
    buf_list.mBuffers[0].mNumberChannels = 1;
    buf_list.mBuffers[0].mDataByteSize = inNumberFrames * sizeof(float);
    buf_list.mBuffers[0].mData = malloc(inNumberFrames * sizeof(float));

    OSStatus err = AudioUnitRender(self->m_au, ioActionFlags, inTimeStamp,
                                   inBusNumber, inNumberFrames, &buf_list);
    if (err == noErr) {
        float *data = static_cast<float *>(buf_list.mBuffers[0].mData);
        self->process_audio(data, inNumberFrames);
    }

    free(buf_list.mBuffers[0].mData);
    return noErr;
}
#endif

// ==================== 数据转换 ====================

// 将采集数据转为 float 格式（取左声道）
void AudioCore::convert_to_float(uint8_t *data, uint32_t frames,
                                  uint32_t channels, uint32_t bits,
                                  std::vector<float> &out)
{
    out.resize(frames);

    if (bits == 16) {
        int16_t *p = reinterpret_cast<int16_t *>(data);
        for (uint32_t i = 0; i < frames; i++) {
            out[i] = static_cast<float>(p[i * channels]) / 32768.0f;
        }
    } else if (bits == 32) {
        float *p = reinterpret_cast<float *>(data);
        for (uint32_t i = 0; i < frames; i++) {
            out[i] = p[i * channels];
        }
    } else if (bits == 24) {
        for (uint32_t i = 0; i < frames; i++) {
            uint32_t offset = i * channels * 3;
            int32_t val = static_cast<int32_t>(data[offset])
                        | (static_cast<int32_t>(data[offset + 1]) << 8)
                        | (static_cast<int32_t>(static_cast<int8_t>(data[offset + 2])) << 16);
            out[i] = static_cast<float>(val) / 8388608.0f;
        }
    } else {
        std::fill(out.begin(), out.end(), 0.0f);
    }
}

// ==================== 音频处理 ====================

// 处理音频数据：FFT + 波形 + 能量
void AudioCore::process_audio(const float *samples, uint32_t frame_count)
{
    // ---- 波形数据 ----
    uint32_t wave_points = m_waveform_points;
    std::vector<float> waveform(wave_points);
    for (uint32_t i = 0; i < wave_points; i++) {
        uint32_t idx = static_cast<uint32_t>(static_cast<float>(i) / wave_points * frame_count);
        if (idx >= frame_count) idx = frame_count - 1;
        waveform[i] = samples[idx];
    }
    if (m_waveform_cb) m_waveform_cb(waveform.data(), wave_points);

    // ---- FFT 频谱数据 ----
    uint32_t fft_n = m_fft_size;
    std::vector<float> real(fft_n, 0.0f);
    std::vector<float> imag(fft_n, 0.0f);

    uint32_t copy_len = frame_count < fft_n ? frame_count : fft_n;
    for (uint32_t i = 0; i < copy_len; i++) {
        float window = 0.5f * (1.0f - cosf(2.0f * static_cast<float>(M_PI) * i / (copy_len - 1)));
        real[i] = samples[i] * window;
    }

    fft(real, imag);

    uint32_t bar_count = m_bar_count;
    std::vector<float> spectrum(bar_count);
    uint32_t half_n = fft_n / 2;

    float log_min = log2f(2.0f);
    float log_max = log2f(static_cast<float>(half_n));
    uint32_t prev_hi = 2;
    for (uint32_t i = 0; i < bar_count; i++) {
        float lo = powf(2.0f, log_min + (log_max - log_min) * i / bar_count);
        float hi = powf(2.0f, log_min + (log_max - log_min) * (i + 1) / bar_count);
        uint32_t bin_lo = static_cast<uint32_t>(lo);
        uint32_t bin_hi = static_cast<uint32_t>(hi);
        if (bin_lo < prev_hi) bin_lo = prev_hi;
        if (bin_hi <= bin_lo) bin_hi = bin_lo + 1;
        if (bin_hi > half_n) bin_hi = half_n;
        prev_hi = bin_hi;
        int count = bin_hi - bin_lo;
        if (count <= 0) count = 1;
        float mag_sum = 0.0f;
        for (uint32_t j = bin_lo; j < bin_hi; j++) {
            float mag = sqrtf(real[j] * real[j] + imag[j] * imag[j]);
            mag_sum += mag;
        }
        spectrum[i] = mag_sum / count / (fft_n / 2);
    }
    if (m_spectrum_cb) m_spectrum_cb(spectrum.data(), bar_count);

    // ---- 总能量 ----
    float energy = 0.0f;
    for (uint32_t i = 0; i < frame_count; i++) {
        energy += samples[i] * samples[i];
    }
    energy = sqrtf(energy / frame_count);
    if (energy > 1.0f) energy = 1.0f;
    if (m_energy_cb) m_energy_cb(energy);
}

// ==================== FFT ====================

// 基2 Cooley-Tukey FFT
void AudioCore::fft(std::vector<float> &real, std::vector<float> &imag)
{
    uint32_t n = static_cast<uint32_t>(real.size());

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
