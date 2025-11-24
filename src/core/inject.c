#include "qkeysys/inject.h"
#include "qkeysys/queue.h"
#include <stdio.h>

// 映射事件类型到默认优先级（数值越小优先级越高）
static int priority_of(qks_event_type_t t){
    switch(t){
        case QKS_EVENT_DEVICE_AUTH: return QKS_PRIORITY_DEVICE_AUTH;
        case QKS_EVENT_CLOCK_SYNC:  return QKS_PRIORITY_CLOCK_SYNC;
        case QKS_EVENT_KEY_REFILL:  return QKS_PRIORITY_KEY_REFILL;
        default: return 10;
    }
}

// 统一事件注入：构造事件 -> 入 FIFO -> 唤醒调度线程
int qks_inject_event(qks_sched_ctx_t* s, qks_event_type_t type, qks_event_dir_t dir, void* payload){
    if(!s) return -1;
    qks_event_t e = {0};
    e.type = type;
    e.dir = dir;
    e.priority = priority_of(type);
    e.stage = QKS_STAGE_PENDING;
    e.payload = payload;
    // 打印注入信息：
    printf("[注入] 事件入队: 类型=%d 优先级=%d 方向=%s\n", (int)type, e.priority, dir==QKS_DIR_MASTER?"主(MASTER)":"从(SLAVE)");
    return qks_queue_push_ready(&s->q, &e);
}

