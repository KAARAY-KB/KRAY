#include "MusicRhythmWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QIcon>
#include <QDebug>

#ifdef _WIN32
#include <Windows.h>
#endif

// 频谱柱数
static const int BAR_COUNT = 64;
// 刷新间隔（毫秒）
static const int REFRESH_MS = 33; // ~30fps

// 构造函数
MusicRhythmWidget::MusicRhythmWidget(QWidget *parent)
    : QWidget(parent)
    , m_capture(nullptr)
    , m_timer(nullptr)
    , m_energy(0.0f)
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

    // 启动采集
    m_capture->start();

    // 创建刷新定时器
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MusicRhythmWidget::on_refresh);
    m_timer->start(REFRESH_MS);
}

// 析构函数
MusicRhythmWidget::~MusicRhythmWidget()
{
    // 停止采集
    if (m_capture) {
        m_capture->stop();
    }
    // 停止定时器
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

// 定时刷新：更新峰值下落 + 重绘
void MusicRhythmWidget::on_refresh()
{
    // 更新频谱峰值下落效果
    for (int i = 0; i < BAR_COUNT && i < m_spectrum.size(); i++) {
        // 当前值超过峰值时，峰值跟随
        if (m_spectrum[i] >= m_spectrum_peak[i]) {
            m_spectrum_peak[i] = m_spectrum[i];
            m_spectrum_fall[i] = 0.0f; // 重置下落速度
        } else {
            // 峰值缓慢下落
            m_spectrum_fall[i] += 0.001f; // 加速度
            m_spectrum_peak[i] -= m_spectrum_fall[i];
            if (m_spectrum_peak[i] < 0.0f) {
                m_spectrum_peak[i] = 0.0f;
            }
        }
    }
    update(); // 触发重绘
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
    int main_h = height() - margin * 2;
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
}

// 绘制频谱柱状图
void MusicRhythmWidget::draw_spectrum(QPainter &painter, const QRect &rect)
{
    int n = qMin(m_spectrum.size(), m_spectrum_peak.size());
    if (n == 0) return;

    float bar_w = (float)rect.width() / n;
    float gap = qMax(1.0f, bar_w * 0.15f); // 柱间间距
    float draw_w = bar_w - gap;

    for (int i = 0; i < n; i++) {
        float val = m_spectrum[i];
        if (val < 0.0f) val = 0.0f;
        if (val > 1.0f) val = 1.0f;

        float bar_h = val * rect.height();
        float x = rect.x() + i * bar_w + gap / 2;
        float y = rect.y() + rect.height() - bar_h;

        // 渐变色：从底部蓝到顶部红
        QLinearGradient grad(x, rect.y() + rect.height(), x, rect.y());
        grad.setColorAt(0.0, QColor(0, 100, 255));   // 蓝
        grad.setColorAt(0.5, QColor(0, 255, 100));   // 绿
        grad.setColorAt(1.0, QColor(255, 50, 50));    // 红

        painter.setBrush(grad);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(QRectF(x, y, draw_w, bar_h), 1.0, 1.0);

        // 峰值指示条
        float peak_val = m_spectrum_peak[i];
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

    path.moveTo(rect.x(), mid_y - m_waveform[0] * rect.height() / 2.0f);
    for (int i = 1; i < n; i++) {
        float x = rect.x() + i * step;
        float y = mid_y - m_waveform[i] * rect.height() / 2.0f;
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
    float val = m_energy;
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

    // 渐变色
    QLinearGradient grad(bar_rect.x(), bar_rect.y() + bar_rect.height(),
                         bar_rect.x(), bar_rect.y());
    grad.setColorAt(0.0, QColor(0, 100, 255));
    grad.setColorAt(0.6, QColor(255, 200, 0));
    grad.setColorAt(1.0, QColor(255, 50, 50));

    painter.setPen(Qt::NoPen);
    painter.setBrush(grad);
    painter.drawRoundedRect(fill_rect, 2, 2);
}
