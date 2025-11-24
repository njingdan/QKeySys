#ifndef QKEYSYS_EVENT_H
#define QKEYSYS_EVENT_H

#include <stdint.h>

// 事件类型：根据需求可扩展
typedef enum {
    QKS_EVENT_DEVICE_AUTH = 1,  // 设备认证（优先级=1）
    QKS_EVENT_CLOCK_SYNC  = 2,  // 时钟同步（优先级=1，注意同为1）
    QKS_EVENT_KEY_REFILL  = 3   // 密钥分发/补充（优先级=4，见默认宏）
} qks_event_type_t;

// 事件方向：主（发起方）/从（接收方）
typedef enum {
    QKS_DIR_MASTER = 1,
    QKS_DIR_SLAVE  = 2
} qks_event_dir_t;

// 事件阶段：
typedef enum {
    QKS_STAGE_PENDING = 0,   // 入队待执行
    QKS_STAGE_INIT    = 1,   // 进入执行区，主初始化Init / 从初始化尚未接纳
    QKS_STAGE_IWAIT   = 2,   // 从初始化已接纳，等待ACT（硬阻塞）
    QKS_STAGE_EXEC    = 3,   // 执行阶段（硬阻塞）
    QKS_STAGE_DONE    = 4,   // 完成
    QKS_STAGE_ABORT   = 5    // 失败/取消
} qks_stage_t;

// 默认优先级（数值越小优先级越高）
// 注意：设备认证与时钟同步优先级均为1，密钥分发为4。
#define QKS_PRIORITY_DEVICE_AUTH 1
#define QKS_PRIORITY_CLOCK_SYNC  1
#define QKS_PRIORITY_KEY_REFILL  4

// 三类事件的Payload占位结构：待实现，根据后续需求完善。
typedef struct {
    // TODO: method/params 等字段，待实现
    void* params_ptr;
    int   params_len;
} qks_payload_auth_t;

typedef struct {
    // TODO: target_level/threshold/policy 等字段，待实现
    void* params_ptr;
    int   params_len;
} qks_payload_keyrefill_t;

typedef struct {
    // TODO: mode/window_ms/tolerance_ms 等字段，待实现
    void* params_ptr;
    int   params_len;
} qks_payload_clocksync_t;

typedef struct {
    qks_event_type_t type;
    qks_event_dir_t  dir;
    int              priority;  // 排序用：越小越高；同优先级严格FIFO
    qks_stage_t      stage;     // 当前阶段
    void*            payload;   // 指向对应payload结构的指针
    uint64_t         seq;       // 入队序号（保持稳定顺序）
    // 反饥饿预留（默认不启用）：
    uint64_t         enqueue_ts;
    uint32_t         preempt_count;
} qks_event_t;

#endif /* QKEYSYS_EVENT_H */

