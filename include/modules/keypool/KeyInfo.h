#ifndef KEY_INFO_H
#define KEY_INFO_H

#include <stdint.h>  // int64_t
#include "KeyConfig.h"

// 字符串长度配置（宏定义声明，全局可见）
#define KEY_ID_MAX_LEN  64  // 密钥ID最大长度
#define KEY_VALUE_MAX_LEN 128 // 密钥值最大长度

// 密钥状态枚举声明
typedef enum {
    KEY_STATUS_VALID,    // 有效（可加密新数据）
    KEY_STATUS_FROZEN,   // 冷冻（不可加密，可解密）
    KEY_STATUS_DELETED   // 已删除（无效）
} KeyStatus;

// 密钥信息结构体声明
typedef struct {
    char key_id[KEY_ID_MAX_LEN];    // 密钥唯一ID（C风格字符串）
    char key_value[KEY_VALUE_MAX_LEN]; // 密钥值（C风格字符串）
    int64_t create_ts;              // 生成时间戳（秒级，NTP同步后）
    KeyStatus status;               // 当前状态（枚举类型）
} KeyInfo;

// 工具函数声明（无实现）
// 计算密钥过期时间
int64_t KeyInfo_GetExpireTime(const KeyInfo* key, const KeyConfig* config);

// 计算密钥删除时间
int64_t KeyInfo_GetDeleteTime(const KeyInfo* key, const KeyConfig* config);

// 初始化密钥结构体
void KeyInfo_Init(KeyInfo* key, const char* key_id, const char* key_value, int64_t create_ts);

#endif // KEY_INFO_H