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
    , m_rhythm_mode_combo(nullptr)
    , m_rhythm_on(false)
    , m_music_ref(nullptr)
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

    // 律动模式选择下拉框
    m_rhythm_mode_combo = new QComboBox(this);
    m_rhythm_mode_combo->setFixedSize(90, 28);
    m_rhythm_mode_combo->move(75, 6);
    for (int i = 0; i < LED_FX_MAX; i++) {
        m_rhythm_mode_combo->addItem(LedEffect::mode_name((LedEffectMode)i));
    }
    m_rhythm_mode_combo->setStyleSheet(
        "QComboBox { background-color: #444; color: white; border-radius: 4px; padding: 2px 6px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background-color: #333; color: white; selection-background-color: #555; }"
    );
    connect(m_rhythm_mode_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GT64HeWidget::on_rhythm_mode_changed);

    // 保存按键当前背景色
    m_key_base_color.clear();
    ui->keyboard->layout->m_panel->getAllKeyNum([this](MKeyboardKey *key, void *user) {
        GT64HeWidget *self = (GT64HeWidget *)user;
        QColor bg = key->get_background_color();
        self->m_key_base_color.append(bg);
    }, this);
    // 初始化灯效
    m_led_effect.set_mode(LED_FX_COLOR_CYCLE);
    m_led_effect.set_base_color(QColor(0, 255, 0));

    // 设置灯效灯数
    m_led_effect.set_count(m_key_base_color.size());

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
                key->set_background_color(self->m_key_base_color[idx]);
                key->updateStyle();
            }
            idx++;
        }, this);
    }
}

// 律动模式切换
void GT64HeWidget::on_rhythm_mode_changed(int index)
{
    m_led_effect.set_mode((LedEffectMode)index);
}

// 接收LED网格数据，映射到键盘按键背景色
void GT64HeWidget::on_led_grid(const QVector<float> &data)
{
    if (!m_rhythm_on) return;

    int cols = 16;
    int rows = 5;
    int key_cnt = m_key_base_color.size();

    // 首帧初始化 prev 缓存
    if (m_key_prev_color.size() != key_cnt) {
        m_key_prev_color.resize(key_cnt);
        m_key_prev_color.fill(QColor(-1, -1, -1));
    }

    // 构建每个按键的亮度数组（从 LED grid 映射）
    QVector<float> brightness(key_cnt, 0.0f);
    int key_idx = 0;
    ui->keyboard->layout->m_panel->getAllKeyNum([this, &data, cols, rows, &brightness, &key_idx](MKeyboardKey *key, void *user) {
        // 用按键 id 查找对应的灯位（light_pos 索引 = key id）
        int id = key->getId();
        int x = device->light_pos[id].x;
        int y = device->light_pos[id].y;
        // 翻转y：键盘y=4(顶行)对应grid y=0
        int grid_y = rows - 1 - y;
        int grid_idx = grid_y * cols + x;

        if (grid_idx >= 0 && grid_idx < data.size()) {
            brightness[key_idx] = data[grid_idx];
        }
        key_idx++;
    }, this);

    // 通过灯效算法计算每个按键颜色
    QVector<QColor> colors = m_led_effect.next_frame(brightness, m_key_base_color);

    // 应用颜色到按键
    key_idx = 0;
    ui->keyboard->layout->m_panel->getAllKeyNum([this, &colors, &key_idx](MKeyboardKey *key, void *user) {
        GT64HeWidget *self = (GT64HeWidget *)user;

        if (key_idx < colors.size()) {
            QColor color = colors[key_idx];
            // 只在颜色变化时才更新样式
            if (color != self->m_key_prev_color[key_idx]) {
                self->m_key_prev_color[key_idx] = color;
                key->set_background_color(color);
                key->updateStyle();
            }
        }
        key_idx++;
    }, this);
}
