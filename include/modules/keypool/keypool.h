#ifndef QKEYSYS_MODULES_KEYPOOL_H
#define QKEYSYS_MODULES_KEYPOOL_H

// 密钥池维护/分发模块占位：
// 备注：当前仅提供占位接口，不做真实更新策略与阈值控制。
// - qks_keypool_update: 使用传入的量子密钥字符串进行占位更新；返回0视为成功。
// - qks_keypool_execute: 兼容保留的旧接口（占位），后续建议逐步移除。

int qks_keypool_update(const char* qk_str);
int qks_keypool_execute(void* payload);

#endif /* QKEYSYS_MODULES_KEYPOOL_H */
