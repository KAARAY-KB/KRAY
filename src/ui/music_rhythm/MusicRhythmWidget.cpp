#include "MusicRhythmWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QIcon>
#include <QDebug>
#include <QHBoxLayout>
#include <cmath>

#ifdef _WIN32
#include <Windows.h>
#endif

#define FFS 30
// 频谱柱数
static const int BAR_COUNT = 64;
// 刷新间隔（毫秒）
static const int REFRESH_MS = 1000 / FFS;

// 构造函数
MusicRhythmWidget::MusicRhythmWidget(QWidget *parent)
    : QWidget(parent)
    , m_energy(0.0f) // 初始能量值
    , m_wave_pts(1024) // 初始波形点数
    , m_bar_cnt(16) // 初始频谱柱数
    , m_user_bar_cnt(16) // 初始用户自定义柱数
    , m_led_mode(LED_MODE_LAYERS) // 初始LED灯效模式
    , m_spec_gain(1.0f) // 初始频谱增益
    , m_wave_gain(5.0f) // 初始波形增益
    , m_energy_gain(5.0f) // 初始能量增益
    , m_auto_gain(10.0f) // 初始自动增益值
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

    // 初始化LED网格数据
    m_led_grid.resize(80);
    m_led_grid.fill(0.0f);
    m_led_grid_prev.resize(80);
    m_led_grid_prev.fill(0.0f);

    // 创建音频采集器
    m_capture = new AudioCapture(this);

    // 创建控制面板
    create_ctrl_panel();

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

// 切换LED灯效模式
void MusicRhythmWidget::on_led_mode_changed(int index)
{
    m_led_mode = (LedMode)index;

    if (m_capture) {
        if (m_led_mode == LED_MODE_BANDS) {
            // 80频段模式需要80根柱子
            m_capture->set_bar_count(80);
        } else {
            // 其他模式恢复用户设置的柱数
            m_capture->set_bar_count(m_user_bar_cnt);
        }
    }
}

// 映射LED网格数据（16列×5行=80个值）
void MusicRhythmWidget::map_led_grid()
{
    m_led_grid.resize(80);

    int cols = 16;
    int rows = 5;

#if 0
    // 综合增益 = 用户增益 × 自动增益
    float gain = m_spec_gain * m_auto_gain;
#else
    // 综合增益 = 自动增益
    float gain = m_auto_gain;
#endif

    // 节拍脉冲：能量突变时增强LED亮度
    static float prev_energy = 0.0f;
    float beat_pulse = 1.0f;
    float energy_delta = m_energy - prev_energy;
    if (energy_delta > 0.05f) {
        beat_pulse = 1.0f + qMin(energy_delta * 8.0f, 1.5f);
    }
    prev_energy = m_energy;

    // 计算目标LED值
    QVector<float> target(80, 0.0f);

    switch (m_led_mode) {
    case LED_MODE_LAYERS: {
        // 强度分层：每列5级强度，底部先亮，渐变亮度
        for (int c = 0; c < cols && c < m_spectrum.size(); c++) {
            float val = m_spectrum[c] * gain * beat_pulse;
            // 适度压缩，保留动态范围（0.8比0.7更保留对比度）
            val = powf(qBound(0.0f, val, 1.0f), 0.8f);
            float level = val * rows;
            for (int r = 0; r < rows; r++) {
                int dist_from_bottom = rows - 1 - r;
                float cell_val = level - dist_from_bottom;
                if (cell_val > 1.0f) cell_val = 1.0f;
                if (cell_val < 0.0f) cell_val = 0.0f;
                target[r * cols + c] = cell_val;
            }
        }
        break;
    }
    case LED_MODE_BANDS: {
        // 80频段：每个格子对应独立频段
        for (int i = 0; i < 80 && i < m_spectrum.size(); i++) {
            float val = m_spectrum[i] * gain * beat_pulse;
            val = qBound(0.0f, val, 1.0f);
            // 适度压缩，保留强弱对比
            val = powf(val, 0.7f);
            target[i] = val;
        }
        break;
    }
    case LED_MODE_MIRROR: {
        // 镜像波形：用频谱数据做对称，中间行最亮
        for (int c = 0; c < cols && c < m_spectrum.size(); c++) {
            float val = m_spectrum[c] * gain * beat_pulse;
            val = powf(qBound(0.0f, val, 1.0f), 0.8f);
            int center = rows / 2;
            float level = val * (center + 1);
            target[center * cols + c] = val;
            for (int l = 1; l <= center; l++) {
                float cell_val = level - l;
                if (cell_val > 1.0f) cell_val = 1.0f;
                if (cell_val < 0.0f) cell_val = 0.0f;
                if (center - l >= 0) target[(center - l) * cols + c] = cell_val;
                if (center + l < rows) target[(center + l) * cols + c] = cell_val;
            }
        }
        break;
    }
    }

    // 余辉衰减：快速上升，快速衰减——让节拍"打"出来
    for (int i = 0; i < 80; i++) {
        if (target[i] >= m_led_grid_prev[i]) {
            // 新值更大：瞬间亮起
            m_led_grid[i] = target[i];
        } else {
            // 旧值更大：快速衰减，让灯迅速暗下去
            m_led_grid[i] = m_led_grid_prev[i] * 0.65f;
            if (m_led_grid[i] < target[i]) m_led_grid[i] = target[i];
        }
    }

    // 保存当前帧（0~1范围）用于下一帧衰减
    m_led_grid_prev = m_led_grid;

    // 将0~1映射到0~100亮度值
    for (int i = 0; i < 80; i++) {
        m_led_grid[i] = qBound(0.0f, m_led_grid[i] * 100.0f, 100.0f);
    }

    emit sig_led_grid(m_led_grid);
}

// 绘制LED网格预览
void MusicRhythmWidget::draw_led_grid(QPainter &painter, const QRect &rect)
{
    int cols = 16;
    int rows = 5;
    float cell_w = (float)rect.width() / cols;
    float cell_h = (float)rect.height() / rows;
    float gap = 2.0f;

    // 区域标题
    painter.setPen(QColor(120, 120, 140));
    painter.setFont(QFont("Consolas", 8));
    painter.drawText(rect.x(), rect.y() - 2, "LED GRID");

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            float val = m_led_grid[r * cols + c];
            float x = rect.x() + c * cell_w + gap / 2;
            float y = rect.y() + r * cell_h + gap / 2;
            float w = cell_w - gap;
            float h = cell_h - gap;

            // 亮度映射：0~100 → 灰度
            int brightness = qBound(0, (int)val, 100);
            int gray = brightness * 255 / 100;
            QColor color(gray, gray, gray);

            painter.setBrush(color);
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(QRectF(x, y, w, h), 2.0, 2.0);
        }
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

// 定时刷新：更新峰值下落 + 映射LED + 重绘
void MusicRhythmWidget::on_refresh()
{
    // 动态调整峰值数组大小
    if (m_spectrum.size() != m_spectrum_peak.size()) {
        m_spectrum_peak.resize(m_spectrum.size());
        m_spectrum_peak.fill(0.0f);
        m_spectrum_fall.resize(m_spectrum.size());
        m_spectrum_fall.fill(0.0f);
    }

    // 更新频谱峰值下落效果
    for (int i = 0; i < m_spectrum.size(); i++) {
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

    // 自动增益：跟踪频谱峰值，确保LED有足够亮度
    float peak = 0.001f;
    for (int i = 0; i < m_spectrum.size(); i++) {
        if (m_spectrum[i] > peak) peak = m_spectrum[i];
    }
    // 快速上升，较快下降——安静时增益快速回落，避免背景噪音点亮LED
    float target_gain = 1.0f / peak;
    if (target_gain > m_auto_gain) {
        m_auto_gain += (target_gain - m_auto_gain) * 0.5f;
    } else {
        m_auto_gain += (target_gain - m_auto_gain) * 0.15f;
    }
    m_auto_gain = qBound(1.0f, m_auto_gain, 200.0f);

    // 映射LED网格数据
    map_led_grid();

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

    // 布局：上频谱，中LED网格预览，下波形，右侧能量条
    int margin = 10;
    int energy_bar_w = 30;
    int main_w = width() - margin * 2 - energy_bar_w - margin;
    int main_h = height() - margin * 2 - 40;
    int spec_h = main_h * 2 / 5;
    int led_h = main_h / 5;
    int wave_h = main_h - spec_h - led_h - margin * 2;

    // 频谱区域
    QRect spec_rect(margin, margin, main_w, spec_h);
    draw_spectrum(painter, spec_rect);

    // LED网格预览区域
    QRect led_rect(margin, margin + spec_h + margin, main_w, led_h);
    draw_led_grid(painter, led_rect);

    // 波形区域
    QRect wave_rect(margin, margin + spec_h + margin + led_h + margin, main_w, wave_h);
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
    x += m_energy_gain_slider->width() + 10;

    // 波形点数
    m_wave_pts_label->move(x, ctrl_y + 3);
    x += m_wave_pts_label->width() + 3;
    m_wave_pts_slider->move(x, ctrl_y + 2);
    x += m_wave_pts_slider->width() + 10;

    // 频谱柱数
    m_bar_cnt_label->move(x, ctrl_y + 3);
    x += m_bar_cnt_label->width() + 3;
    m_bar_cnt_slider->move(x, ctrl_y + 2);
    x += m_bar_cnt_slider->width() + 10;

    // LED模式
    m_led_mode_label->move(x, ctrl_y + 3);
    x += m_led_mode_label->width() + 3;
    m_led_mode_combo->move(x, ctrl_y);
}

// 绘制频谱柱状图
void MusicRhythmWidget::draw_spectrum(QPainter &painter, const QRect &rect)
{
    int n = qMin(m_spectrum.size(), m_spectrum_peak.size());
    if (n == 0) return;

    float bar_w = (float)rect.width() / n;
    float gap = qMax(1.0f, bar_w * 0.15f);
    float draw_w = bar_w - gap;

#if 0
    // 频谱自动增益（复用LED的自动增益）
    float gain = m_spec_gain * m_auto_gain;
#else
    float gain = m_auto_gain;
#endif

    for (int i = 0; i < n; i++) {
        float val = m_spectrum[i] * gain;
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
        float peak_val = m_spectrum_peak[i] * gain;
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
    connect(m_spec_gain_slider, &QSlider::valueChanged, this, [this](uint32_t val) {
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
    connect(m_wave_gain_slider, &QSlider::valueChanged, this, [this](uint32_t val) {
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
    connect(m_energy_gain_slider, &QSlider::valueChanged, this, [this](uint32_t val) {
        m_energy_gain = (float)val;
        m_energy_gain_label->setText(QString("NRG: %1").arg(m_energy_gain, 0, 'f', 0));
    });

    // ---- 波形点数 ----
    m_wave_pts_label = new QLabel(this);
    m_wave_pts_label->setStyleSheet(label_style);
    m_wave_pts_label->setText(QString("WPTS: %1").arg(m_wave_pts));
    m_wave_pts_slider = new QSlider(Qt::Horizontal, this);
    m_wave_pts_slider->setRange(1024, 44100);
    m_wave_pts_slider->setSingleStep(1);
    m_wave_pts_slider->setPageStep(1);
    m_wave_pts_slider->setValue(m_wave_pts);
    m_wave_pts_slider->setFixedWidth(80);
    m_wave_pts_slider->setStyleSheet(slider_style);
    connect(m_wave_pts_slider, &QSlider::valueChanged, this, [this](uint32_t val) {
        m_wave_pts = val;
        m_wave_pts_label->setText(QString("WPTS: %1").arg(m_wave_pts));
        if (m_capture) m_capture->set_waveform_points(m_wave_pts);
    });

    // ---- 频谱柱数 ----
    m_bar_cnt_label = new QLabel(this);
    m_bar_cnt_label->setStyleSheet(label_style);
    m_bar_cnt_label->setText(QString("BARS: %1").arg(m_bar_cnt));
    m_bar_cnt_slider = new QSlider(Qt::Horizontal, this);
    m_bar_cnt_slider->setRange(16, 512);
    m_bar_cnt_slider->setSingleStep(1);
    m_bar_cnt_slider->setPageStep(1);
    m_bar_cnt_slider->setValue(m_bar_cnt);
    m_bar_cnt_slider->setFixedWidth(80);
    m_bar_cnt_slider->setStyleSheet(slider_style);
    connect(m_bar_cnt_slider, &QSlider::valueChanged, this, [this](uint32_t val) {
        m_bar_cnt = val;
        m_user_bar_cnt = val;
        m_bar_cnt_label->setText(QString("BARS: %1").arg(m_bar_cnt));
        if (m_capture && m_led_mode != LED_MODE_BANDS) m_capture->set_bar_count(m_bar_cnt);
    });

    // ---- LED模式选择 ----
    m_led_mode_label = new QLabel(this);
    m_led_mode_label->setStyleSheet(label_style);
    m_led_mode_label->setText("LED:");

    m_led_mode_combo = new QComboBox(this);
    m_led_mode_combo->setFixedWidth(100);
    m_led_mode_combo->setStyleSheet(combo_style);
    m_led_mode_combo->addItem("强度分层", LED_MODE_LAYERS);
    m_led_mode_combo->addItem("80频段", LED_MODE_BANDS);
    m_led_mode_combo->addItem("镜像波形", LED_MODE_MIRROR);
    connect(m_led_mode_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MusicRhythmWidget::on_led_mode_changed);
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
