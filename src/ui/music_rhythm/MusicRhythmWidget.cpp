#include "MusicRhythmWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QIcon>
#include <QDebug>
#include <QHBoxLayout>

#ifdef _WIN32
#include <Windows.h>
#endif

// 频谱柱数
static const int BAR_COUNT = 64;
// 刷新间隔（毫秒）
static const int REFRESH_MS = 33;

// 构造函数
MusicRhythmWidget::MusicRhythmWidget(QWidget *parent)
    : QWidget(parent)
    , m_capture(nullptr)
    , m_timer(nullptr)
    , m_energy(0.0f)
    , m_device_combo(nullptr)
    , m_device_label(nullptr)
    , m_spec_gain_slider(nullptr)
    , m_spec_gain_label(nullptr)
    , m_spec_gain(10.0f) // 初始频谱增益
    , m_wave_gain_slider(nullptr)
    , m_wave_gain_label(nullptr)
    , m_wave_gain(1.0f) // 初始波形增益
    , m_energy_gain_slider(nullptr)
    , m_energy_gain_label(nullptr)
    , m_energy_gain(1.0f) // 初始能量增益
{
    // 窗口属性
    setAttribute(Qt::WA_QuitOnClose, false);
    setWindowTitle("Music Rhythm");
    setWindowIcon(QIcon(":/images/pixel_parrot.png"));
    resize(800, 600);
    setMinimumSize(600, 400);

    // 注册跨线程信号所需的元类型
    qRegisterMetaType<QVector<float>>("QVector<float>");

    // 初始化数据
    m_spectrum.resize(BAR_COUNT);
    m_spectrum.fill(0.0f);
    m_waveform.resize(256);
    m_waveform.fill(0.0f);
    m_spectrum_peak.resize(BAR_COUNT);
    m_spectrum_peak.fill(0.0f);
    m_spectrum_fall.resize(BAR_COUNT);
    m_spectrum_fall.fill(0.0f);

    // 创建控制面板
    create_ctrl_panel();

    // 创建音频采集器
    m_capture = new AudioCapture(this);

    // 连接采集信号
    connect(m_capture, &AudioCapture::sig_spectrum,
            this, &MusicRhythmWidget::on_spectrum);
    connect(m_capture, &AudioCapture::sig_waveform,
            this, &MusicRhythmWidget::on_waveform);
    connect(m_capture, &AudioCapture::sig_energy,
            this, &MusicRhythmWidget::on_energy);
    connect(m_capture, &AudioCapture::sig_error,
            this, [](const QString &msg) {
        qDebug() << "AudioCapture error:" << msg;
    });

    // 转发采集信号到外部（供外部设备控制使用）
    connect(m_capture, &AudioCapture::sig_spectrum,
            this, &MusicRhythmWidget::sig_spectrum);
    connect(m_capture, &AudioCapture::sig_waveform,
            this, &MusicRhythmWidget::sig_waveform);
    connect(m_capture, &AudioCapture::sig_energy,
            this, &MusicRhythmWidget::sig_energy);

    // 刷新设备列表并启动采集
    refresh_devices();

    // 创建刷新定时器
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MusicRhythmWidget::on_refresh);
    m_timer->start(REFRESH_MS);
}

// 析构函数
MusicRhythmWidget::~MusicRhythmWidget()
{
    if (m_capture) {
        m_capture->stop();
    }
    if (m_timer) {
        m_timer->stop();
    }
}

// 关闭事件
void MusicRhythmWidget::closeEvent(QCloseEvent *event)
{
    if (m_capture) {
        m_capture->stop();
    }
    if (m_timer) {
        m_timer->stop();
    }
    event->accept();
    emit exitWindow();
    QWidget::closeEvent(event);
}

// 置顶显示
void MusicRhythmWidget::show_top(void)
{
    if (this->isMinimized()) {
        this->showNormal();
    }
#ifdef _WIN32
    SetWindowPos(HWND(this->winId()), HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetWindowPos(HWND(this->winId()), HWND_NOTOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#endif
    this->show();
    this->activateWindow();
}

// 更新频谱数据
void MusicRhythmWidget::on_spectrum(const QVector<float> &data)
{
    m_spectrum = data;
}

// 更新波形数据
void MusicRhythmWidget::on_waveform(const QVector<float> &data)
{
    m_waveform = data;
}

// 更新能量数据
void MusicRhythmWidget::on_energy(float energy)
{
    m_energy = energy;
}

// 切换采集设备
void MusicRhythmWidget::on_device_changed(int index)
{
    if (!m_capture || index < 0) return;

    // 从 itemData 取出 "dir:id" 格式的数据
    QString data = m_device_combo->itemData(index).toString();
    if (data.isEmpty()) return;

    // 解析方向和设备 ID
    AudioDevDir dir = AUDIO_DEV_OUT;
    QString device_id;
    if (data.startsWith("out:")) {
        dir = AUDIO_DEV_OUT;
        device_id = data.mid(4);
    } else if (data.startsWith("in:")) {
        dir = AUDIO_DEV_IN;
        device_id = data.mid(3);
    }

    // 停止、设置、重启
    m_capture->stop();
    m_capture->set_device(device_id, dir);
    m_capture->start();
}

// 定时刷新：更新峰值下落 + 重绘
void MusicRhythmWidget::on_refresh()
{
    // 更新频谱峰值下落效果
    for (int i = 0; i < BAR_COUNT && i < m_spectrum.size(); i++) {
        if (m_spectrum[i] >= m_spectrum_peak[i]) {
            m_spectrum_peak[i] = m_spectrum[i];
            m_spectrum_fall[i] = 0.0f;
        } else {
            m_spectrum_fall[i] += 0.001f;
            m_spectrum_peak[i] -= m_spectrum_fall[i];
            if (m_spectrum_peak[i] < 0.0f) {
                m_spectrum_peak[i] = 0.0f;
            }
        }
    }
    update();
}

// 绘制事件
void MusicRhythmWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 背景
    painter.fillRect(rect(), QColor(15, 15, 25));

    // 布局：上半部分频谱，下半部分波形，右侧能量条
    int margin = 10;
    int energy_bar_w = 30;
    int main_w = width() - margin * 2 - energy_bar_w - margin;
    int main_h = height() - margin * 2 - 40; // 底部留空给控制面板
    int half_h = main_h / 2 - margin / 2;

    // 频谱区域
    QRect spec_rect(margin, margin, main_w, half_h);
    draw_spectrum(painter, spec_rect);

    // 波形区域
    QRect wave_rect(margin, margin + half_h + margin, main_w, half_h);
    draw_waveform(painter, wave_rect);

    // 能量指示器区域
    QRect energy_rect(width() - margin - energy_bar_w, margin, energy_bar_w, main_h);
    draw_energy(painter, energy_rect);

    // 定位控制面板到窗口底部
    int ctrl_y = height() - 35;
    int x = margin;

    // 设备选择
    m_device_label->move(x, ctrl_y + 3);
    x += m_device_label->width() + 3;
    m_device_combo->move(x, ctrl_y);
    x += m_device_combo->width() + 15;

    // 频谱增益
    m_spec_gain_label->move(x, ctrl_y + 3);
    x += m_spec_gain_label->width() + 3;
    m_spec_gain_slider->move(x, ctrl_y + 2);
    x += m_spec_gain_slider->width() + 10;

    // 波形增益
    m_wave_gain_label->move(x, ctrl_y + 3);
    x += m_wave_gain_label->width() + 3;
    m_wave_gain_slider->move(x, ctrl_y + 2);
    x += m_wave_gain_slider->width() + 10;

    // 能量增益
    m_energy_gain_label->move(x, ctrl_y + 3);
    x += m_energy_gain_label->width() + 3;
    m_energy_gain_slider->move(x, ctrl_y + 2);
}

// 绘制频谱柱状图
void MusicRhythmWidget::draw_spectrum(QPainter &painter, const QRect &rect)
{
    int n = qMin(m_spectrum.size(), m_spectrum_peak.size());
    if (n == 0) return;

    float bar_w = (float)rect.width() / n;
    float gap = qMax(1.0f, bar_w * 0.15f);
    float draw_w = bar_w - gap;

    for (int i = 0; i < n; i++) {
        float val = m_spectrum[i] * m_spec_gain;
        if (val < 0.0f) val = 0.0f;
        if (val > 1.0f) val = 1.0f;

        float bar_h = val * rect.height();
        float x = rect.x() + i * bar_w + gap / 2;
        float y = rect.y() + rect.height() - bar_h;

        QLinearGradient grad(x, rect.y() + rect.height(), x, rect.y());
        grad.setColorAt(0.0, QColor(0, 100, 255));
        grad.setColorAt(0.5, QColor(0, 255, 100));
        grad.setColorAt(1.0, QColor(255, 50, 50));

        painter.setBrush(grad);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(QRectF(x, y, draw_w, bar_h), 1.0, 1.0);

        // 峰值指示条
        float peak_val = m_spectrum_peak[i] * m_spec_gain;
        if (peak_val > 1.0f) peak_val = 1.0f;
        float peak_y = rect.y() + rect.height() - peak_val * rect.height();
        painter.setBrush(QColor(255, 255, 255, 200));
        painter.drawRect(QRectF(x, peak_y, draw_w, 2));
    }
}

// 绘制波形图
void MusicRhythmWidget::draw_waveform(QPainter &painter, const QRect &rect)
{
    int n = m_waveform.size();
    if (n == 0) return;

    // 中线
    float mid_y = rect.y() + rect.height() / 2.0f;
    painter.setPen(QPen(QColor(60, 60, 80), 1));
    painter.drawLine(rect.x(), (int)mid_y, rect.x() + rect.width(), (int)mid_y);

    // 波形路径
    QPainterPath path;
    float step = (float)rect.width() / (n - 1);

    path.moveTo(rect.x(), mid_y - m_waveform[0] * m_wave_gain * rect.height() / 2.0f);
    for (int i = 1; i < n; i++) {
        float x = rect.x() + i * step;
        float y = mid_y - m_waveform[i] * m_wave_gain * rect.height() / 2.0f;
        path.lineTo(x, y);
    }

    // 绘制波形线条
    QLinearGradient grad(rect.x(), rect.y(), rect.x() + rect.width(), rect.y());
    grad.setColorAt(0.0, QColor(0, 200, 255));
    grad.setColorAt(1.0, QColor(100, 255, 100));

    painter.setPen(QPen(grad, 1.5));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    // 绘制半透明填充
    QPainterPath fill_path = path;
    fill_path.lineTo(rect.x() + rect.width(), mid_y);
    fill_path.lineTo(rect.x(), mid_y);
    fill_path.closeSubpath();

    QLinearGradient fill_grad(rect.x(), rect.y(), rect.x(), rect.y() + rect.height());
    fill_grad.setColorAt(0.0, QColor(0, 200, 255, 40));
    fill_grad.setColorAt(0.5, QColor(0, 200, 255, 10));
    fill_grad.setColorAt(1.0, QColor(0, 200, 255, 40));

    painter.setPen(Qt::NoPen);
    painter.setBrush(fill_grad);
    painter.drawPath(fill_path);
}

// 绘制能量指示器
void MusicRhythmWidget::draw_energy(QPainter &painter, const QRect &rect)
{
    float val = m_energy * m_energy_gain;
    if (val < 0.0f) val = 0.0f;
    if (val > 1.0f) val = 1.0f;

    // 背景框
    painter.setPen(QPen(QColor(60, 60, 80), 1));
    painter.setBrush(QColor(20, 20, 35));
    painter.drawRoundedRect(rect, 3, 3);

    // 能量条
    int bar_margin = 3;
    QRectF bar_rect(rect.x() + bar_margin,
                    rect.y() + bar_margin,
                    rect.width() - bar_margin * 2,
                    rect.height() - bar_margin * 2);

    float fill_h = val * bar_rect.height();
    QRectF fill_rect(bar_rect.x(),
                     bar_rect.y() + bar_rect.height() - fill_h,
                     bar_rect.width(),
                     fill_h);

    QLinearGradient grad(bar_rect.x(), bar_rect.y() + bar_rect.height(),
                         bar_rect.x(), bar_rect.y());
    grad.setColorAt(0.0, QColor(0, 100, 255));
    grad.setColorAt(0.6, QColor(255, 200, 0));
    grad.setColorAt(1.0, QColor(255, 50, 50));

    painter.setPen(Qt::NoPen);
    painter.setBrush(grad);
    painter.drawRoundedRect(fill_rect, 2, 2);
}

// 创建控制面板
void MusicRhythmWidget::create_ctrl_panel()
{
    // 通用滑块样式
    QString slider_style =
        "QSlider::groove:horizontal {"
        "   background: #333344;"
        "   height: 6px;"
        "   border-radius: 3px;"
        "}"
        "QSlider::handle:horizontal {"
        "   background: #00aaff;"
        "   width: 14px;"
        "   margin: -4px 0;"
        "   border-radius: 7px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "   background: #0066cc;"
        "   border-radius: 3px;"
        "}";

    // 通用标签样式
    QString label_style =
        "color: #cccccc;"
        "font-size: 12px;"
        "background: transparent;";

    // 通用下拉框样式
    QString combo_style =
        "QComboBox {"
        "   background: #222233;"
        "   color: #cccccc;"
        "   border: 1px solid #444466;"
        "   border-radius: 3px;"
        "   padding: 2px 5px;"
        "   font-size: 11px;"
        "}"
        "QComboBox::drop-down {"
        "   border: none;"
        "}"
        "QComboBox QAbstractItemView {"
        "   background: #222233;"
        "   color: #cccccc;"
        "   selection-background-color: #0066cc;"
        "}";

    // ---- 设备选择（输入输出合一） ----
    m_device_label = new QLabel(this);
    m_device_label->setStyleSheet(label_style);
    m_device_label->setText("DEV:");

    m_device_combo = new QComboBox(this);
    m_device_combo->setFixedWidth(200);
    m_device_combo->setStyleSheet(combo_style);
    connect(m_device_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MusicRhythmWidget::on_device_changed);

    // ---- 频谱增益 ----
    m_spec_gain_label = new QLabel(this);
    m_spec_gain_label->setStyleSheet(label_style);
    m_spec_gain_label->setText(QString("SPEC: %1").arg(m_spec_gain, 0, 'f', 0));
    m_spec_gain_slider = new QSlider(Qt::Horizontal, this);
    m_spec_gain_slider->setRange(1, 100);
    m_spec_gain_slider->setSingleStep(1);
    m_spec_gain_slider->setPageStep(1);
    m_spec_gain_slider->setValue((int)m_spec_gain);
    m_spec_gain_slider->setFixedWidth(80);
    m_spec_gain_slider->setStyleSheet(slider_style);
    connect(m_spec_gain_slider, &QSlider::valueChanged, this, [this](int val) {
        m_spec_gain = (float)val;
        m_spec_gain_label->setText(QString("SPEC: %1").arg(m_spec_gain, 0, 'f', 0));
    });

    // ---- 波形增益 ----
    m_wave_gain_label = new QLabel(this);
    m_wave_gain_label->setStyleSheet(label_style);
    m_wave_gain_label->setText(QString("WAVE: %1").arg(m_wave_gain, 0, 'f', 0));
    m_wave_gain_slider = new QSlider(Qt::Horizontal, this);
    m_wave_gain_slider->setRange(1, 100);
    m_wave_gain_slider->setSingleStep(1);
    m_wave_gain_slider->setPageStep(1);
    m_wave_gain_slider->setValue((int)m_wave_gain);
    m_wave_gain_slider->setFixedWidth(80);
    m_wave_gain_slider->setStyleSheet(slider_style);
    connect(m_wave_gain_slider, &QSlider::valueChanged, this, [this](int val) {
        m_wave_gain = (float)val;
        m_wave_gain_label->setText(QString("WAVE: %1").arg(m_wave_gain, 0, 'f', 0));
    });

    // ---- 能量增益 ----
    m_energy_gain_label = new QLabel(this);
    m_energy_gain_label->setStyleSheet(label_style);
    m_energy_gain_label->setText(QString("NRG: %1").arg(m_energy_gain, 0, 'f', 0));
    m_energy_gain_slider = new QSlider(Qt::Horizontal, this);
    m_energy_gain_slider->setRange(1, 100);
    m_energy_gain_slider->setSingleStep(1);
    m_energy_gain_slider->setPageStep(1);
    m_energy_gain_slider->setValue((int)m_energy_gain);
    m_energy_gain_slider->setFixedWidth(80);
    m_energy_gain_slider->setStyleSheet(slider_style);
    connect(m_energy_gain_slider, &QSlider::valueChanged, this, [this](int val) {
        m_energy_gain = (float)val;
        m_energy_gain_label->setText(QString("NRG: %1").arg(m_energy_gain, 0, 'f', 0));
    });
}

// 刷新设备列表
void MusicRhythmWidget::refresh_devices()
{
    // 暂时断开信号，避免填充列表时触发切换
    disconnect(m_device_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
               this, &MusicRhythmWidget::on_device_changed);

    m_device_combo->clear();

    // 枚举输出设备，显示名前加 [OUT]
    auto out_devs = AudioCapture::enum_devices(AUDIO_DEV_OUT);
    for (const auto &dev : out_devs) {
        // itemData 格式 "out:device_id"，默认设备 id 为空
        QString data = "out:" + dev.second;
        m_device_combo->addItem("[OUT] " + dev.first, data);
    }

    // 枚举输入设备，显示名前加 [IN]
    auto in_devs = AudioCapture::enum_devices(AUDIO_DEV_IN);
    for (const auto &dev : in_devs) {
        QString data = "in:" + dev.second;
        m_device_combo->addItem("[IN] " + dev.first, data);
    }

    // 重新连接信号
    connect(m_device_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MusicRhythmWidget::on_device_changed);

    // 启动采集（默认第一个输出设备）
    if (m_capture && m_device_combo->count() > 0) {
        on_device_changed(0);
    }
}
