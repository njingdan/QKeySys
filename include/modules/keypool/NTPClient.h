#ifndef NTP_CLIENT_H
#define NTP_CLIENT_H

#include <stdint.h>
#include <stdbool.h>

// 接口声明（无实现）
// 获取当前NTP同步时间戳（秒级）
int64_t NTPClient_GetCurrentTime();

// 检查本地时间与NTP服务器时间是否同步
// max_diff：允许的最大时间差（秒）
bool NTPClient_CheckTimeSync(int max_diff);

// 执行时间同步操作（模拟）
bool NTPClient_SyncTime();

// 设置时间同步模拟状态（用于测试场景）
void NTPClient_SetSimulateTimeSync(bool is_sync);

// 重置模拟时间（用于场景切换时的时间隔离）
void NTPClient_ResetSimulatedTime();

// 测试辅助函数：推进模拟时间（用于测试密钥过期）
void NTPClient_AdvanceSimulatedTime(int seconds);

#endif // NTP_CLIENT_H