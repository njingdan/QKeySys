#ifndef KEY_MANAGER_H
#define KEY_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "KeyInfo.h"
#include "KeyConfig.h"
#include "GatewayComm.h"
#include <pthread.h> // 确保包含pthread头文件（关键）

// 密钥管理器结构体声明
typedef struct {
    KeyInfoList local_keys;        // 本地存储的所有密钥
    KeyConfig config;              // 当前运行配置
    pthread_mutex_t mutex;         // 互斥锁
    bool is_inited;                // 初始化标志
    pthread_t poll_thread;         // 轮询线程
    volatile bool is_running;      // 线程运行标志
} KeyManager;

// 核心接口声明
KeyManager* KeyManager_GetInstance();
bool KeyManager_Init(KeyManager* self);
void KeyManager_Stop(KeyManager* self);
bool KeyManager_GetValidKey(KeyManager* self, KeyInfo* out_key);
void KeyManager_CheckAllKeys(KeyManager* self);
void KeyManager_Reset(KeyManager* self);
KeyConfig KeyManager_GetConfig(const KeyManager* self);
void KeyManager_GetLocalKeys(const KeyManager* self, KeyInfoList* out_list);
void KeyManager_SimulateKeyExpiry(KeyManager* self, int number_of_keys_to_expire);
// 在 KeyManager.h 中添加
bool KeyManager_AddExternalKey(KeyManager* self, const char* key_value);

#endif // KEY_MANAGER_H