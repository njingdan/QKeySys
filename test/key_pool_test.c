#include "keypool.h"
#include <stdio.h>
#include "KeyManager.h"
#include <windows.h>  // 用于 Sleep

int main() {
    printf("=== 开始测试 keypool 接口 ===\n");

    // 测试1：调用 qks_keypool_update
    printf("\n[测试1] 调用 qks_keypool_update（密钥1）\n");
    qks_keypool_update("qk_2025_0501_abcdef1234");
    Sleep(100);  // 短暂延迟，避免锁竞争

    // 测试2：调用 qks_keypool_update
    printf("\n[测试2] 调用 qks_keypool_update（密钥2）\n");
    qks_keypool_update("qk_2025_0501_567890abcd");
    Sleep(100);

    // 测试3：调用 qks_keypool_execute（兼容模式）
    printf("\n[测试3] 调用 qks_keypool_execute（兼容模式）\n");
    const char* qk3 = "qk_2025_0501_xyz789";
    qks_keypool_execute((void*)qk3);
    Sleep(100);

    // 测试4：调用 qks_keypool_execute（检查模式）
    printf("\n[测试4] 调用 qks_keypool_execute（检查模式）\n");
    qks_keypool_execute(NULL);
    Sleep(100);

    // 停止密钥管理器
    printf("\n[测试5] 停止密钥管理器\n");
    KeyManager* manager = KeyManager_GetInstance();
    KeyManager_Stop(manager);

    printf("\n=== 所有测试执行完成！ ===\n");
    return 0;
}