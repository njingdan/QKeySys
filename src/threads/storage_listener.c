// 储能柜侧监听线程（仅 B 侧启动，占位实现）
// 职责：未来接入本地设备/总线消息的监听逻辑，与 Zigbee 监听配合；
// 当前：空循环，仅按 s->stop 退出。
#include "qkeysys/scheduler.h"
#include "qkeysys/os.h"

static void* storage_listener_thread(void* arg){
    qks_sched_ctx_t* s=(qks_sched_ctx_t*)arg;
    while(!s->stop){
        qks_sleep_ms(200);
    }
    return NULL;
}

int qks_storage_listener_start(qks_sched_ctx_t* s){
    return qks_thread_create(&s->th_storage, storage_listener_thread, s);
}

int qks_storage_listener_stop(qks_sched_ctx_t* s){
    if(s->th_storage){ qks_thread_join(s->th_storage); s->th_storage=NULL; }
    return 0;
}
