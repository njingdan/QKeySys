#include "GatewayComm.h"
#include <stdlib.h> // malloc、realloc、free
#include <string.h> // strncpy
#include <stddef.h> // NULL
#include <stdio.h>  // snprintf
#include <windows.h> // Sleep（Windows标准睡眠函数）
#include "NTPClient.h"

// ------------------------------
// 动态数组KeyInfoList的实现（仅在GatewayComm.c中实现，其他文件通过头文件调用）
// ------------------------------
void KeyInfoList_Init(KeyInfoList* list) {
    if (list == NULL) return;
    list->data = NULL;
    list->size = 0;
    list->capacity = 0;
}

int KeyInfoList_Add(KeyInfoList* list, const KeyInfo* key) {
    if (list == NULL || key == NULL) return -1;

    if (list->size >= list->capacity) {
        int new_cap = (list->capacity == 0) ? 4 : list->capacity * 2;
        KeyInfo* new_data = (KeyInfo*)realloc(list->data, new_cap * sizeof(KeyInfo));
        if (new_data == NULL) return -1;
        list->data = new_data;
        list->capacity = new_cap;
    }

    list->data[list->size] = *key;
    list->size++;
    return 0;
}

void KeyInfoList_Free(KeyInfoList* list) {
    if (list == NULL) return;
    if (list->data != NULL) {
        free(list->data);
        list->data = NULL;
    }
    list->size = 0;
    list->capacity = 0;
}

// ------------------------------
// 网关通信接口实现
// ------------------------------
// 向网关请求新密钥（模拟）
int GatewayComm_RequestNewKeys(int need_count, const KeyConfig* config, KeyInfoList* out_keys) {
    if (need_count <= 0 || config == NULL || out_keys == NULL) {
        return -1;
    }

    // 模拟网关返回密钥：生成need_count个测试密钥（ID改为时间戳+序号）
    for (int i = 0; i < need_count; i++) {
        char key_id[KEY_ID_MAX_LEN];  // KEY_ID_MAX_LEN是KeyInfo.h中定义的，确保足够长（建议≥32）
        char key_value[KEY_VALUE_MAX_LEN];
        
        // 核心修改：密钥ID = 当前时间戳（秒级） + 序号（避免同一时间生成多个密钥导致ID重复）
        int64_t current_ts = NTPClient_GetCurrentTime();  // 获取当前模拟时间戳
        snprintf(key_id, KEY_ID_MAX_LEN, "key_%lld_%d", (long long)current_ts, i + 1);  // 格式：key_1764599175_1
        
        // 密钥值也可以关联时间戳（可选，保持唯一）
        snprintf(key_value, KEY_VALUE_MAX_LEN, "value_%lld_%d_abc123", (long long)current_ts, i + 1);

        // 初始化密钥（创建时间就是当前时间戳）
        KeyInfo key;
        KeyInfo_Init(&key, key_id, key_value, current_ts);

        // 添加到输出列表
        if (KeyInfoList_Add(out_keys, &key) != 0) {
            KeyInfoList_Free(out_keys);
            return -1;
        }
    }

    return 0;
}

bool GatewayComm_SendEncryptedData(const char* data, int data_len, const char* key_id) {
    if (data == NULL || data_len <= 0 || key_id == NULL) return false;
    // 模拟发送成功
    return true;
}

KeyConfig GatewayComm_SyncConfig() {
    // 模拟同步配置
    return DEFAULT_CONFIG;
}

// ------------------------------
// 内部辅助函数（仅当前文件可见）
// ------------------------------
bool GatewayComm_RetryRequest(int max_retry, int interval) {
    if (max_retry <= 0 || interval <= 0) return false;

    for (int i = 0; i < max_retry; i++) {
        if (i == max_retry - 1) return true;
        Sleep(interval * 1000); // 替换_sleep为Sleep（无警告）
    }
    return false;
}