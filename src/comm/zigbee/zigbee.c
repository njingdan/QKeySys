#include "comm/zigbee/zigbee.h"
#include "qkeysys/os.h"
#include <stdio.h>

// Zigbee 收发占位实现：打印并短暂 sleep 模拟耗时。
// 发送 Zigbee 报文（占位）：仅打印长度与指针，sleep 模拟耗时。
int qks_zigbee_send(const uint8_t* data, size_t len){
    printf("[Zigbee] 发送: 长度=%zu 数据指针=%p\n", len, (const void*)data);
    qks_sleep_ms(30); // 模拟发送时间
    return 0;
}

// 接收 Zigbee 报文（占位）：打印“接收成功(占位)”，输出 0 长度，sleep 模拟耗时。
int qks_zigbee_recv(uint8_t* buf, size_t cap, size_t* out_len){
    (void)buf; (void)cap;
    qks_sleep_ms(60); // 模拟接收时间
    if(out_len) *out_len = 0; // 默认无数据（占位）
    // 打印一次占位信息，后续可在监听线程按需调用
    printf("[Zigbee] 接收: 成功(占位, 无数据)\n");
    return 0;
}
