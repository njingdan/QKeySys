#ifndef QKEYSYS_INIT_SLAVE_H
#define QKEYSYS_INIT_SLAVE_H

#include "scheduler.h"
#include "event.h"

// 从初始化线程相关接口（占位实现）：
// - admitable: 判断当前是否可立即执行；
// - accept_and_claim: 接受并占位（直接安装到执行区，stage=InitWaitAct，立刻硬阻塞）；
// - on_act: 收到 ACT 后进入 Exec（保持硬阻塞）；
// - cancel: 超时/取消时清理并结束事件。

int qks_init_slave_admitable(qks_sched_ctx_t* s, qks_event_type_t type);
int qks_init_slave_accept_and_claim(qks_sched_ctx_t* s, qks_event_type_t type, void* payload);
int qks_init_slave_on_act(qks_sched_ctx_t* s);
int qks_init_slave_reject_busy(qks_sched_ctx_t* s);
int qks_init_slave_cancel(qks_sched_ctx_t* s);

#endif /* QKEYSYS_INIT_SLAVE_H */

