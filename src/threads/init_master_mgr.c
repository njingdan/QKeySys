// 主初始化线程管理（事件临时线程）
// - spawn：当 MASTER 事件被装入执行区（Init阶段）时，创建一个短生命周期的主初始化线程
// - kill_current：发生抢占或系统停止时，强制终止当前主初始化线程并回收句柄
// 线程体：调用占位流程 INIT_REQ→等待ACK→发送ACT→切EXEC→唤醒执行线程，完成后自行退出
#include "qkeysys/threads.h"
#include "qkeysys/init_master.h"
#include "qkeysys/os.h"
#ifdef QKS_OS_POSIX
#include <pthread.h>
#endif

static void* init_master_thread(void* arg){
    qks_sched_ctx_t* s=(qks_sched_ctx_t*)arg;
#ifdef QKS_OS_POSIX
    // 允许异步取消，便于抢占时立即终止
    (void)pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    (void)pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
#endif
    // 拷贝当前执行区事件快照
    qks_event_t ev = s->q.exec_event;
    (void)qks_init_master_start(s, &ev);
    // 线程结束前复位运行标志
    s->init_master_running = 0;
    return NULL;
}

int qks_init_master_spawn(qks_sched_ctx_t* s){
    if(s->init_master_running){
        // 已有线程，先终止（保险）
        if(s->th_init_master){ qks_thread_kill(s->th_init_master); s->th_init_master=NULL; }
        s->init_master_running=0;
    }
    s->init_master_running=1;
    return qks_thread_create(&s->th_init_master, init_master_thread, s);
}

int qks_init_master_kill_current(qks_sched_ctx_t* s){
    if(s->th_init_master){ qks_thread_kill(s->th_init_master); s->th_init_master=NULL; }
    s->init_master_running=0;
    return 0;
}
