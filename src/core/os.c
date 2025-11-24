#include "qkeysys/os.h"
#include <stdlib.h>

#ifdef QKS_OS_WINDOWS
#include <windows.h>
typedef struct { CRITICAL_SECTION cs; } win_mutex_t;
// 兼容旧版 MinGW：使用 Event 模拟条件变量（简化实现）
typedef struct { HANDLE ev; } win_cond_t;

// 将 qks_thread_fn 适配为 Windows 线程入口（DWORD WINAPI）
typedef struct {
    qks_thread_fn fn;
    void*         arg;
} win_thread_start_t;

static DWORD WINAPI qks_win_thread_start(LPVOID param){
    win_thread_start_t* s = (win_thread_start_t*)param;
    qks_thread_fn f = s->fn;
    void* a = s->arg;
    free(s);
    (void)f(a);
    return 0;
}
#else
#include <pthread.h>
#include <time.h>
typedef struct { pthread_mutex_t mu; } posix_mutex_t;
typedef struct { pthread_cond_t cv; } posix_cond_t;
#endif

// 创建线程：
// - Windows: 以包装启动函数适配 LPTHREAD_START_ROUTINE；
// - POSIX  : 直接 pthread_create，返回 pthread_t 指针；
// 返回 0 表示成功，非 0 表示失败。
int qks_thread_create(qks_thread_t* t, qks_thread_fn fn, void* arg){
#ifdef QKS_OS_WINDOWS
    win_thread_start_t* pack = (win_thread_start_t*)malloc(sizeof(win_thread_start_t));
    if(!pack) return -1;
    pack->fn = fn;
    pack->arg = arg;
    HANDLE h = CreateThread(NULL, 0, qks_win_thread_start, pack, 0, NULL);
    if(!h){
        free(pack);
        return -1;
    }
    *t = h;
    return 0;
#else
    pthread_t* th = (pthread_t*)malloc(sizeof(pthread_t));
    if(!th) return -1;
    int r = pthread_create(th, NULL, fn, arg);
    if(r){
        free(th);
        return -1;
    }
    *t = th;
    return 0;
#endif
}

// 等待线程退出并回收资源：
// - Windows：使用 WaitForSingleObject 等待，再 CloseHandle 关闭句柄；
// - POSIX ：使用 pthread_join 等待，并释放包装的 pthread_t 指针。
int qks_thread_join(qks_thread_t t){
#ifdef QKS_OS_WINDOWS
    WaitForSingleObject((HANDLE)t, INFINITE);
    CloseHandle((HANDLE)t);
    return 0;
#else
    pthread_t* th = (pthread_t*)t;
    if(!th) return -1;
    pthread_join(*th, NULL);
    free(th);
    return 0;
#endif
}

// 强制终止线程：
// - Windows 使用 TerminateThread（随后 CloseHandle）；
// - POSIX(Linux) 使用 pthread_cancel + pthread_join（建议目标线程开启异步可取消）。
// 注意：这是“占位实现”，不建议用于生产；目标线程未必能安全释放资源，真实项目应优先使用退出标志/可取消点。
int qks_thread_kill(qks_thread_t t){
#ifdef QKS_OS_WINDOWS
    if(!t) return 0;
    TerminateThread((HANDLE)t, 0);
    CloseHandle((HANDLE)t);
    return 0;
#else
    if(!t) return 0;
    pthread_t* th = (pthread_t*)t;
    int r = pthread_cancel(*th);
    // 无论取消是否成功，都进行 join 以回收资源
    pthread_join(*th, NULL);
    free(th);
    (void)r;
    return 0;
#endif
}

// 初始化互斥量包装。
int qks_mutex_init(qks_mutex_t* m){
#ifdef QKS_OS_WINDOWS
    win_mutex_t* wm=(win_mutex_t*)malloc(sizeof(win_mutex_t)); if(!wm) return -1; InitializeCriticalSection(&wm->cs); m->h=wm; return 0;
#else
    posix_mutex_t* pm=(posix_mutex_t*)malloc(sizeof(posix_mutex_t)); if(!pm) return -1; pthread_mutex_init(&pm->mu,NULL); m->h=pm; return 0;
#endif
}
// 加锁包装。
int qks_mutex_lock(qks_mutex_t* m){
#ifdef QKS_OS_WINDOWS
    EnterCriticalSection(&((win_mutex_t*)m->h)->cs); return 0;
#else
    return pthread_mutex_lock(&((posix_mutex_t*)m->h)->mu);
#endif
}
// 解锁包装。
int qks_mutex_unlock(qks_mutex_t* m){
#ifdef QKS_OS_WINDOWS
    LeaveCriticalSection(&((win_mutex_t*)m->h)->cs); return 0;
#else
    return pthread_mutex_unlock(&((posix_mutex_t*)m->h)->mu);
#endif
}
// 销毁互斥量包装。
int qks_mutex_destroy(qks_mutex_t* m){
#ifdef QKS_OS_WINDOWS
    win_mutex_t* wm=(win_mutex_t*)m->h; if(!wm) return 0; DeleteCriticalSection(&wm->cs); free(wm); m->h=NULL; return 0;
#else
    posix_mutex_t* pm=(posix_mutex_t*)m->h; if(!pm) return 0; pthread_mutex_destroy(&pm->mu); free(pm); m->h=NULL; return 0;
#endif
}

// 初始化条件变量（Windows 用 Event 简化实现；POSIX 用 pthread_cond）。
int qks_cond_init(qks_cond_t* c){
#ifdef QKS_OS_WINDOWS
    win_cond_t* wc=(win_cond_t*)malloc(sizeof(win_cond_t)); if(!wc) return -1; wc->ev = CreateEvent(NULL, TRUE, FALSE, NULL); if(!wc->ev){ free(wc); return -1; } c->h=wc; return 0;
#else
    posix_cond_t* pc=(posix_cond_t*)malloc(sizeof(posix_cond_t)); if(!pc) return -1; pthread_cond_init(&pc->cv,NULL); c->h=pc; return 0;
#endif
}
// 等待条件：调用方必须已持有互斥量 m；
// - Windows: 释放锁→等待事件→复位→重新加锁；
// - POSIX  : pthread_cond_wait。
int qks_cond_wait(qks_cond_t* c, qks_mutex_t* m){
#ifdef QKS_OS_WINDOWS
    // 释放锁→等待事件→复位→重新加锁（简化版条件变量）
    LeaveCriticalSection(&((win_mutex_t*)m->h)->cs);
    WaitForSingleObject(((win_cond_t*)c->h)->ev, INFINITE);
    ResetEvent(((win_cond_t*)c->h)->ev);
    EnterCriticalSection(&((win_mutex_t*)m->h)->cs);
    return 0;
#else
    return pthread_cond_wait(&((posix_cond_t*)c->h)->cv, &((posix_mutex_t*)m->h)->mu);
#endif
}
// 唤醒一个等待者。
int qks_cond_signal(qks_cond_t* c){
#ifdef QKS_OS_WINDOWS
    SetEvent(((win_cond_t*)c->h)->ev); return 0;
#else
    return pthread_cond_signal(&((posix_cond_t*)c->h)->cv);
#endif
}
// 唤醒所有等待者。
int qks_cond_broadcast(qks_cond_t* c){
#ifdef QKS_OS_WINDOWS
    // 简化：同 signal
    SetEvent(((win_cond_t*)c->h)->ev); return 0;
#else
    return pthread_cond_broadcast(&((posix_cond_t*)c->h)->cv);
#endif
}
// 销毁条件变量包装。
int qks_cond_destroy(qks_cond_t* c){
#ifdef QKS_OS_WINDOWS
    win_cond_t* wc=(win_cond_t*)c->h; if(!wc) return 0; CloseHandle(wc->ev); free(wc); c->h=NULL; return 0;
#else
    posix_cond_t* pc=(posix_cond_t*)c->h; if(!pc) return 0; pthread_cond_destroy(&pc->cv); free(pc); c->h=NULL; return 0;
#endif
}

// 毫秒级睡眠：Windows 用 Sleep，POSIX 用 nanosleep。
void qks_sleep_ms(uint32_t ms){
#ifdef QKS_OS_WINDOWS
    Sleep(ms);
#else
    struct timespec ts; ts.tv_sec = ms/1000; ts.tv_nsec = (ms%1000)*1000000L; nanosleep(&ts,NULL);
#endif
}
