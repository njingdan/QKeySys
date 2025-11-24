#ifndef QKEYSYS_OS_H
#define QKEYSYS_OS_H

#include <stdint.h>

typedef void* qks_thread_t;
typedef void* (*qks_thread_fn)(void*);

typedef struct { void* h; } qks_mutex_t;
typedef struct { void* h; } qks_cond_t;

int qks_thread_create(qks_thread_t* t, qks_thread_fn fn, void* arg);
int qks_thread_join(qks_thread_t t);
// 强制终止线程（占位实现）：
// - 在 POSIX 上使用 pthread_cancel + pthread_join；
// - 在 Windows 上使用 TerminateThread + CloseHandle。
int qks_thread_kill(qks_thread_t t);

int qks_mutex_init(qks_mutex_t* m);
int qks_mutex_lock(qks_mutex_t* m);
int qks_mutex_unlock(qks_mutex_t* m);
int qks_mutex_destroy(qks_mutex_t* m);

int qks_cond_init(qks_cond_t* c);
int qks_cond_wait(qks_cond_t* c, qks_mutex_t* m);
int qks_cond_signal(qks_cond_t* c);
int qks_cond_broadcast(qks_cond_t* c);
int qks_cond_destroy(qks_cond_t* c);

void qks_sleep_ms(uint32_t ms);

#endif /* QKEYSYS_OS_H */
