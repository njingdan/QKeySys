#include "sim/injector.h"
#include "qkeysys/queue.h"
#include "qkeysys/os.h"
#include "qkeysys/inject.h" // 使用核心层统一注入接口
#include <stdio.h>

static qks_thread_t g_sim_thread = NULL;
static volatile int g_sim_running = 0;

// 模拟注入线程：
// - A 侧：MASTER AUTH/CLOCK（较低频）+ SLAVE KEY_REFILL（较高频），便于观察 SLAVE 立即决策（接受/丢弃）；
// - B 侧：MASTER KEY_REFILL → 窗口内注入 SLAVE AUTH/CLOCK 触发抢占；窗口外注入 SLAVE 验证丢弃；
// - 线程可安全停止，关闭不影响其他线程。
static void* sim_thread(void* arg){
    qks_sched_ctx_t* s = (qks_sched_ctx_t*)arg;
    g_sim_running = 1;
    int cycle = 0;
    while(!s->stop && g_sim_running){
        if(s->cfg.role == QKS_ROLE_A){
            // A侧：频繁注入 SLAVE KEY_REFILL（低优），以观察“立即接受/丢弃”；
            // 每隔数轮注入 MASTER AUTH/CLOCK（高优），制造守卫与窗口切换。
            qks_inject_event(s, QKS_EVENT_KEY_REFILL, QKS_DIR_SLAVE, NULL);
            for(int i=0;i<3 && !s->stop && g_sim_running;i++) qks_sleep_ms(40); // ~120ms
            if((cycle % 3)==0){
                qks_inject_event(s, QKS_EVENT_DEVICE_AUTH, QKS_DIR_MASTER, NULL);
                for(int i=0;i<6 && !s->stop && g_sim_running;i++) qks_sleep_ms(50); // ~300ms
            }
            if((cycle % 5)==0){
                qks_inject_event(s, QKS_EVENT_CLOCK_SYNC, QKS_DIR_MASTER, NULL);
                for(int i=0;i<6 && !s->stop && g_sim_running;i++) qks_sleep_ms(50); // ~300ms
            }
        } else {
            // B侧：
            // 1) 先注入 MASTER KEY_REFILL（低优），进入 MASTER Init 窗口；
            // 2) 短延时后注入 SLAVE AUTH（高优）→ 触发抢占；
            // 3) 再短延时注入 SLAVE CLOCK（高优）→ 因硬阻塞被丢弃；
            // 4) 较长延时留给低优 MASTER 后续成功执行。
            qks_inject_event(s, QKS_EVENT_KEY_REFILL, QKS_DIR_MASTER, NULL);
            for(int i=0;i<2 && !s->stop && g_sim_running;i++) qks_sleep_ms(40); // ~80ms 落在 Init 窗口
            qks_inject_event(s, QKS_EVENT_DEVICE_AUTH, QKS_DIR_SLAVE, NULL); // 预期抢占
            for(int i=0;i<2 && !s->stop && g_sim_running;i++) qks_sleep_ms(40); // ~80ms 硬阻塞期
            qks_inject_event(s, QKS_EVENT_CLOCK_SYNC, QKS_DIR_SLAVE, NULL);  // 预期丢弃
            for(int i=0;i<6 && !s->stop && g_sim_running;i++) qks_sleep_ms(50); // ~300ms 留窗
        }
        cycle++;
    }
    g_sim_running = 0;
    return NULL;
}

// 启动模拟注入器线程（仅生成事件，不中断系统其他线程）。
int qks_sim_injector_start(qks_sched_ctx_t* s){
    if(g_sim_running) return 0;
    return qks_thread_create(&g_sim_thread, sim_thread, s);
}

// 停止模拟注入器线程：置运行标志为 0 并 join。
int qks_sim_injector_stop(qks_sched_ctx_t* s){
    (void)s;
    g_sim_running = 0;
    if(g_sim_thread){ qks_thread_join(g_sim_thread); g_sim_thread = NULL; }
    return 0;
}
