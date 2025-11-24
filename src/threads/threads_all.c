// 统一线程生命周期管理
// - start_all: 启动执行工作线程、各监听线程（占位）、调度线程
// - stop_all : 设置全局停止，按顺序停止调度、监听与执行线程，并杀死可能仍在运行的主初始化线程
#include "qkeysys/threads.h"
#include "qkeysys/exec.h"
#include "qkeysys/os.h"
#include "sim/injector.h"

// forward decls from sibling C files
int qks_scheduler_thread_start(qks_sched_ctx_t* s);
int qks_scheduler_thread_stop(qks_sched_ctx_t* s);
int qks_zigbee_listener_start(qks_sched_ctx_t* s);
int qks_zigbee_listener_stop(qks_sched_ctx_t* s);
int qks_clock_thread_start(qks_sched_ctx_t* s);
int qks_clock_thread_stop(qks_sched_ctx_t* s);
int qks_storage_listener_start(qks_sched_ctx_t* s);
int qks_storage_listener_stop(qks_sched_ctx_t* s);

// 启动全部线程：
// - 执行分发/工作线程；
// - 监听线程（Zigbee/时钟/储能柜 B）；
// - 可选模拟器（sim=on 时启动，A/B 通用，仅生成事件）；
// - 调度线程；
int qks_threads_start_all(qks_sched_ctx_t* s){
    // 启动执行线程（单工作线程）
    qks_exec_threads_start(s);
    // 监听线程（占位）
    qks_zigbee_listener_start(s);
    qks_clock_thread_start(s);
    if(s->cfg.role == QKS_ROLE_B) qks_storage_listener_start(s);
    // 模拟注入器
    if(s->cfg.sim_enabled){
        qks_sim_injector_start(s);
    }
    // 调度线程
    qks_scheduler_thread_start(s);
    return 0;
}

// 停止全部线程：
// - 置 stop 并唤醒调度线程；
// - 先停调度、模拟器与监听线程，再停执行线程，最后清理初始化线程；
int qks_threads_stop_all(qks_sched_ctx_t* s){
    // 发出全局停止
    s->stop = 1;
    // 唤醒可能在等待 READY 的调度线程
    qks_queue_wake(&s->q);
    // 停止调度线程
    qks_scheduler_thread_stop(s);
    // 停止模拟注入器
    qks_sim_injector_stop(s);
    // 停止监听（空实现）
    qks_zigbee_listener_stop(s);
    qks_clock_thread_stop(s);
    if(s->cfg.role == QKS_ROLE_B) qks_storage_listener_stop(s);
    // 停止执行线程
    qks_exec_threads_stop(s);
    // 终止潜在的主初始化线程
    qks_init_master_kill_current(s);
    return 0;
}
