#ifndef QKEYSYS_SIM_INJECTOR_H
#define QKEYSYS_SIM_INJECTOR_H

#include "qkeysys/scheduler.h"

// 模拟注入线程：仅在 A 侧且模拟开关为 on 时启动
int qks_sim_injector_start(qks_sched_ctx_t* s);
int qks_sim_injector_stop(qks_sched_ctx_t* s);

#endif /* QKEYSYS_SIM_INJECTOR_H */

