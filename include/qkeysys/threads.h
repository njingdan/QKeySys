#ifndef QKEYSYS_THREADS_H
#define QKEYSYS_THREADS_H

#include "qkeysys/scheduler.h"

// 统一线程启动/停止
int qks_threads_start_all(qks_sched_ctx_t* s);
int qks_threads_stop_all(qks_sched_ctx_t* s);

// 主初始化线程（短生命周期）管理
int qks_init_master_spawn(qks_sched_ctx_t* s);
int qks_init_master_kill_current(qks_sched_ctx_t* s);

#endif /* QKEYSYS_THREADS_H */

