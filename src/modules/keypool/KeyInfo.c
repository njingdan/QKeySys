#include "KeyInfo.h"
#include <string.h> // 用于strncpy
#include <stddef.h> // 用于NULL定义

// 计算密钥过期时间（实现）
int64_t KeyInfo_GetExpireTime(const KeyInfo* key, const KeyConfig* config) {
    if (key == NULL || config == NULL) {
        return 0;
    }
    // 生成时间 + 有效期 = 过期时间
    return key->create_ts + config->key_validity;
}

// 计算密钥删除时间（实现）
int64_t KeyInfo_GetDeleteTime(const KeyInfo* key, const KeyConfig* config) {
    if (key == NULL || config == NULL) {
        return 0;
    }
    // 过期时间 + 冷冻期 = 删除时间（复用已有函数，减少冗余）
    return KeyInfo_GetExpireTime(key, config) + KeyConfig_GetFreezeDuration(config);
}

// 初始化密钥结构体（实现）
void KeyInfo_Init(KeyInfo* key, const char* key_id, const char* key_value, int64_t create_ts) {
    if (key == NULL || key_id == NULL || key_value == NULL) {
        return;
    }

    // 安全拷贝字符串：用strncpy避免缓冲区溢出，手动添加字符串结束符
    strncpy(key->key_id, key_id, KEY_ID_MAX_LEN - 1);
    key->key_id[KEY_ID_MAX_LEN - 1] = '\0'; // 确保字符串终止

    strncpy(key->key_value, key_value, KEY_VALUE_MAX_LEN - 1);
    key->key_value[KEY_VALUE_MAX_LEN - 1] = '\0';

    key->create_ts = create_ts;
    key->status = KEY_STATUS_VALID; // 初始状态默认为“有效”
}