#ifndef QKEYSYS_INIT_MASTER_H
#define QKEYSYS_INIT_MASTER_H

#include "scheduler.h"
#include "event.h"

// 主初始化线程相关接口（占位实现）：
// - start: 被调度唤醒，进入 Init，发送 INIT_REQ 并等待 ACK；
// - on_ack: 收到 ACK 后发送 ACT 并请求进入 Exec（硬阻塞）；
// - cancel: 抢占/上层取消时安全退出（基于代次 token + 取消标志）。

int qks_init_master_start(qks_sched_ctx_t* s, const qks_event_t* e);
int qks_init_master_on_ack(qks_sched_ctx_t* s);
int qks_init_master_send_act(qks_sched_ctx_t* s);
int qks_init_master_cancel(qks_sched_ctx_t* s);

#endif /* QKEYSYS_INIT_MASTER_H */

