#ifndef QKEYSYS_SCHEDULER_H
#define QKEYSYS_SCHEDULER_H

#include "types.h"
#include "config.h"
#include "queue.h"

typedef struct {
    qks_config_t cfg;
    qks_queue_t  q;
    int          stop; // 全局停止标志（自动退出）

    // 执行线程唤醒信号（低线程模式：单执行工作线程）
    struct {
        qks_mutex_t      mu;
        qks_cond_t       cv;
        int              wake;
        qks_event_type_t pending_type;
        qks_role_t       pending_role;   // 新增：用于分发到不同角色工作线程
        int              worker_done;    // 分发器等待专用工作线程完成
        qks_cond_t       cv_done;
    } exec_sig;

    // 线程句柄
    qks_thread_t th_scheduler;
    qks_thread_t th_exec;         // 单执行工作线程
    qks_thread_t th_init_master;  // 主初始化线程（临时）
    int          init_master_running;
    qks_thread_t th_zigbee;       // 监听线程（占位）
    qks_thread_t th_clock;        // 时钟线程（占位）
    qks_thread_t th_storage;      // B 侧监听（占位）

    // 专用执行工作线程（每事件×角色 A/B，共6个），以及对应唤醒信号
    struct {
        qks_mutex_t mu;
        qks_cond_t  cv;
        int         wake;
    } worker_sig[6];
    qks_thread_t th_worker[6];
} qks_sched_ctx_t;

// 主调度线程：从就绪队列取事件，处理硬/软阻塞，执行抢占规则，并唤醒对应初始化线程。
int qks_scheduler_start(qks_sched_ctx_t* s);
int qks_scheduler_stop(qks_sched_ctx_t* s);

// 执行区与守卫管理（供初始化线程调用）
int qks_scheduler_set_exec_slot(qks_sched_ctx_t* s, const qks_event_t* e, qks_stage_t stage, int hard_blocking);
int qks_scheduler_clear_exec_slot(qks_sched_ctx_t* s);
int qks_scheduler_set_blocking(qks_sched_ctx_t* s, int hard_blocking);
int qks_scheduler_update_stage(qks_sched_ctx_t* s, qks_stage_t stage);
int qks_scheduler_notify_event_done(qks_sched_ctx_t* s, qks_event_type_t type, int result_code);

// 抢占当前 Init：仅在 MASTER Init 窗口触发，终止正在进行的初始化线程。
int qks_scheduler_preempt_current(qks_sched_ctx_t* s);

// 执行线程唤醒（切换到Exec后由初始化线程调用）
int qks_exec_threads_start(qks_sched_ctx_t* s);
int qks_exec_threads_stop(qks_sched_ctx_t* s);
int qks_exec_signal(qks_sched_ctx_t* s, qks_event_type_t type);

// 线程集合管理与主初始化线程管理（在 threads 层实现）
// 由 main 调用的统一启停接口在 threads.h 中声明。

#endif /* QKEYSYS_SCHEDULER_H */
