#include "qkeysys/queue.h"
#include <stdlib.h>
#include <stdio.h>

static int cmp(const qks_event_t* a, const qks_event_t* b){
    if(a->priority != b->priority) return (a->priority < b->priority) ? -1 : 1; // 小值优先
    if(a->seq != b->seq) return (a->seq < b->seq) ? -1 : 1; // 先入先出
    return 0;
}

// 等待堆（最小堆）实现
static void heap_swap(qks_event_t* h, int i, int j){ qks_event_t t=h[i]; h[i]=h[j]; h[j]=t; }
static void heap_up(qks_event_t* h, int idx){
    while(idx>0){ int p=(idx-1)/2; if(cmp(&h[idx], &h[p])>=0) break; heap_swap(h, idx, p); idx=p; }
}
static void heap_down(qks_event_t* h, int n, int idx){
    while(1){ int l=idx*2+1, r=l+1, m=idx; if(l<n && cmp(&h[l], &h[m])<0) m=l; if(r<n && cmp(&h[r], &h[m])<0) m=r; if(m==idx) break; heap_swap(h, idx, m); idx=m; }
}
static int heap_push(qks_queue_t* q, const qks_event_t* e){
    if(q->wait_size+1 > q->wait_cap){ int nc = q->wait_cap? q->wait_cap*2 : 16; qks_event_t* nh=(qks_event_t*)realloc(q->wait_heap, sizeof(qks_event_t)*nc); if(!nh) return -1; q->wait_heap=nh; q->wait_cap=nc; }
    q->wait_heap[q->wait_size] = *e; heap_up(q->wait_heap, q->wait_size); q->wait_size++; return 0;
}
static int heap_peek(qks_queue_t* q, qks_event_t* e){ if(q->wait_size<=0) return -1; *e=q->wait_heap[0]; return 0; }
static int heap_pop(qks_queue_t* q, qks_event_t* e){ if(q->wait_size<=0) return -1; *e=q->wait_heap[0]; q->wait_size--; if(q->wait_size>0){ q->wait_heap[0]=q->wait_heap[q->wait_size]; heap_down(q->wait_heap, q->wait_size, 0);} return 0; }

// 初始化队列/执行区：
// - FIFO 接入缓冲置空；等待堆容量/大小置 0；
// - 执行区清空；守卫与硬阻塞复位；
// - 初始化互斥量与条件变量。
int qks_queue_init(qks_queue_t* q){
    q->fifo_head=NULL; q->fifo_tail=NULL; q->wait_heap=NULL; q->wait_size=0; q->wait_cap=0;
    q->has_exec=0; q->hard_blocking=0; q->guard_priority=0; q->seq_counter=0;
    qks_mutex_init(&q->mu); qks_cond_init(&q->cv); return 0;
}

// 销毁队列：
// - 释放 FIFO 链表与等待堆内存；
// - 清理互斥量与条件变量。
int qks_queue_destroy(qks_queue_t* q){
    qks_mutex_lock(&q->mu);
    qks_qnode_t* p=q->fifo_head; while(p){ qks_qnode_t* t=p->next; free(p); p=t; }
    q->fifo_head=q->fifo_tail=NULL;
    if(q->wait_heap){ free(q->wait_heap); q->wait_heap=NULL; }
    q->has_exec=0;
    qks_mutex_unlock(&q->mu);
    qks_mutex_destroy(&q->mu); qks_cond_destroy(&q->cv); return 0;
}

// 入队（FIFO）：
// - 首次入队的事件分配自增 seq；
// - 尾插到 FIFO，并 signal 唤醒调度器。
int qks_queue_push_ready(qks_queue_t* q, const qks_event_t* e){
    qks_mutex_lock(&q->mu);
    qks_event_t ev=*e; if(ev.seq==0) ev.seq=++q->seq_counter;
    qks_qnode_t* n=(qks_qnode_t*)malloc(sizeof(qks_qnode_t)); if(!n){ qks_mutex_unlock(&q->mu); return -1; }
    n->ev=ev; n->next=NULL;
    if(!q->fifo_tail){ q->fifo_head=q->fifo_tail=n; } else { q->fifo_tail->next=n; q->fifo_tail=n; }
    qks_cond_signal(&q->cv);
    qks_mutex_unlock(&q->mu);
    printf("[队列] FIFO 入队: 类型=%d 优先级=%d 序号=%llu\n", (int)e->type, e->priority, (unsigned long long)ev.seq);
    return 0;
}

// 查看 FIFO 头（不弹出）。
int qks_queue_peek_fifo(qks_queue_t* q, qks_event_t* e){ qks_mutex_lock(&q->mu); if(!q->fifo_head){ qks_mutex_unlock(&q->mu); return -1; } *e=q->fifo_head->ev; qks_mutex_unlock(&q->mu); return 0; }
// 弹出 FIFO 头（若为空返回非 0）。
int qks_queue_pop_ready(qks_queue_t* q, qks_event_t* e){
    qks_mutex_lock(&q->mu);
    if(!q->fifo_head){ qks_mutex_unlock(&q->mu); return -1; }
    qks_qnode_t* n=q->fifo_head; q->fifo_head=n->next; if(!q->fifo_head) q->fifo_tail=NULL; *e=n->ev; 
    printf("[队列] FIFO 出队: 类型=%d 优先级=%d 序号=%llu\n", (int)e->type, e->priority, (unsigned long long)e->seq);
    free(n);
    qks_mutex_unlock(&q->mu); return 0;
}

// 入等待堆（仅 MASTER）：保持原 priority/seq。
int qks_queue_push_wait(qks_queue_t* q, const qks_event_t* e){
    qks_mutex_lock(&q->mu);
    qks_event_t ev=*e; // 保留原 seq/priority
    heap_push(q, &ev);
    qks_mutex_unlock(&q->mu);
    printf("[队列] 等待堆 入队: 类型=%d 优先级=%d 序号=%llu\n", (int)e->type, e->priority, (unsigned long long)e->seq);
    return 0;
}
// 查看等待堆顶（不弹出）。
int qks_queue_peek_wait(qks_queue_t* q, qks_event_t* e){ qks_mutex_lock(&q->mu); int r=heap_peek(q,e); qks_mutex_unlock(&q->mu); return r; }
// 弹出等待堆顶。
int qks_queue_pop_wait(qks_queue_t* q, qks_event_t* e){ qks_mutex_lock(&q->mu); int r=heap_pop(q,e); if(r==0) printf("[队列] 等待堆 出队: 类型=%d 优先级=%d 序号=%llu\n", (int)e->type, e->priority, (unsigned long long)e->seq); qks_mutex_unlock(&q->mu); return r; }

// 队列是否为空（FIFO 与等待堆均为空）。
int qks_queue_is_empty(qks_queue_t* q){ qks_mutex_lock(&q->mu); int empty = (!q->fifo_head && q->wait_size==0); qks_mutex_unlock(&q->mu); return empty; }

int qks_queue_set_exec_slot(qks_queue_t* q, const qks_event_t* e, qks_stage_t stage, int hard_block){
    qks_mutex_lock(&q->mu);
    q->exec_event = *e; q->exec_event.stage=stage; q->has_exec=1; q->hard_blocking=hard_block ? 1 : 0; q->guard_priority=e->priority;
    qks_mutex_unlock(&q->mu);
    printf("[调度] 安装到执行区(初始化): 类型=%d 方向=%d(1=主,2=从) 优先级=%d 守卫=%d 硬阻塞=否\n", (int)e->type, (int)e->dir, e->priority, e->priority);
    return 0;
}
int qks_queue_clear_exec_slot(qks_queue_t* q){
    qks_mutex_lock(&q->mu);
    q->has_exec=0; q->hard_blocking=0; q->guard_priority=0;
    qks_mutex_unlock(&q->mu);
    printf("[调度] 执行区清空; 优先级守卫已释放\n");
    return 0;
}
int qks_queue_set_blocking(qks_queue_t* q, int hard_block){
    qks_mutex_lock(&q->mu); q->hard_blocking = hard_block ? 1 : 0; qks_mutex_unlock(&q->mu);
    printf("[调度] 硬阻塞 %s\n", hard_block?"已开启":"已关闭");
    return 0;
}
int qks_queue_update_stage(qks_queue_t* q, qks_stage_t stage){
    qks_mutex_lock(&q->mu); if(q->has_exec) q->exec_event.stage = stage; qks_mutex_unlock(&q->mu);
    printf("[调度] 阶段切换 -> %d (1=INIT,2=IWAIT,3=EXEC,4=DONE,5=ABORT)\n", (int)stage);
    return 0;
}
int qks_queue_get_guard(qks_queue_t* q, int* hard_blocking, int* guard_priority){
    qks_mutex_lock(&q->mu);
    if(hard_blocking) *hard_blocking = q->hard_blocking;
    if(guard_priority) *guard_priority = q->guard_priority;
    qks_mutex_unlock(&q->mu); return 0;
}

// 当 READY 为空时等待；stop_flag 为非零时提前返回
// 等待直到“FIFO 或 等待堆”有事件或收到停止信号；适用于“执行区空闲”场景。
int qks_queue_wait_nonempty(qks_queue_t* q, volatile int* stop_flag){
    qks_mutex_lock(&q->mu);
    while((!stop_flag || *stop_flag==0) && q->fifo_head == NULL && q->wait_size==0){
        qks_cond_wait(&q->cv, &q->mu);
    }
    qks_mutex_unlock(&q->mu);
    return 0;
}

// 主动唤醒等待 READY 的线程
// 广播唤醒所有等待在队列条件变量上的线程（新事件入队或执行完成等）。
int qks_queue_wake(qks_queue_t* q){ qks_mutex_lock(&q->mu); qks_cond_broadcast(&q->cv); qks_mutex_unlock(&q->mu); return 0; }

// 仅等待 FIFO 非空（用于执行区占用阶段，忽略等待堆是否有元素）
// 等待直到 FIFO 非空或收到停止信号；适用于“执行区占用”场景（忽略等待堆）。
int qks_queue_wait_for_fifo(qks_queue_t* q, volatile int* stop_flag){
    qks_mutex_lock(&q->mu);
    while((!stop_flag || *stop_flag==0) && q->fifo_head == NULL){
        qks_cond_wait(&q->cv, &q->mu);
    }
    qks_mutex_unlock(&q->mu);
    return 0;
}
