#ifndef QKEYSYS_TYPES_H
#define QKEYSYS_TYPES_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    QKS_OK = 0,
    QKS_ERR = -1,
    QKS_TIMEOUT = -2,
    QKS_BUSY = -3,
    QKS_CANCELLED = -4
} qks_status_t;

typedef enum {
    QKS_ROLE_A = 1,  // 网关侧（唯一）
    QKS_ROLE_B = 2   // 储能柜侧（多个）
} qks_role_t;

#endif /* QKEYSYS_TYPES_H */

