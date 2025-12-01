#ifndef GATEWAY_COMM_H
#define GATEWAY_COMM_H

#include <stdint.h>
#include <stdbool.h>
#include "KeyInfo.h"
#include "KeyConfig.h"

// 动态数组结构体声明（替代std::vector<KeyInfo>）
typedef struct {
    KeyInfo* data;       // 存储数据的指针（动态分配）
    int size;            // 当前元素个数
    int capacity;        // 已分配的容量
} KeyInfoList;

// 动态数组基础操作声明
void KeyInfoList_Init(KeyInfoList* list);
int KeyInfoList_Add(KeyInfoList* list, const KeyInfo* key);
void KeyInfoList_Free(KeyInfoList* list);

// 网关通信接口声明（无实现）
// 向网关请求新密钥（out_keys需提前初始化，调用者负责释放）
int GatewayComm_RequestNewKeys(int need_count, const KeyConfig* config, KeyInfoList* out_keys);

// 向网关发送加密数据（data：数据缓冲区，data_len：数据长度）
bool GatewayComm_SendEncryptedData(const char* data, int data_len, const char* key_id);

// 从网关同步配置参数
KeyConfig GatewayComm_SyncConfig();

// 新增：添加GatewayComm_RetryRequest的声明（关键！）
bool GatewayComm_RetryRequest(int max_retry, int interval);

#endif // GATEWAY_COMM_H