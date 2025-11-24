#ifndef QKEYSYS_COMM_USRP_H
#define QKEYSYS_COMM_USRP_H

#include <stddef.h>
#include <stdint.h>

// USRP 通信占位接口：当前不做真实通信，仅保留函数原型。
int qks_usrp_send(const uint8_t* data, size_t len);
int qks_usrp_recv(uint8_t* buf, size_t cap, size_t* out_len);
// 设备认证相关占位：USRP 信息采集与文件确认（仅打印“已调用”）
int qks_usrp_collect_info(const char* role_hint);
int qks_usrp_file_confirm(void);

#endif /* QKEYSYS_COMM_USRP_H */
