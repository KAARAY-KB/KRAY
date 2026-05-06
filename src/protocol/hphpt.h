#ifndef HPHPT_H
#define HPHPT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ec.h"

#define HPHPT_SIZE                    ( 64u )

/* frame header */
#define HPHPT_FH_SIZE                 ( 9u )
#define HPHPT_FH_POS                  ( 0u )
#define HPHPT_FH_SOF_POS              ( HPHPT_FH_POS )
#define HPHPT_FH_SOF_SIZE             ( 1u )
#define HPHPT_FH_SOF                  ( 0x4A )
#define HPHPT_FH_SEQ_POS              ( HPHPT_FH_SOF_POS + HPHPT_FH_SOF_SIZE )
#define HPHPT_FH_SEQ_SIZE             ( 1u )
#define HPHPT_FH_RESERVER_POS         ( HPHPT_FH_SEQ_POS + HPHPT_FH_SEQ_SIZE )
#define HPHPT_FH_RESERVER_SIZE        ( 4u )
#define HPHPT_FH_PL_POS               ( HPHPT_FH_RESERVER_POS + HPHPT_FH_RESERVER_SIZE )
#define HPHPT_FH_PL_SIZE              ( 2u )
#define HPHPT_FH_CRC8_POS             ( HPHPT_FH_PL_POS + HPHPT_FH_PL_SIZE )
#define HPHPT_FH_CRC8_SIZE            ( 1u )
#define HPHPT_FH_END_POS              ( HPHPT_FH_CRC8_POS + HPHPT_FH_CRC8_SIZE )
#if                                 ((HPHPT_FH_END_POS - HPHPT_FH_POS) != HPHPT_FH_SIZE)
#error "frame header data offset calc error !!!"
#endif

/* payload header */
#define HPHPT_PH_SIZE                 ( 3u )
#define HPHPT_PH_POS                  ( HPHPT_FH_POS + HPHPT_FH_SIZE )
#define HPHPT_PH_CM_POS               ( HPHPT_PH_POS )
#define HPHPT_PH_CM_SIZE              ( 1u )
#define HPHPT_PH_CS_POS               ( HPHPT_PH_CM_POS + HPHPT_PH_CM_SIZE)
#define HPHPT_PH_CS_SIZE              ( 1u )
#define HPHPT_PH_PT_POS               ( HPHPT_PH_CS_POS + HPHPT_PH_CS_SIZE)
#define HPHPT_PH_PT_SIZE              ( 1u )
#define HPHPT_PH_END_POS              ( HPHPT_PH_PT_POS + HPHPT_PH_PT_SIZE )
#if                                 ((HPHPT_PH_END_POS - HPHPT_PH_POS) != HPHPT_PH_SIZE)
#error "payload header data offset calc error !!!"
#endif

/* payload n-byte */
#define HPHPT_PLD_POS                 ( HPHPT_PH_POS + HPHPT_PH_SIZE )

/* frame tail */
#define HPHPT_FT_SIZE                 ( 2u )
#define HPHPT_FT_POS(payload_size)    ( HPHPT_PLD_POS + payload_size )

/* frame message */
#define HPHPT_FS_MAX                  ( HPHPT_SIZE )
#define HPHPT_FS_MIN                  ( HPHPT_FH_SIZE + HPHPT_PH_SIZE + HPHPT_FT_SIZE )
#define HPHPT_PS_MAX                  ( HPHPT_FS_MAX - HPHPT_FS_MIN )
#if                                 ((HPHPT_FS_MIN + HPHPT_PS_MAX) != HPHPT_FS_MAX)
#error "frame size calc error !!!"
#endif

typedef ECType_t (*hpt_cb_t)(uint8_t *payload, uint8_t pt, uint16_t len);

typedef enum {
    HPHPT_PT_CHAR = 0,
	HPHPT_PT_INT8,
    HPHPT_PT_UINT8,
    HPHPT_PT_INT16,
    HPHPT_PT_UINT16,
    HPHPT_PT_INT32,
    HPHPT_PT_UINT32,
    HPHPT_PT_FLOAT,
    HPHPT_PT_DOUBLE,
	HPHPT_PT_MAX,
} hpt_pt_t;

#pragma pack(1)

typedef struct {
    uint8_t cm;
    uint8_t cs;
    hpt_cb_t cb;
} hpt_cmd_tab_t;

typedef struct {
    uint8_t cm;
    uint8_t cs;
    uint8_t ty; /* payload type */
} hpt_ph_t;

typedef struct {
    uint8_t sof;
    uint8_t seq;
    uint32_t rsv;
    uint16_t pl;
    uint8_t crc8;
} hpt_h_t;

typedef struct {
    uint16_t crc16;
} hpt_t_t;

typedef struct {
    hpt_pt_t ty;
    uint8_t size;
} hpt_pt_sz_t;

#pragma pack()

#ifdef __cplusplus
}
#endif

ECType_t hpt_decode(uint8_t *buffer, uint16_t len);

#endif // HPHPT_H


