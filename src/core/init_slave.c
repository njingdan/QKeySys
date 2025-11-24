// 从初始化（SLAVE Init）
// - SLAVE 事件“仅允许立即执行”，不进入等待堆；是否可立即执行由调度器在临界区内决策；
// - 被接受时：直接在执行区装入 IWAIT，并开启硬阻塞（立即占位）；
// - 随后在收到 ACT 时切换为 EXEC，并唤醒执行分发线程；
// - 不可立即执行时：由调度器直接丢弃（打印忙碌/丢弃），本模块无需处理。
#include "qkeysys/init_slave.h"
#include "qkeysys/exec.h"
#include "qkeysys/queue.h"
#include "config/timeouts.h"
#include "qkeysys/log.h"
#include "comm/zigbee/zigbee.h"  // 统一通信入口：占位 Zigbee 收发
#include <stdio.h>

static int priority_of(qks_event_type_t t){
    switch(t){
        case QKS_EVENT_DEVICE_AUTH: return QKS_PRIORITY_DEVICE_AUTH;
        case QKS_EVENT_CLOCK_SYNC:  return QKS_PRIORITY_CLOCK_SYNC;
        case QKS_EVENT_KEY_REFILL:  return QKS_PRIORITY_KEY_REFILL;
        default: return 10;
    }
}

// 是否可接受（仅用于占位/自检）：
// - 硬阻塞=ON 或 执行区占用 → 返回 0（忙碌）；
// - 否则返回 1（可接受）。实际项目中该判断由调度器集中执行更安全。
int qks_init_slave_admitable(qks_sched_ctx_t* s, qks_event_type_t type){
    (void)type;
    int hard, pri; qks_queue_get_guard(&s->q, &hard, &pri);
    if(hard) return 0; // 硬阻塞直接忙碌
    if(s->q.has_exec) return 0; // 执行区占用
    return 1;
}

// 接受并占位：
// - 直接在执行区安装 SLAVE 事件为 IWAIT，并开启硬阻塞，表明“已占用执行区等待 ACT”；
// - 后续由 on_act 切到 EXEC。
int qks_init_slave_accept_and_claim(qks_sched_ctx_t* s, qks_event_type_t type, void* payload){
    qks_event_t e={0}; e.type=type; e.dir=QKS_DIR_SLAVE; e.priority=priority_of(type); e.stage=QKS_STAGE_IWAIT; e.payload=payload;
    // 直接安装到执行区，并立即硬阻塞（IWAIT）：表明已接纳请求、等待主端 ACT
    qks_scheduler_set_exec_slot(s, &e, QKS_STAGE_IWAIT, 1);
    printf("[从初始化] 接受并占位: 类型=%d; 阶段=IWAIT; 硬阻塞=开启\n", (int)type);

    // 发送 ACK：统一走 Zigbee 占位发送
    (void)qks_zigbee_send((const uint8_t*)"INIT_ACK", 8);
    return 0;
}

// 收到 ACT：
// - 将阶段切换为 EXEC，并维持硬阻塞；
// - 唤醒执行分发线程进入实际事件执行。
int qks_init_slave_on_act(qks_sched_ctx_t* s){
    // 可选：回传 ACT_OK（占位），统一走 Zigbee；真实实现可能由执行线程或控制平面发送
    (void)qks_zigbee_send((const uint8_t*)"ACT_OK", 6);

    // 切换到 Exec 并继续硬阻塞，唤醒对应执行线程
    qks_scheduler_update_stage(s, QKS_STAGE_EXEC);
    qks_scheduler_set_blocking(s, 1);
    printf("[从初始化] 收到 ACT; 切换到 EXEC; 硬阻塞保持开启\n");
    qks_exec_signal(s, s->q.exec_event.type);
    return 0;
}

// 拒绝（忙碌）：占位（当前仅返回成功，实际系统应发送“忙碌”响应）。
int qks_init_slave_reject_busy(qks_sched_ctx_t* s){ (void)s; return 0; }
// 取消从初始化：占位（当前不含实际逻辑）。
int qks_init_slave_cancel(qks_sched_ctx_t* s){ (void)s; return 0; }
