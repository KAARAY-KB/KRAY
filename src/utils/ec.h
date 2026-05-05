#ifndef ERROR_CODE_H
#define ERROR_CODE_H
#ifdef __cplusplus
 extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

typedef uint32_t ECType_t;

// ECType_t co  : 12; //0bit
// ECType_t id  : 10;
// ECType_t rsv : 10;
#define EC_NONE                 ( 0u )
#define EC_SET(id, co)          ( (ECType_t)((((ECType_t)(0) & 0x03FFu) << 22u) | (((ECType_t)(id) & 0x03FFu) << 12u) | (((ECType_t)(co) & 0x0FFFu))) )
#define EC_GET_CO(co)           ( (ECType_t)(((ECType_t)(co)  >> 0u)  & 0x0FFFu) )
#define EC_GET_ID(id)           ( (ECType_t)(((ECType_t)(id)  >> 12u) & 0x03FFu) )
#define EC_GET_RSV(rsv)         ( (ECType_t)(((ECType_t)(rsv) >> 22u) & 0x03FFu) )
#define EC_GET_MODULE(id)       ( EC_GET_ID(id) )

typedef enum {
    EC_MODULE_HPHPT = 0,
    EC_MODULE_HPHPT_CMD,
    EC_MODULE_MAX,
} ec_module_t;

#ifdef __cplusplus
}
#endif
#endif  // ERROR_CODE_H



