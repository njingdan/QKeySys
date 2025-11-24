#include "modules/keypool/keypool.h"
#include <stdio.h>

// 密钥分发/补充执行期占位实现：
// 备注：
// - qks_keypool_update: 按传入的量子密钥字符串进行占位更新，当前仅返回成功；
// - qks_keypool_execute: 兼容旧接口（无类型约束），建议后续替换为 qks_keypool_update。
// 密钥池更新占位：传入量子密钥字符串，当前仅打印“已调用”。
int qks_keypool_update(const char* qk_str){ (void)qk_str; 
    printf("[KeyPool] qks_keypool_update 调用成功\n");
    return 0; 
}
// 兼容旧接口：当前未使用，仅打印“已调用”。
int qks_keypool_execute(void* payload){ (void)payload; 
    printf("[KeyPool] qks_keypool_execute 调用成功\n");
    return 0; 
}
