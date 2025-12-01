#ifndef KEY_CONFIG_H
#define KEY_CONFIG_H

// 密钥配置参数（支持从网关同步，比例联动）
typedef struct {
    int key_validity;     // 密钥有效期（秒）
    float freeze_ratio;   // 冷冻期比例（相对于有效期）
    float poll_ratio;     // 轮询周期比例（相对于有效期）
    int valid_threshold;  // 有效密钥数量阈值
    int ntp_max_diff;     // NTP时间最大允许偏差（秒）
} KeyConfig;

// 计算联动参数的工具函数声明（无实现）
// 获取冷冻期时长
int KeyConfig_GetFreezeDuration(const KeyConfig* config);

// 获取轮询周期
int KeyConfig_GetPollCycle(const KeyConfig* config);

// 默认配置实例（const+static避免修改和多重定义）
static const KeyConfig DEFAULT_CONFIG = {
    60,         // 测试用密钥有效期：1分钟（60秒）
    0.3f,       // 冷冻期比例：30%
    0.03f,      // 轮询周期比例：3%
    3,          // 有效密钥阈值：3个
    30          // 时间最大偏差：30秒
};

#endif // KEY_CONFIG_H