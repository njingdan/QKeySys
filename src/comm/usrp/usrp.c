#include "comm/usrp/usrp.h"
#include <stdio.h>

// USRP 占位实现：当前不进行真实通信，仅保留接口，返回成功或零数据。
// 发送 USRP 报文（占位）。
int qks_usrp_send(const uint8_t* data, size_t len){ (void)data; (void)len; return 0; }
// 接收 USRP 报文（占位）。
int qks_usrp_recv(uint8_t* buf, size_t cap, size_t* out_len){ (void)buf; (void)cap; if(out_len) *out_len=0; return 0; }

int qks_usrp_collect_info(const char* role_hint){
    printf("[USRP] 信息采集(%s) 已调用\n", role_hint?role_hint:"?");
    return 0;
}

int qks_usrp_file_confirm(void){
    printf("[USRP] 文件确认 已调用\n");
    return 0;
}
