#ifndef QKEYSYS_INJECT_H
#define QKEYSYS_INJECT_H

#include "event.h"
#include "scheduler.h"

// 事件注入统一接口（核心层）
// 目的：为模拟器、监听线程以及真实系统提供统一的事件入队入口，避免各处直接操作队列。
// 用法：在任意持有 qks_sched_ctx_t* 的线程中调用；函数内部会将事件入 FIFO，并唤醒调度线程。
// 说明：
// - 优先级按事件类型自动赋值（设备认证=1、时钟同步=1、密钥分发=4）。
// - 事件阶段固定填入 QKS_STAGE_PENDING；seq=0 由队列层自动分配；payload 透传（当前未使用）。
// - 注入后是否立即执行由调度器决定：MASTER 可能入等待堆；SLAVE 仅“立即决策”，不可执行则丢弃。
// 返回：0 表示成功；非 0 表示失败（例如内存不足）。
int qks_inject_event(qks_sched_ctx_t* s, qks_event_type_t type, qks_event_dir_t dir, void* payload);

#endif /* QKEYSYS_INJECT_H */
