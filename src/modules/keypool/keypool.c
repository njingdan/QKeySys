#include "modules/keypool/keypool.h"  // 师姐的头文件
#include <stdio.h>
#include <string.h>  // 用于strncpy
#include <pthread.h> // 添加线程支持
#include "KeyManager.h"   // 你的密钥管理器头文件
#include "NTPClient.h"    // 你的时间工具头文件
#include "KeyInfo.h"      // 你的密钥结构体头文件

// 如果没有定义，在这里添加常量
#ifndef MAX_KEY_VALUE_LEN
#define MAX_KEY_VALUE_LEN 256
#endif

// 全局变量：持有你的密钥管理器单例（确保全局唯一）
static KeyManager* g_key_manager = NULL;

// 初始化密钥管理器（内部私有函数，仅调用一次）
static void keypool_init_if_needed() {
    static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
    static bool init_called = false;
    
    if (init_called) return;
    
    pthread_mutex_lock(&init_mutex);
    if (!init_called) {
        g_key_manager = KeyManager_GetInstance();
        if (g_key_manager == NULL) {
            fprintf(stderr, "[KeyPool] 错误：KeyManager_GetInstance 返回NULL\n");
        } else if (!KeyManager_Init(g_key_manager)) {
            fprintf(stderr, "[KeyPool] 密钥管理器初始化失败！\n");
            g_key_manager = NULL; // 标记为失败
        } else {
            printf("[KeyPool] 密钥管理器初始化成功（阈值：%d个，有效期：%d秒）\n",
                   g_key_manager->config.valid_threshold,
                   g_key_manager->config.key_validity);
        }
        init_called = true;
    }
    pthread_mutex_unlock(&init_mutex);
}

// --------------- 实现师姐要求的 qks_keypool_update ---------------
int qks_keypool_update(const char* qk_str) {
    // 1. 入参检查
    if (qk_str == NULL) {
        fprintf(stderr, "[KeyPool] 错误：传入的密钥字符串为NULL\n");
        return -1;
    }
    
    size_t len = strlen(qk_str);
    if (len == 0) {
        fprintf(stderr, "[KeyPool] 错误：密钥字符串为空\n");
        return -1;
    }
    
    if (len > MAX_KEY_VALUE_LEN) {
        fprintf(stderr, "[KeyPool] 警告：密钥字符串过长（%zu > %d），将被截断\n",
                len, MAX_KEY_VALUE_LEN);
        // 可以继续执行，但密钥会被截断
    }

    // 2. 确保密钥管理器已初始化
    keypool_init_if_needed();
    if (g_key_manager == NULL) {
        fprintf(stderr, "[KeyPool] 错误：密钥管理器实例为NULL\n");
        return -1;
    }
    
    if (!g_key_manager->is_inited) {
        fprintf(stderr, "[KeyPool] 错误：密钥管理器未初始化\n");
        return -1;
    }

    // 3. 直接调用 KeyManager 的API
    // 注意：KeyManager_AddExternalKey 需要已经实现！
    if (!KeyManager_AddExternalKey(g_key_manager, qk_str)) {
        fprintf(stderr, "[KeyPool] 错误：KeyManager_AddExternalKey 失败\n");
        return -1;
    }
    
    printf("[KeyPool] qks_keypool_update 成功：添加密钥 '%s'\n", qk_str);
    return 0;
}

// --------------- 实现师姐要求的 qks_keypool_execute ---------------
int qks_keypool_execute(void* payload) {
    // 1. 确保密钥管理器已初始化
    keypool_init_if_needed();
    if (g_key_manager == NULL) {
        fprintf(stderr, "[KeyPool] 错误：密钥管理器实例为NULL\n");
        return -1;
    }
    
    if (!g_key_manager->is_inited) {
        fprintf(stderr, "[KeyPool] 错误：密钥管理器未初始化\n");
        return -1;
    }

    if (payload != NULL) {
        // 用法1：payload是密钥字符串 → 直接调用 update
        const char* qk_str = (const char*)payload;
        printf("[KeyPool] qks_keypool_execute（兼容模式）：传入密钥字符串\n");
        return qks_keypool_update(qk_str);
    } else {
        // 用法2：payload是NULL → 触发密钥池全量检查
        printf("[KeyPool] qks_keypool_execute（检查模式）：触发密钥池维护\n");
        
        // 直接调用 KeyManager 的函数
        KeyManager_CheckAllKeys(g_key_manager);

        // 可选：打印维护后的状态
#ifdef DEBUG_KEYPOOL
        KeyInfoList key_list;
        KeyManager_GetLocalKeys(g_key_manager, &key_list);
        
        int valid_count = 0;
        for (int i = 0; i < key_list.size; i++) {
            if (key_list.data[i].status == KEY_STATUS_VALID) {
                valid_count++;
            }
        }
        
        printf("[KeyPool] 维护完成：总密钥数=%d，有效密钥数=%d（阈值=%d）\n",
               key_list.size, valid_count, 
               g_key_manager->config.valid_threshold);
        
        KeyInfoList_Free(&key_list);
#endif
    }

    return 0;
}