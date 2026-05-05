#include "hphpt.h"
#include "utils.h"

#define EC_PACK_LEN         ( 1 )
#define EC_HEAD_SOF         ( 2 )
#define EC_HEAD_CRC         ( 3 )
#define EC_PLD_LEN          ( 4 )
#define EC_PACK_CRC         ( 5 )
#define EC_CMD_TAB_NULL     ( 6 )
#define EC_CMD_NOT_FIND     ( 7 )
#define EC_CMD_FUNC_RET     ( 8 )
#define EC_PACK_PLD_TY      ( 9 )
#define EC(co)              ( EC_SET(EC_MODULE_HPHPT, co) )


extern uint16_t hpt_get_kb_cmd_tab_len(void);
extern const hpt_cmd_tab_t *hpt_get_kb_cmd_tab(void);

static uint8_t frameBuffer[HPHPT_FS_MAX];
static const hpt_pt_sz_t hpt_pt_sz[HPHPT_PT_MAX] = {
    {HPHPT_PT_CHAR,   1},
    {HPHPT_PT_INT8,   1},
    {HPHPT_PT_UINT8,  1},
    {HPHPT_PT_INT16,  2},
    {HPHPT_PT_UINT16, 2},
    {HPHPT_PT_INT32,  4},
    {HPHPT_PT_UINT32, 4},
    {HPHPT_PT_FLOAT,  4},
    {HPHPT_PT_DOUBLE, 8},
};

uint8_t hpt_crc_head(uint8_t *buffer, uint16_t len) {
    return 0;
}
uint16_t hpt_crc_full(uint8_t *buffer, uint16_t len) {
    return 0;
}

ECType_t hpt_pld_decode(uint8_t *buffer, uint16_t len) {
    ECType_t ec = EC_NONE;
   return ec;
}
ECType_t hpt_decode(uint8_t *buffer, uint16_t len) {
    ECType_t ec     = EC_NONE;
    bool found      = false;
    
    if ((len < HPHPT_FS_MIN) || (len > HPHPT_FS_MAX)) { /* check packet length */
        return EC(EC_PACK_LEN);
    }

    hpt_h_t *head = (hpt_h_t *)&buffer[HPHPT_FH_POS];
    uint8_t calcHeadCrc = hpt_crc_head(&buffer[HPHPT_FH_POS], HPHPT_FH_SIZE);
    if (head->sof != HPHPT_FH_SOF) { /* check packet head */
        return EC(EC_HEAD_SOF);
    }
    if (head->pl > HPHPT_PS_MAX) { /* check packet payload length */
        return EC(EC_PLD_LEN);
    }
    if (calcHeadCrc != head->crc8) { /* check packet head crc8 */
        return EC(EC_HEAD_CRC);
    }

    hpt_ph_t *payloadHead = (hpt_ph_t *)&buffer[HPHPT_PH_POS];
    if (payloadHead->ty >= HPHPT_PT_MAX) { /* check packet payload type */
        return EC(EC_PACK_PLD_TY);
    }

    uint16_t calcFullCrc = hpt_crc_full(&buffer[HPHPT_FH_POS], HPHPT_FH_SIZE + head->pl);
    uint16_t crcFullPack = (*(uint16_t *)&buffer[HPHPT_FT_POS(head->pl)]);
    if (calcFullCrc != crcFullPack) { /* check packet tail crc16 */
        return EC(EC_PACK_CRC);
    }
    
    uint8_t *payload = &buffer[HPHPT_PLD_POS];
    const hpt_cmd_tab_t *cmd_tab = hpt_get_kb_cmd_tab();
    const uint16_t cmd_tab_len = hpt_get_kb_cmd_tab_len();
    for (uint16_t i = 0; i < cmd_tab_len; i++) {
        if ((cmd_tab[i].cm == payloadHead->cm) && (cmd_tab[i].cs == payloadHead->cs) && (cmd_tab[i].cb != NULL)) {
            if (cmd_tab[i].cb(payload, payloadHead->ty, head->pl) != EC_NONE) {
                ec = EC(EC_CMD_FUNC_RET);
            }
            found = true;
            break;
        }
    }

    if (found == false) {
        ec = EC(EC_CMD_NOT_FIND);
    }
    return ec;
}

ECType_t hpt_encode(uint8_t *buffer, uint16_t lenght) {
    if (lenght > HPHPT_PS_MAX) { /* packet payload length */
        return EC(EC_PLD_LEN);
    }

    return EC(EC_PACK_LEN);
}


