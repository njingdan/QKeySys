// 调度器（Scheduler）核心实现
// 说明：
// - 本文件实现事件调度主循环与与执行区/守卫管理接口；
// - 采用“双队列”策略：
//   * FIFO 接入缓冲：所有新事件先入 FIFO，唤醒调度器；
//   * 等待堆（小顶堆）：仅主事件（MASTER）允许等待或被抢占回退；按(优先级 asc, seq asc)排序保障同优先级 FIFO；
// - 从事件（SLAVE）永不进入等待堆：
//   * 仅参与“立即决策”，若能立即执行（执行区空/或 MASTER Init 被更高优先级抢占）则接受；否则直接丢弃；
// - 抢占只在“MASTER Init 窗口”发生，由新到达（FIFO 头）触发；
// - 执行完成后不再 flush 等待区，仅清空执行区并唤醒调度器，让下一轮从 FIFO/等待堆中择优。
#include "qkeysys/scheduler.h"
#include "qkeysys/queue.h"
#include "qkeysys/init_master.h"
#include "qkeysys/init_slave.h"
#include "qkeysys/threads.h"
#include "qkeysys/log.h"
#include "qkeysys/os.h"
#include "config/timeouts.h"
#include <stdio.h>

// 说明：
// 本文件实现主调度循环的骨架逻辑与对执行区/守卫的管理接口。
// - 硬阻塞：Exec 或 SLAVE InitWaitAct，禁止任何事件进入执行区。
// - 软阻塞：MASTER Init，仅允许更高优先级事件抢占。
// - 抢占：仅在 MASTER Init 窗口，终止当前初始化线程。
// 实际的 INIT_REQ/ACK/ACT 流程交由 init_master/init_slave 占位实现。

// 是否允许抢占：仅比较优先级（数值越小优先级越高）。
static int can_preempt(int guard_priority, const qks_event_t* e){
    return e->priority < guard_priority; // 小数值=更高优先级
}

int qks_scheduler_set_exec_slot(qks_sched_ctx_t* s, const qks_event_t* e, qks_stage_t stage, int hard_blocking){
    return qks_queue_set_exec_slot(&s->q, e, stage, hard_blocking);
}
int qks_scheduler_clear_exec_slot(qks_sched_ctx_t* s){
    return qks_queue_clear_exec_slot(&s->q);
}
int qks_scheduler_set_blocking(qks_sched_ctx_t* s, int hard_blocking){
    return qks_queue_set_blocking(&s->q, hard_blocking);
}
int qks_scheduler_update_stage(qks_sched_ctx_t* s, qks_stage_t stage){
    return qks_queue_update_stage(&s->q, stage);
}
// 事件完成的统一回调：
// - 清空执行区与守卫；
// - 唤醒调度循环（不做回刷/搬运），下一轮由调度器在 FIFO/等待堆之间择优选择；
int qks_scheduler_notify_event_done(qks_sched_ctx_t* s, qks_event_type_t type, int result_code){
    (void)type; (void)result_code;
    qks_queue_clear_exec_slot(&s->q);
    // 通知等待中的调度线程检查 FIFO/等待堆选择下一个
    qks_queue_wake(&s->q);
    return 0;
}

int qks_scheduler_preempt_current(qks_sched_ctx_t* s){ return 0; }

// 安装事件到执行区并启动初始化线程：
// - MASTER：装入执行区（stage=INIT），设置软守卫，拉起主初始化线程；
// - SLAVE：装入执行区（stage=INIT）；SLAVE 的“接受+IWAIT+EXEC”在调度器路径里调用从初始化接口完成；
static void handle_event(qks_sched_ctx_t* s, const qks_event_t* e){
    // 将事件安装到执行区：MASTER→Init（软阻塞），SLAVE 可能直接进入 InitWaitAct（硬阻塞逻辑在从初始化中处理）
    qks_queue_set_exec_slot(&s->q, e, QKS_STAGE_INIT, 0);
    if(e->dir == QKS_DIR_MASTER){
        // 拉起主初始化线程（短生命周期）
        qks_init_master_spawn(s);
    }else{
        // 从初始化通常由监听触发；此处仅占位说明：当从端已接纳时会通过 accept_and_claim 直接硬阻塞
        // qks_init_slave_accept_and_claim(s, e->type, e->payload);
    }
}

// 调度主循环：
// - 等待“FIFO 非空或等待堆非空或 stop”；
// - 若硬阻塞=ON：继续等待；
// - 执行区空：在 FIFO 头 与 等待堆顶 之间择优；若选择 SLAVE 则立即接受并切 EXEC；
// - 执行区占用：仅在 MASTER Init 窗口由“新到达（FIFO 头）”触发抢占检查；
//   * MASTER 不可抢占：转入等待堆；SLAVE 不可抢占：直接丢弃；
int qks_scheduler_start(qks_sched_ctx_t* s){
    s->stop=0;

    // 条件变量驱动的调度循环：
    // - 当 READY 队列为空时在 q->cv 上等待；
    // - 新事件入队（push_ready）或执行完成（flush_buffer_to_ready）都会 signal/broadcast 唤醒这里；
    for(;;){
        if(s->stop) break;
        // 沉睡策略优化：
        // - 执行区空闲：等待“FIFO 或 等待堆”非空；
        // - 执行区占用：仅等待 FIFO 非空（新到达可能触发抢占）或等待执行完成的唤醒；
        if(!s->q.has_exec){
            qks_queue_wait_nonempty(&s->q, &s->stop);
        }else{
            // 快速检查 FIFO 是否为空，空则等待 FIFO；执行完成会通过 wake 唤醒这里
            qks_event_t _tmp; if(qks_queue_peek_fifo(&s->q, &_tmp)!=0){ qks_queue_wait_for_fifo(&s->q, &s->stop); }
        }
        if(s->stop) break;

        // 读取当前守卫/阻塞状态
        int hard_block=0, guard_pri=0; qks_queue_get_guard(&s->q, &hard_block, &guard_pri);

        // 若硬阻塞，继续等待（新事件仍在 FIFO/堆中排队）
        if(hard_block) continue;

        // 无执行区：在 FIFO 头与等待堆顶之间择优
        if(!s->q.has_exec){
            qks_event_t f, h; int has_f = (qks_queue_peek_fifo(&s->q, &f)==0); int has_h = (qks_queue_peek_wait(&s->q, &h)==0);
            if(!has_f && !has_h) continue; // 伪唤醒
            qks_event_t chosen; int from_fifo=0;
            int choose_fifo = 0;
            if(has_f && !has_h) choose_fifo = 1;
            else if(has_f && has_h){
                if(f.priority < h.priority) choose_fifo = 1;
                else if(f.priority == h.priority && f.seq < h.seq) choose_fifo = 1;
            }
            if(choose_fifo){ from_fifo=1; qks_queue_pop_ready(&s->q, &chosen); }
            else { qks_queue_pop_wait(&s->q, &chosen); }
            printf("[调度] 选择候选: 来自%s 类型=%d 优先级=%d\n", from_fifo?"FIFO":"等待堆", (int)chosen.type, chosen.priority);

            if(chosen.dir == QKS_DIR_SLAVE){
                // SLAVE：仅允许“立即执行”，否则丢弃
                // 此时执行区空闲且无硬阻塞，允许立即接受
                qks_init_slave_accept_and_claim(s, chosen.type, chosen.payload);
                qks_init_slave_on_act(s);
            } else {
                handle_event(s, &chosen);
            }
            continue;
        }

        // 已有执行区：只由新到达（FIFO 头）触发抢占检查，否则将 MASTER 新事件转入等待堆；SLAVE 直接丢弃
        qks_event_t f; if(qks_queue_peek_fifo(&s->q, &f)==0){
            int allow_preempt = 0;
            if(s->q.exec_event.stage==QKS_STAGE_INIT && s->q.exec_event.dir==QKS_DIR_MASTER){
                if(can_preempt(guard_pri, &f)) allow_preempt=1;
            }
            if(allow_preempt){
                // 抢占当前主初始化
                qks_init_master_kill_current(s);
                qks_event_t prev = s->q.exec_event;
                qks_queue_clear_exec_slot(&s->q);
                qks_queue_push_wait(&s->q, &prev); // 保留原 seq 回等待堆
                // 取走 FIFO 头作为新执行事件
                qks_queue_pop_ready(&s->q, &f);
                printf("[调度] 抢占: 新事件优先级=%d 高于守卫=%d; 原事件入等待堆 类型=%d\n", f.priority, guard_pri, (int)prev.type);
                if(f.dir == QKS_DIR_SLAVE){
                    // SLAVE 抢占：立即接受并切 EXEC
                    qks_init_slave_accept_and_claim(s, f.type, f.payload);
                    qks_init_slave_on_act(s);
                } else {
                    handle_event(s, &f);
                }
            } else {
                // 不可抢占
                qks_queue_pop_ready(&s->q, &f);
                if(f.dir == QKS_DIR_MASTER){
                    qks_queue_push_wait(&s->q, &f);
                    printf("[调度] 不可抢占: MASTER 新事件转入等待堆 类型=%d 守卫=%d\n", (int)f.type, guard_pri);
                } else {
                    // SLAVE 不排队：直接丢弃
                    printf("[调度] 不可抢占: SLAVE 新事件丢弃 类型=%d 守卫=%d\n", (int)f.type, guard_pri);
                }
            }
        } else {
            // 无新到达，保持现状等待即可
            continue;
        }
    }
    return 0;
}

int qks_scheduler_stop(qks_sched_ctx_t* s){ s->stop=1; return 0; }
