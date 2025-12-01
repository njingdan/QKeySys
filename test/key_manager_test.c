#include <stdio.h>
#include <unistd.h>  // 用于sleep
#include "KeyManager.h"
#include "NTPClient.h"
#include "KeyInfo.h"
#include "GatewayComm.h"

// 辅助函数：打印单个密钥信息
void printKeyInfo(const KeyInfo* key) {
    if (key == NULL) {
        printf("无效密钥\n");
        return;
    }
    const char* status_str = NULL;
    switch (key->status) {
        case KEY_STATUS_VALID:   status_str = "有效"; break;
        case KEY_STATUS_FROZEN:  status_str = "冷冻"; break;
        case KEY_STATUS_DELETED: status_str = "已删除"; break;
        default:                 status_str = "未知";
    }
    printf("密钥ID：%s | 密钥值：%s | 创建时间：%lld | 状态：%s\n",
           key->key_id, key->key_value, (long long)key->create_ts, status_str);
}

// 辅助函数：打印本地所有密钥列表
void printAllLocalKeys(KeyManager* manager) {
    if (manager == NULL) return;
    KeyInfoList key_list;
    KeyManager_GetLocalKeys(manager, &key_list);
    printf("\n=== 当前本地密钥列表（共%d个） ===\n", key_list.size);
    for (int i = 0; i < key_list.size; i++) {
        printKeyInfo(&key_list.data[i]);
    }
    KeyInfoList_Free(&key_list);  // 释放列表内存
}

int main() {
    printf("=== 密钥管理器测试程序 ===\n");

    // 1. 获取密钥管理器实例并初始化
    KeyManager* manager = KeyManager_GetInstance();
    if (!KeyManager_Init(manager)) {
        fprintf(stderr, "错误：密钥管理器初始化失败！\n");
        return -1;
    }
    printf("✅ 密钥管理器初始化成功\n");


    // 2. 打印初始配置
    KeyConfig config = KeyManager_GetConfig(manager);
    printf("\n=== 当前配置 ===\n");
    printf("密钥有效期：%d秒 | 冷冻期比例：%.2f | 轮询周期比例：%.2f\n",
           config.key_validity, config.freeze_ratio, config.poll_ratio);
    printf("有效密钥阈值：%d个 | NTP最大偏差：%d秒\n",
           config.valid_threshold, config.ntp_max_diff);


    // 3. 获取初始有效密钥
    KeyInfo valid_key;
    if (KeyManager_GetValidKey(manager, &valid_key)) {
        printf("\n✅ 获取到初始有效密钥：\n");
        printKeyInfo(&valid_key);
    } else {
        fprintf(stderr, "\n❌ 未获取到初始有效密钥！\n");
        KeyManager_Stop(manager);
        return -1;
    }


    // 4. 打印本地所有密钥
    printAllLocalKeys(manager);


    // 5. 模拟时间推进（让密钥过期：有效期60秒，推进70秒）
    printf("\n=== 模拟时间推进70秒（密钥有效期60秒） ===\n");
    NTPClient_AdvanceSimulatedTime(70);
    printf("当前模拟时间：%lld\n", (long long)NTPClient_GetCurrentTime());


    // 6. 等待轮询线程自动检查（轮询周期3%*60=1.8秒，这里等待2秒）
    printf("\n等待轮询线程检查密钥状态...\n");
    sleep(2);


    // 7. 再次打印本地密钥（原密钥应变为“已删除”，并自动请求新密钥）
    printAllLocalKeys(manager);


    // 8. 再次获取有效密钥（应获取到新请求的密钥）
    if (KeyManager_GetValidKey(manager, &valid_key)) {
        printf("\n✅ 过期后获取到新有效密钥：\n");
        printKeyInfo(&valid_key);
    } else {
        fprintf(stderr, "\n❌ 过期后未获取到有效密钥！\n");
        KeyManager_Stop(manager);
        return -1;
    }


    // 9. 测试手动模拟密钥过期
    printf("\n=== 手动模拟2个密钥过期 ===\n");
    KeyManager_SimulateKeyExpiry(manager, 2);
    printAllLocalKeys(manager);


    // 10. 停止密钥管理器
    printf("\n=== 停止密钥管理器 ===\n");
    KeyManager_Stop(manager);
    printf("✅ 密钥管理器已停止\n");


    system("pause");

    return 0;
}