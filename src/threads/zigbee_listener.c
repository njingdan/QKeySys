// Zigbee 监听线程（占位实现）
// 职责：未来接入 qks_zigbee_recv，解析 INIT/ACK/ACT 帧；
//       在收到对端 INIT_REQ 时，调用 qks_init_slave_accept_and_claim 等接口接入从初始化逻辑；
// 当前：空循环，仅按 s->stop 退出。
#include "qkeysys/scheduler.h"
#include "qkeysys/os.h"

static void* zigbee_listener_thread(void* arg){
    qks_sched_ctx_t* s=(qks_sched_ctx_t*)arg;
    while(!s->stop){
        qks_sleep_ms(100);
    }
    return NULL;
}

int qks_zigbee_listener_start(qks_sched_ctx_t* s){
    return qks_thread_create(&s->th_zigbee, zigbee_listener_thread, s);
}

int qks_zigbee_listener_stop(qks_sched_ctx_t* s){
    if(s->th_zigbee){ qks_thread_join(s->th_zigbee); s->th_zigbee=NULL; }
    return 0;
}
