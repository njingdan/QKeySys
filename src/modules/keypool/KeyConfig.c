#include "KeyConfig.h"
#include <stddef.h> // 用于NULL定义

// 计算冷冻期时长（实现）
int KeyConfig_GetFreezeDuration(const KeyConfig* config) {
    // 空指针保护：避免传入NULL导致崩溃
    if (config == NULL) {
        return 0;
    }
    // 按比例计算，强转为int（与原C++逻辑一致）
    return (int)(config->key_validity * config->freeze_ratio);
}

// 计算轮询周期（实现）
int KeyConfig_GetPollCycle(const KeyConfig* config) {
    if (config == NULL) {
        return 0;
    }
    return (int)(config->key_validity * config->poll_ratio);
}