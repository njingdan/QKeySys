#ifndef QKEYSYS_EXEC_H
#define QKEYSYS_EXEC_H

#include "scheduler.h"
#include "event.h"

// 执行线程：每类事件一个工作线程。被切换到 Exec 时被唤醒，仅打印并通知完成。
int qks_exec_threads_start(qks_sched_ctx_t* s);
int qks_exec_threads_stop(qks_sched_ctx_t* s);
int qks_exec_signal(qks_sched_ctx_t* s, qks_event_type_t type);

#endif /* QKEYSYS_EXEC_H */

