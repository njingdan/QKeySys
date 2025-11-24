#ifndef QKEYSYS_QUEUE_H
#define QKEYSYS_QUEUE_H

#include "event.h"
#include "os.h"

typedef struct qks_qnode_s {
    qks_event_t ev;
    struct qks_qnode_s* next;
} qks_qnode_t;

// 队列与守卫状态（优化版：FIFO 接入缓冲 + 等待堆）
typedef struct {
    // FIFO 接入缓冲：新事件先入此队，调度器消费或转入等待堆
    qks_qnode_t* fifo_head;
    qks_qnode_t* fifo_tail;

    // 等待区：小顶堆，按 (priority asc, seq asc)
    qks_event_t* wait_heap;
    int          wait_size;
    int          wait_cap;

    // 执行区
    int          has_exec;      // 是否占用执行区
    qks_event_t  exec_event;    // 执行区事件（单槽）

    // 守卫：
    int  hard_blocking;         // 硬阻塞（Exec 或 SLAVE InitWaitAct）
    int  guard_priority;        // 软阻塞时的守卫优先级（仅允许更高优先级抢占）

    uint64_t     seq_counter;   // 入队序号

    // 并发控制
    qks_mutex_t  mu;
    qks_cond_t   cv;
} qks_queue_t;

int qks_queue_init(qks_queue_t* q);
int qks_queue_destroy(qks_queue_t* q);

// 入队/出队
// FIFO 接口（新事件接入）
int qks_queue_push_ready(qks_queue_t* q, const qks_event_t* e);            // 入 FIFO（首次入队分配 seq）
int qks_queue_peek_fifo(qks_queue_t* q, /*out*/ qks_event_t* e);           // 仅查看 FIFO 头，若无返回非0
int qks_queue_pop_ready(qks_queue_t* q, /*out*/ qks_event_t* e);           // 出 FIFO 头，若无返回非0

// 等待堆接口（阻塞/不可抢占事件进入此区）
int qks_queue_push_wait(qks_queue_t* q, const qks_event_t* e);             // 入等待堆（保持 e->seq）
int qks_queue_peek_wait(qks_queue_t* q, /*out*/ qks_event_t* e);           // 查看堆顶，若无返回非0
int qks_queue_pop_wait(qks_queue_t* q, /*out*/ qks_event_t* e);            // 出堆顶，若无返回非0
int qks_queue_is_empty(qks_queue_t* q);

// 执行区与守卫
int qks_queue_set_exec_slot(qks_queue_t* q, const qks_event_t* e, qks_stage_t stage, int hard_block);
int qks_queue_clear_exec_slot(qks_queue_t* q);
int qks_queue_set_blocking(qks_queue_t* q, int hard_block);
int qks_queue_update_stage(qks_queue_t* q, qks_stage_t stage);
int qks_queue_get_guard(qks_queue_t* q, /*out*/ int* hard_blocking, /*out*/ int* guard_priority);

// 等待/唤醒
// 当 READY 队列为空时阻塞等待；当 *stop_flag 为非零时立即返回（用于安全退出）
int qks_queue_wait_nonempty(qks_queue_t* q, volatile int* stop_flag);
// 主动唤醒等待 READY 的调度线程（例如停止或外部事件）
int qks_queue_wake(qks_queue_t* q);

// 仅等待 FIFO 非空（用于“执行区占用”阶段的沉睡策略）：
// 当 FIFO 为空时阻塞等待；当 *stop_flag 为非零时立即返回。
int qks_queue_wait_for_fifo(qks_queue_t* q, volatile int* stop_flag);

#endif /* QKEYSYS_QUEUE_H */
