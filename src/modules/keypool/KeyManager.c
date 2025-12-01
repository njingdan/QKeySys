#include <stdint.h>  // 新增：确保int64_t类型被定义
#include "NTPClient.h" // 移到最前面，优先包含
#include "KeyManager.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <windows.h>


// ------------------------------
// 内部辅助函数声明（先声明，后使用）
// ------------------------------
static void* KeyManager_PollThreadFunc(void* arg);
static void KeyManager_CheckAndRequestKeys(KeyManager* self);
static void KeyManager_UpdateKeyStatus(KeyInfo* key, const KeyConfig* config);
static void KeyManager_DeleteExpiredKeys(KeyManager* self);
// 在文件开头（内部函数声明区域）添加这个声明
static void KeyManager_InitSingleton(KeyManager* instance);

// 全局变量
static KeyManager g_keymanager_instance;
static pthread_once_t g_once_control = PTHREAD_ONCE_INIT;

// 无参数初始化函数
static void KeyManager_InitInstance(void) {
    memset(&g_keymanager_instance, 0, sizeof(KeyManager));
    
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&g_keymanager_instance.mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    
    g_keymanager_instance.is_inited = false;
    g_keymanager_instance.is_running = false;
}

KeyManager* KeyManager_GetInstance() {
    pthread_once(&g_once_control, KeyManager_InitInstance);
    return &g_keymanager_instance;
}

// ------------------------------
// 核心接口实现
// ------------------------------
// KeyManager.c - 修复 GetInstance


bool KeyManager_Init(KeyManager* self) {
    if (self == NULL || self->is_inited) return false;

    // 初始化互斥锁
    // if (pthread_mutex_init(&self->mutex, NULL) != 0) return false;

    // 同步配置
    self->config = GatewayComm_SyncConfig();

    // 初始化密钥列表
    KeyInfoList_Init(&self->local_keys);

    // 初始请求密钥
    KeyManager_CheckAndRequestKeys(self);

    // 启动轮询线程
    self->is_running = true;
    if (pthread_create(&self->poll_thread, NULL, KeyManager_PollThreadFunc, self) != 0) {
        // 线程创建失败，回滚资源
        self->is_running = false;
        // pthread_mutex_destroy(&self->mutex);
        KeyInfoList_Free(&self->local_keys);
        return false;
    }

    self->is_inited = true;
    return true;
}

void KeyManager_Stop(KeyManager* self) {
    if (self == NULL || !self->is_inited) return;

    // 通知线程退出
    self->is_running = false;

    // 等待线程结束
    pthread_join(self->poll_thread, NULL);

    // 释放资源
    // pthread_mutex_destroy(&self->mutex);
    KeyInfoList_Free(&self->local_keys);
    self->is_inited = false;
}

bool KeyManager_GetValidKey(KeyManager* self, KeyInfo* out_key) {
    if (self == NULL || out_key == NULL || !self->is_inited) return false;

    pthread_mutex_lock(&self->mutex);

    bool found = false;
    for (int i = 0; i < self->local_keys.size; i++) {
        if (self->local_keys.data[i].status == KEY_STATUS_VALID) {
            *out_key = self->local_keys.data[i];
            found = true;
            break;
        }
    }

    pthread_mutex_unlock(&self->mutex);
    return found;
}

void KeyManager_CheckAllKeys(KeyManager* self) {
    if (self == NULL || !self->is_inited) return;

    pthread_mutex_lock(&self->mutex);

    // 更新所有密钥状态
    for (int i = 0; i < self->local_keys.size; i++) {
        KeyManager_UpdateKeyStatus(&self->local_keys.data[i], &self->config);
    }

    // 删除过期密钥
    KeyManager_DeleteExpiredKeys(self);

    // 检查并请求新密钥
    KeyManager_CheckAndRequestKeys(self);

    pthread_mutex_unlock(&self->mutex);
}

void KeyManager_Reset(KeyManager* self) {
    if (self == NULL) return;

    pthread_mutex_lock(&self->mutex);
    KeyInfoList_Free(&self->local_keys);
    KeyInfoList_Init(&self->local_keys);
    self->config = DEFAULT_CONFIG;
    pthread_mutex_unlock(&self->mutex);
}

KeyConfig KeyManager_GetConfig(const KeyManager* self) {
    KeyConfig config = {0};
    if (self != NULL && self->is_inited) {
        config = self->config;
    }
    return config;
}

void KeyManager_GetLocalKeys(const KeyManager* self, KeyInfoList* out_list) {
    if (self == NULL || out_list == NULL || !self->is_inited) return;

    KeyInfoList_Init(out_list);
    pthread_mutex_lock(&((KeyManager*)self)->mutex);

    for (int i = 0; i < self->local_keys.size; i++) {
        KeyInfoList_Add(out_list, &self->local_keys.data[i]);
    }

    pthread_mutex_unlock(&((KeyManager*)self)->mutex);
}

void KeyManager_SimulateKeyExpiry(KeyManager* self, int number_of_keys_to_expire) {
    if (self == NULL || number_of_keys_to_expire <= 0 || !self->is_inited) return;

    pthread_mutex_lock(&self->mutex);

    int expired_count = 0;
    for (int i = 0; i < self->local_keys.size && expired_count < number_of_keys_to_expire; i++) {
        if (self->local_keys.data[i].status == KEY_STATUS_VALID) {
            self->local_keys.data[i].status = KEY_STATUS_DELETED;
            expired_count++;
        }
    }

    pthread_mutex_unlock(&self->mutex);
}

// ------------------------------
// 内部辅助函数实现
// ------------------------------
static void* KeyManager_PollThreadFunc(void* arg) {
    KeyManager* self = (KeyManager*)arg;
    if (self == NULL) return NULL;

    while (self->is_running) {
        int poll_cycle = KeyConfig_GetPollCycle(&self->config);
        if (poll_cycle <= 0) poll_cycle = 1;

        // 原代码：一次性睡眠 poll_cycle 秒，期间无法响应退出
        // Sleep(poll_cycle * 1000);
        
        // 修改后：分段睡眠，每 100ms 检查一次 is_running
        for (int i = 0; i < poll_cycle * 10 && self->is_running; i++) {
            Sleep(100);  // 每次睡 100ms，总共 poll_cycle 秒
        }

        KeyManager_CheckAllKeys(self);
    }

    return NULL;
}

static void KeyManager_CheckAndRequestKeys(KeyManager* self) {
    if (self == NULL) return;

    // 统计有效密钥数量
    int valid_count = 0;
    for (int i = 0; i < self->local_keys.size; i++) {
        if (self->local_keys.data[i].status == KEY_STATUS_VALID) {
            valid_count++;
        }
    }

    // 不足阈值，请求新密钥
    if (valid_count < self->config.valid_threshold) {
        int need_count = self->config.valid_threshold - valid_count;
        KeyInfoList new_keys;
        KeyInfoList_Init(&new_keys);

        // 原代码：GatewayComm_RetryRequest(3, 1) 会循环 3 次，每次睡 1 秒
        // if (GatewayComm_RetryRequest(3, 1) &&
        //     GatewayComm_RequestNewKeys(need_count, &self->config, &new_keys) == 0) {

        // 修改后：直接请求新密钥，去掉重试阻塞
        if (GatewayComm_RequestNewKeys(need_count, &self->config, &new_keys) == 0) {
            for (int i = 0; i < new_keys.size; i++) {
                KeyInfoList_Add(&self->local_keys, &new_keys.data[i]);
            }
        }

        KeyInfoList_Free(&new_keys);
    }
}

static void KeyManager_UpdateKeyStatus(KeyInfo* key, const KeyConfig* config) {
    if (key == NULL || config == NULL) return;

    int64_t current_ts = NTPClient_GetCurrentTime();
    int64_t expire_ts = KeyInfo_GetExpireTime(key, config);
    int64_t delete_ts = KeyInfo_GetDeleteTime(key, config);

    if (current_ts < expire_ts) {
        key->status = KEY_STATUS_VALID;
    } else if (current_ts < delete_ts) {
        key->status = KEY_STATUS_FROZEN;
    } else {
        key->status = KEY_STATUS_DELETED;
    }
}

static void KeyManager_DeleteExpiredKeys(KeyManager* self) {
    if (self == NULL || self->local_keys.size == 0) return;

    int new_size = 0;
    for (int i = 0; i < self->local_keys.size; i++) {
        if (self->local_keys.data[i].status != KEY_STATUS_DELETED) {
            self->local_keys.data[new_size] = self->local_keys.data[i];
            new_size++;
        }
    }

    if (new_size < self->local_keys.size) {
        self->local_keys.size = new_size;
        KeyInfo* new_data = (KeyInfo*)realloc(self->local_keys.data, new_size * sizeof(KeyInfo));
        if (new_data != NULL) {
            self->local_keys.data = new_data;
            self->local_keys.capacity = new_size;
        }
    }
}

bool KeyManager_AddExternalKey(KeyManager* self, const char* key_value) {
    if (self == NULL || key_value == NULL || !self->is_inited) {
        return false;
    }

    pthread_mutex_lock(&self->mutex);
    
    // 生成ID
    char key_id[KEY_ID_MAX_LEN];
    int64_t current_ts = NTPClient_GetCurrentTime();
    static int seq = 0;
    snprintf(key_id, sizeof(key_id), "ext_%lld_%d", 
             (long long)current_ts, ++seq % 1000);
    
    // 创建并添加密钥
    KeyInfo new_key;
    KeyInfo_Init(&new_key, key_id, key_value, current_ts);
    KeyInfoList_Add(&self->local_keys, &new_key);
    
    // 更新状态
    KeyManager_UpdateKeyStatus(&self->local_keys.data[self->local_keys.size - 1], 
                               &self->config);
    
    pthread_mutex_unlock(&self->mutex);
    
    return true;
}