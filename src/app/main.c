#include <stdio.h>
#include "qkeysys/config.h"
#include "qkeysys/scheduler.h"
#include "qkeysys/queue.h"
#include "qkeysys/exec.h"
#include "qkeysys/os.h"
#include "qkeysys/threads.h"

// 主程序：
// - 解析角色（A/B）；
// - 初始化队列与调度上下文；
// - 启动调度循环（当前为阻塞空跑，未注入事件）。

// 计时线程：
// - 若配置了 run_seconds>0，则每秒检查一次 stop 标志；
// - 时间到达后将 s->stop=1 以触发退出；
// - 不负责终止其他线程，统一在主线程收尾时通过 qks_threads_stop_all 处理。
static void* timer_thread(void* arg){
    qks_sched_ctx_t* s=(qks_sched_ctx_t*)arg;
    int secs = s->cfg.run_seconds;
    if(secs>0){
        for(int i=0;i<secs;i++){ if(s->stop) break; qks_sleep_ms(1000); }
        s->stop=1;
    }
    return NULL;
}

// 程序入口：
// - 装载配置（环境变量与命令行参数）；
// - 初始化调度上下文（队列与配置）；
// - 启动全部线程（调度、执行、监听、模拟器可选）；
// - 根据 run_seconds 选择常驻或定时退出；
// - 退出时统一停止所有线程。
int main(int argc, char** argv){
    qks_config_t cfg; qks_config_load_env(&cfg); qks_config_parse_args(&cfg, argc, argv);
    printf("QKeySys start. Role=%s\n", cfg.role==QKS_ROLE_A?"A":"B");

    qks_sched_ctx_t sched = {0}; sched.cfg = cfg; qks_queue_init(&sched.q);
    // 启动线程集合（事件队列、执行、监听等）
    qks_threads_start_all(&sched);
    // 可选自动退出：启动定时停止线程
    qks_thread_t th_timer = NULL;
    if(cfg.run_seconds>0){ qks_thread_create(&th_timer, timer_thread, &sched); }

    // 等待退出：由计时器或外部信号触发 stop
    // 主线程空转等待
    while(!sched.stop){ qks_sleep_ms(100); }

    // 退出：停止并回收线程
    qks_threads_stop_all(&sched);
    
    return 0;
}
