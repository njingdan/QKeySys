// 主初始化（MASTER Init）
// - 当调度器将 MASTER 事件装入执行区（stage=INIT）后，拉起本线程；
// - 线程模拟 INIT_REQ→等待 ACK（提供可抢占窗口）→发送 ACT；
// - 发送 ACT 后：切换执行区为 EXEC，并开启硬阻塞（防止被打断），同时唤醒执行分发线程。
#include "qkeysys/init_master.h"
#include "qkeysys/exec.h"
#include "qkeysys/queue.h"
#include "qkeysys/os.h"
#include "config/timeouts.h"
#include "qkeysys/log.h"
#include "comm/zigbee/zigbee.h"  // 统一通信入口：占位 Zigbee 收发
#include <stdio.h>

// 启动主初始化：
// - 用短循环 sleep 模拟等待 ACK 的时延；此期间若发生抢占，调度器会杀死该线程；
int qks_init_master_start(qks_sched_ctx_t* s, const qks_event_t* e){
    // 发送 INIT_REQ：统一走 Zigbee 占位发送；真实实现后由 Zigbee 层承载实际收发
    // 当前阶段仍采用“本线程轮询占位接收”来模拟 ACK 到达，以保持可抢占窗口
    (void)e;
    printf("[主初始化] 发送 INIT_REQ, 等待 ACK ...\n");
    (void)qks_zigbee_send((const uint8_t*)"INIT_REQ", 8);

    // 等待 ACK：占位做法——调用 Zigbee 的占位接收（内部短暂 sleep）若干次，后续要改这个循环
    // 抢占时调度器会 kill 本线程；在 Windows 使用 TerminateThread，Linux 使用 pthread_cancel
    int elapsed = 0;
    while(elapsed < QKS_ACK_TIMEOUT_MS){
        if(s->stop) return 0;
        (void)qks_zigbee_recv(NULL, 0, NULL); // 占位接收：实际应由监听线程解析并回调 on_ack
        elapsed += 60; // 与占位接收中的 sleep 对齐，近似累计等待时长
    }

    // 模拟“收到 ACK”：后续由监听线程接入后改为回调 qks_init_master_on_ack
    printf("[主初始化] 收到 ACK, 发送 ACT\n");
    return qks_init_master_send_act(s);
}

// 收到 ACK 的占位回调（当前不含实际逻辑，保留扩展点）。
int qks_init_master_on_ack(qks_sched_ctx_t* s){ (void)s; return 0; }

// 发送 ACT 并切 EXEC：
// - 更新执行区阶段为 EXEC；
// - 开启硬阻塞（后续直到执行完成期间不可被打断）；
// - 唤醒执行分发线程，进入具体事件的工作线程。
int qks_init_master_send_act(qks_sched_ctx_t* s){
    // 发送 ACT：统一走 Zigbee 占位发送
    // 说明：真实实现中建议将“发送 ACT → 等从端 on_act”改由 Zigbee 监听线程驱动
    (void)qks_zigbee_send((const uint8_t*)"ACT", 3);

    // 切换到 Exec 并设置硬阻塞，然后唤醒对应执行线程
    qks_scheduler_update_stage(s, QKS_STAGE_EXEC);
    qks_scheduler_set_blocking(s, 1);
    printf("[主初始化] 切换到 EXEC; 硬阻塞=开启\n");
    qks_exec_signal(s, s->q.exec_event.type);
    return 0;
}

// 取消主初始化：占位（当前不含实际逻辑，抢占路径由调度器直接杀线程）。
int qks_init_master_cancel(qks_sched_ctx_t* s){ (void)s; return 0; }
