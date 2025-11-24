// 时钟线程（占位实现）
// 职责：未来可在 A 侧周期生成策略性事件（如认证/时钟同步），或进行密钥寿命管理；
// 当前：空循环，仅按 s->stop 退出。
#include "qkeysys/scheduler.h"
#include "qkeysys/os.h"

static void* clock_thread(void* arg){
    qks_sched_ctx_t* s=(qks_sched_ctx_t*)arg;
    while(!s->stop){
        qks_sleep_ms(200);
    }
    return NULL;
}

int qks_clock_thread_start(qks_sched_ctx_t* s){
    return qks_thread_create(&s->th_clock, clock_thread, s);
}

int qks_clock_thread_stop(qks_sched_ctx_t* s){
    if(s->th_clock){ qks_thread_join(s->th_clock); s->th_clock=NULL; }
    return 0;
}
