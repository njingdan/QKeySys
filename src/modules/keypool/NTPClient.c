#include "NTPClient.h"
#include <time.h>   // 用于time()、clock_gettime()
#include <stddef.h> // 用于NULL定义

// 全局变量（仅当前文件可见，模拟C++静态成员变量）
static bool g_simulate_time_sync = true;  // 模拟时间同步状态
static volatile int64_t g_simulated_time = 0;  // 模拟时间戳（volatile避免编译器优化）

// ------------------------------
// 内部辅助函数
// ------------------------------
// 初始化模拟时间（首次调用时初始化）
static void NTPClient_InitSimulatedTime() {
    if (g_simulated_time == 0) {
        // 初始化为系统当前时间戳（秒级）
        time_t now = time(NULL);
        g_simulated_time = (int64_t)now;
    }
}

// ------------------------------
// 接口实现
// ------------------------------
// 获取当前NTP同步时间戳（秒级）
int64_t NTPClient_GetCurrentTime() {
    NTPClient_InitSimulatedTime();

    // 模拟模式：返回模拟时间；否则返回系统真实时间
    if (g_simulate_time_sync) {
        return g_simulated_time;
    } else {
        return (int64_t)time(NULL);
    }
}

// 检查本地时间与NTP服务器时间是否同步
bool NTPClient_CheckTimeSync(int max_diff) {
    if (max_diff <= 0) {
        return false;
    }

    // 模拟逻辑：同步状态为true则认为同步成功
    if (g_simulate_time_sync) {
        return true;
    }

    // 实际场景逻辑（注释保留，需替换为真实NTP同步检查）
    // int64_t local_ts = (int64_t)time(NULL);
    // int64_t ntp_ts = NTPClient_GetCurrentTime();
    // return abs(local_ts - ntp_ts) <= max_diff;

    return false;
}

// 执行时间同步操作（模拟）
bool NTPClient_SyncTime() {
    // 模拟同步成功：更新模拟时间为系统当前时间
    time_t now = time(NULL);
    g_simulated_time = (int64_t)now;
    g_simulate_time_sync = true;
    return true;
}

// 设置时间同步模拟状态（测试用）
void NTPClient_SetSimulateTimeSync(bool is_sync) {
    g_simulate_time_sync = is_sync;
}

// 重置模拟时间（测试用）
void NTPClient_ResetSimulatedTime() {
    g_simulated_time = 0;
    g_simulate_time_sync = true;
}

// 推进模拟时间（测试用，用于模拟密钥过期）
void NTPClient_AdvanceSimulatedTime(int seconds) {
    if (seconds <= 0) {
        return;
    }
    NTPClient_InitSimulatedTime();
    g_simulated_time += seconds; // 向前推进seconds秒
}