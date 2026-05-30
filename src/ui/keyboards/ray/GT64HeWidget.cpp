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
    , m_rhythm_check_combo(nullptr)
    , m_rhythm_on(false)
    , m_rhythm_check_mode(RHYTHM_CHECK_SKIP)
    , m_music_ref(nullptr)
{
    ui->setupUi(this);

    // 创建并打开 GT-64HE 设备
    device = new GT64HeDevice(info);
    if (device->open()) {
        Console::info("GT64HeWidget") << "device opened" << std::endl;
    }
    else {
        Console::error("GT64HeWidget") << "device open failed" << std::endl;
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

    // 律动选中模式选择下拉框
    m_rhythm_check_combo = new QComboBox(this);
    m_rhythm_check_combo->setFixedSize(110, 28);
    m_rhythm_check_combo->move(170, 6);
    m_rhythm_check_combo->addItem("选中不跳");
    m_rhythm_check_combo->addItem("选中也跳");
    m_rhythm_check_combo->setStyleSheet(
        "QComboBox { background-color: #444; color: white; border-radius: 4px; padding: 2px 6px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background-color: #333; color: white; selection-background-color: #555; }"
    );
    connect(m_rhythm_check_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GT64HeWidget::on_rhythm_check_mode_changed);

    // 保存按键当前背景色，并从设备映射表读取每个按键的LED灯数量
    m_key_base_color.clear();
    m_key_light_count.clear();
    m_total_led_count = 0;
    ui->keyboard->layout->m_panel->getAllKeyNum([this](MKeyboardKey *key, void *user) {
        GT64HeWidget *self = (GT64HeWidget *)user;
        QColor bg = key->get_background_color();
        self->m_key_base_color.append(bg);
        // 从设备 light_msg 读取LED数量
        int id = key->getId();
        int light_cnt = (id >= 0 && id < 61) ? self->device->light_msg[id].num : 1;
        self->m_key_light_count.append(light_cnt);
        self->m_total_led_count += light_cnt;
        // 多LED按键设置分段数
        if (light_cnt > 1) {
            key->set_light_count(light_cnt);
        }
    }, this);
    // 初始化灯效
    m_led_effect.set_mode(LED_FX_COLOR_CYCLE);
    m_led_effect.set_base_color(QColor(0, 255, 0));

    // 设置灯效灯数为总LED数（包含多LED按键的额外LED）
    m_led_effect.set_count(m_total_led_count);

    m_activeWindowTimer->start(50);
}

GT64HeWidget::~GT64HeWidget() {
    if (m_activeWindow) {
        emit activeWindowChanged(false);
    }
    if (m_activeWindowTimer) {
        Console::info("GT64HeWidget") << "m_activeWindowTimer stopped" << std::endl;
        m_activeWindowTimer->stop();
        delete m_activeWindowTimer;
    }
    if (device) {
        Console::info("GT64HeWidget") << "device closed" << std::endl;
        device->close();
        delete device;
    }
    delete ui;
}

void GT64HeWidget::closeEvent(QCloseEvent *event) {
    Console::info("GT64HeWidget") << "closeEvent" << std::endl;
    event->accept();
    emit exitWindow();
    QWidget::closeEvent(event);
}
void GT64HeWidget::closeWidget(void) {
    Console::info("GT64HeWidget") << "closeWidget" << std::endl;
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
    Console::info("GT64HeWidget") << str << std::endl;
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
                Console::info("GT64HeWidget") << "rhythm connected" << std::endl;
                // 标记所有按键进入律动模式，多LED按键重新设置分段数
                int idx2 = 0;
                ui->keyboard->layout->m_panel->getAllKeyNum([this, &idx2](MKeyboardKey *key, void *) {
                    key->set_rhythm_active(true);
                    int light_cnt = (idx2 < m_key_light_count.size()) ? m_key_light_count[idx2] : 1;
                    if (light_cnt > 1) {
                        key->set_light_count(light_cnt);
                    }
                    idx2++;
                }, nullptr);
                return;
            }
        }
        // 没找到，关闭律动
        Console::warn("GT64HeWidget") << "no MusicRhythmWidget found" << std::endl;
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
                // 退出律动模式
                key->set_rhythm_active(false);
                // 多LED按键清除段颜色，恢复单色模式
                if (self->m_key_light_count[idx] > 1) {
                    key->clear_segment_colors();
                }
                key->set_background_color(self->m_key_base_color[idx]);
                key->updateStyle();
            }
            idx++;
        }, this);
    }
}

// 律动灯效模式切换
void GT64HeWidget::on_rhythm_mode_changed(int index)
{
    m_led_effect.set_mode((LedEffectMode)index);
}

// 律动选中模式切换
void GT64HeWidget::on_rhythm_check_mode_changed(int index)
{
    m_rhythm_check_mode = (RhythmCheckMode)index;
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

    // 构建每颗LED的亮度数组和底色数组（多LED按键有多颗LED）
    QVector<float> led_brightness(m_total_led_count, 0.0f);
    QVector<QColor> led_base_colors(m_total_led_count);

    int led_offset = 0;
    int key_idx = 0;
    ui->keyboard->layout->m_panel->getAllKeyNum([this, &data, cols, rows, &led_brightness, &led_base_colors, &led_offset, &key_idx](MKeyboardKey *key, void *user) {
        // 用按键 id 从 light_msg 读取LED位置
        int id = key->getId();
        int light_cnt = m_key_light_count[key_idx];

        // 为每颗LED从 light_msg[id].pos 读取xy位置，映射到网格亮度
        for (int seg = 0; seg < light_cnt; seg++) {
            int x = device->light_msg[id].pos[seg].x;
            int y = device->light_msg[id].pos[seg].y;
            // 翻转y：键盘y=4(顶行)对应grid y=0
            int grid_y = rows - 1 - y;
            int grid_idx = grid_y * cols + x;

            if (grid_idx >= 0 && grid_idx < data.size()) {
                led_brightness[led_offset + seg] = data[grid_idx];
            }
            led_base_colors[led_offset + seg] = m_key_base_color[key_idx];
        }
        led_offset += light_cnt;
        key_idx++;
    }, this);

    // 通过灯效算法计算每颗LED的颜色
    QVector<QColor> led_colors = m_led_effect.next_frame(led_brightness, led_base_colors);

    // 应用颜色到按键
    led_offset = 0;
    key_idx = 0;
    ui->keyboard->layout->m_panel->getAllKeyNum([this, &led_colors, &led_offset, &key_idx](MKeyboardKey *key, void *user) {
        GT64HeWidget *self = (GT64HeWidget *)user;
        int light_cnt = self->m_key_light_count[key_idx];

        // 模式1：选中按键律动不跳，保持选中色
        if (self->m_rhythm_check_mode == RHYTHM_CHECK_SKIP && key->isChecked()) {
            led_offset += light_cnt;
            key_idx++;
            return;
        }

        if (light_cnt > 1) {
            // 多LED按键：设置多段颜色
            bool changed = false;
            for (int seg = 0; seg < light_cnt; seg++) {
                QColor color = led_colors[led_offset + seg];
                if (color != key->get_segment_color(seg)) {
                    changed = true;
                }
            }
            if (changed) {
                for (int seg = 0; seg < light_cnt; seg++) {
                    key->set_segment_color(seg, led_colors[led_offset + seg]);
                }
                key->updateStyle();
            }
        } else {
            // 单LED按键：设置单一背景色
            QColor color = led_colors[led_offset];
            if (color != self->m_key_prev_color[key_idx]) {
                self->m_key_prev_color[key_idx] = color;
                key->set_background_color(color);
                key->updateStyle();
            }
        }

        led_offset += light_cnt;
        key_idx++;
    }, this);
}
