#ifndef QKEYSYS_COMM_ZIGBEE_H
#define QKEYSYS_COMM_ZIGBEE_H

#include <stddef.h>
#include <stdint.h>

// Zigbee 通信占位接口：当前不做真实通信，仅保留收发接口原型。
int qks_zigbee_send(const uint8_t* data, size_t len);
int qks_zigbee_recv(uint8_t* buf, size_t cap, size_t* out_len);

#endif /* QKEYSYS_COMM_ZIGBEE_H */
