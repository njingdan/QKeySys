// 调度线程封装：将阻塞式的 qks_scheduler_start 放入独立线程运行
// 职责：持续从就绪队列取事件，处理硬/软阻塞与抢占，装入执行区并拉起主初始化线程
// 注意：调度循环内部已检查 s->stop 标志，此处仅负责启动与回收线程
#include "qkeysys/scheduler.h"
#include "qkeysys/os.h"

static void* scheduler_thread(void* arg){
    qks_sched_ctx_t* s=(qks_sched_ctx_t*)arg;
    qks_scheduler_start(s);
    return NULL;
}

int qks_scheduler_thread_start(qks_sched_ctx_t* s){
    return qks_thread_create(&s->th_scheduler, scheduler_thread, s);
}

int qks_scheduler_thread_stop(qks_sched_ctx_t* s){
    if(s->th_scheduler){ qks_thread_join(s->th_scheduler); s->th_scheduler=NULL; }
    return 0;
}
