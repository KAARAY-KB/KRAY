#include "GT64HeWidget.h"
#include "Ui_GT64HeWidget.h"
#include "hphpt.h"
#include "console.h"
#include <QDebug>
#include <QApplication>


GT64HeWidget::GT64HeWidget(UsbDeviceInfo info, QWidget *parent) 
    : QWidget(parent)
    , ui(new Ui::GT64HeWidget)
    , m_rhythm_btn(nullptr)
    , m_rhythm_on(false)
    , m_music_ref(nullptr)
    , m_key_rhythm_color(0, 255, 0) // 律动色
{
    ui->setupUi(this);

    // 创建并打开 GT-64HE 设备
    device = new GT64HeDevice(info);
    if (device->open()) {
        Console::out() << "GT64HeWidget: device opened" << std::endl;
    } else {
        Console::out() << "GT64HeWidget: device open failed" << std::endl;
    }

    connect(this, &GT64HeWidget::activeWindowChanged, this, &GT64HeWidget::slot_activeWindowChanged);
    m_activeWindowTimer = new QTimer(this);
    connect(m_activeWindowTimer, &QTimer::timeout, this, [this](){
        if (m_activeWindow != this->isActiveWindow()) {
            m_activeWindow = this->isActiveWindow();
            emit activeWindowChanged(m_activeWindow);
        }
        if (m_activeWindowTimer->interval() != 1000) {
            m_activeWindowTimer->setInterval(1000);
        }
    });
    connect(ui->keyboard->layout, &MKeyboardLayout::keyClicked, this, &GT64HeWidget::slot_keyboard_key_clicked);

    // 律动开关按钮
    m_rhythm_btn = new QPushButton("律动", this);
    m_rhythm_btn->setCheckable(true);
    m_rhythm_btn->setFixedSize(60, 30);
    m_rhythm_btn->move(10, 5);
    m_rhythm_btn->setStyleSheet(
        "QPushButton { background-color: #555; color: white; border-radius: 4px; }"
        "QPushButton:checked { background-color: #e74c3c; }"
    );
    connect(m_rhythm_btn, &QPushButton::clicked, this, &GT64HeWidget::on_rhythm_btn_clicked);

    m_key_rhythm_color = QColor(0, 255, 0, 128);     // 律动色
        // 保存按键当前背景色（开启律动前快照）
        m_key_base_color.clear();
        ui->keyboard->layout->m_panel->getAllKeyNum([this](MKeyboardKey *key, void *user) {
            GT64HeWidget *self = (GT64HeWidget *)user;
            QColor bg = key->get_dft_background_color();
            self->m_key_base_color.append(bg);
        }, this);

    m_activeWindowTimer->start(50);
}

GT64HeWidget::~GT64HeWidget() {
    if (m_activeWindow) {
        emit activeWindowChanged(false);
    }
    if (m_activeWindowTimer) {
        Console::out() << "GT64HeWidget::~m_activeWindowTimer" << std::endl;
        m_activeWindowTimer->stop();
        delete m_activeWindowTimer;
    }
    if (device) {
        Console::out() << "GT64HeWidget::~device" << std::endl;
        device->close();
        delete device;
    }
    delete ui;
}

void GT64HeWidget::closeEvent(QCloseEvent *event) {
    Console::out() << "GT64HeWidget: close event" << std::endl;
    event->accept();
    emit exitWindow();
    QWidget::closeEvent(event);
}
void GT64HeWidget::closeWidget(void) {
    Console::out() << "GT64HeWidget: close widget" << std::endl;
    emit exitWidget();
}

void GT64HeWidget::updateParamValue() {
    double val = 2.0;
    ui->param->property->spinBox_param[0]->setValue(val);
}

// 读取完成回调
void GT64HeWidget::on_read_done(const std::vector<uint8_t>& data) {
    std::string str = "usb rx[" + std::to_string(data.size()) + "]: ";
    for (size_t i = 0; i < 10 && i < data.size(); ++i) {
        str += std::to_string(data[i]) + ",";
    }
    if (!data.empty()) str.pop_back();
    Console::out() << str << std::endl;
}

// 律动开关按钮点击
void GT64HeWidget::on_rhythm_btn_clicked()
{
    m_rhythm_on = m_rhythm_btn->isChecked();

    if (m_rhythm_on) {
        // 查找已打开的音乐律动窗口
        foreach (QWidget *w, QApplication::topLevelWidgets()) {
            MusicRhythmWidget *mw = qobject_cast<MusicRhythmWidget*>(w);
            if (mw) {
                m_music_ref = mw;
                connect(m_music_ref, &MusicRhythmWidget::sig_led_grid,
                        this, &GT64HeWidget::on_led_grid);
                Console::out() << "GT64HeWidget: rhythm connected" << std::endl;
                return;
            }
        }
        // 没找到，关闭律动
        Console::out() << "GT64HeWidget: no MusicRhythmWidget found" << std::endl;
        m_rhythm_on = false;
        m_rhythm_btn->setChecked(false);
    } else {
        // 断开连接，恢复按键原始颜色
        if (m_music_ref) {
            disconnect(m_music_ref, &MusicRhythmWidget::sig_led_grid,
                       this, &GT64HeWidget::on_led_grid);
            m_music_ref = nullptr;
        }
        // 恢复原始背景色
        int idx = 0;
        ui->keyboard->layout->m_panel->getAllKeyNum([this, &idx](MKeyboardKey *key, void *user) {
            GT64HeWidget *self = (GT64HeWidget *)user;
            if (idx < self->m_key_base_color.size()) {
                key->set_dft_background_color(self->m_key_base_color[idx]);
                key->updateStyle();
            }
            idx++;
        }, this);
    }
}

// 根据亮度在按键原始底色和律动颜色之间插值，brightness: 0~100
QColor GT64HeWidget::blend_color(const QColor base_color, const QColor rhythm_color, float brightness)
{
    int t = qBound(0, (int)brightness, 100);
    int r = base_color.red()   + (rhythm_color.red()   - base_color.red())   * t / 100;
    int g = base_color.green() + (rhythm_color.green() - base_color.green()) * t / 100;
    int b = base_color.blue()  + (rhythm_color.blue()  - base_color.blue())  * t / 100;
    return QColor(r, g, b);
}

// 接收LED网格数据，映射到键盘按键背景色
void GT64HeWidget::on_led_grid(const QVector<float> &data)
{
    if (!m_rhythm_on) return;

    int cols = 16;
    int rows = 5;
    int key_idx = 0;

    // 首帧初始化 prev 缓存
    if (m_key_prev_color.size() != m_key_base_color.size()) {
        m_key_prev_color.resize(m_key_base_color.size());
        m_key_prev_color.fill(QColor(-1, -1, -1));
    }

    ui->keyboard->layout->m_panel->getAllKeyNum([this, &data, cols, rows, &key_idx](MKeyboardKey *key, void *user) {
        GT64HeWidget *self = (GT64HeWidget *)user;

        // 用按键 id 查找对应的灯位（light_pos 索引 = key id）
        int id = key->getId();
        int x = self->device->light_pos[id].x;
        int y = self->device->light_pos[id].y;
        // 翻转y：键盘y=4(顶行)对应grid y=0
        int grid_y = rows - 1 - y;
        int grid_idx = grid_y * cols + x;

        float brightness = 0.0f;
        if (grid_idx >= 0 && grid_idx < data.size()) {
            brightness = data[grid_idx];
        }

        // 按键原始底色→律动颜色插值
        QColor color = self->blend_color(self->m_key_base_color[key_idx], self->m_key_rhythm_color, brightness);

        // 只在颜色变化时才更新样式（避免每帧1830次setStyleSheet）
        if (color != self->m_key_prev_color[key_idx]) {
            self->m_key_prev_color[key_idx] = color;
            key->set_dft_background_color(color);
            key->updateStyle();
        }

        key_idx++;
    }, this);
}
