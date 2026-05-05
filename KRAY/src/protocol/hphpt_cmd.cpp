#include "hphpt.h"


typedef enum {
    CM_DBG,
    CM_ATTR,
    CM_LAMP,
    CM_NOTIFY,
    CM_BLE,
    CM_RADIO,
    CM_MAX,
} cm_t;

typedef enum {
    DBG_TEST,
} dbg_t;
typedef enum {
    ATTR_TEST,
} attr_t;
typedef enum {
    LAMP_TEST,
} lamp_t;
typedef enum {
    NOTIFY_TEST,
} notify_t;
typedef enum {
    BLE_TEST,
} ble_t;
typedef enum {
    RADIO_TEST,
} radio_t;



static ECType_t dbg_test(uint8_t *payload, uint8_t pt, uint16_t len) {
    return EC_NONE;
}
static ECType_t attr_test(uint8_t *payload, uint8_t pt, uint16_t len) {
    return EC_NONE;
}
static ECType_t lamp_test(uint8_t *payload, uint8_t pt, uint16_t len) {
    return EC_NONE;
}
static ECType_t notify_test(uint8_t *payload, uint8_t pt, uint16_t len) {
    return EC_NONE;
}
static ECType_t ble_test(uint8_t *payload, uint8_t pt, uint16_t len) {
    return EC_NONE;
}
static ECType_t radio_test(uint8_t *payload, uint8_t pt, uint16_t len) {
    return EC_NONE;
}


static const hpt_cmd_tab_t hpt_cmd_tab[] = {
    { .cm = CM_DBG,             .cs = DBG_TEST,             .cb = dbg_test      },
    { .cm = CM_ATTR,            .cs = ATTR_TEST,            .cb = attr_test     },
    { .cm = CM_LAMP,            .cs = LAMP_TEST,            .cb = lamp_test     },
    { .cm = CM_NOTIFY,          .cs = NOTIFY_TEST,          .cb = notify_test   },
    { .cm = CM_BLE,             .cs = BLE_TEST,             .cb = ble_test      },
    { .cm = CM_RADIO,           .cs = RADIO_TEST,           .cb = radio_test    },
};
uint16_t hpt_get_kb_cmd_tab_len(void) {
    return (sizeof(hpt_cmd_tab) / sizeof(hpt_cmd_tab[0]));
}
const hpt_cmd_tab_t *hpt_get_kb_cmd_tab(void) {
    return hpt_cmd_tab;
}

