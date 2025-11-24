#ifndef QKEYSYS_TIMEOUTS_H
#define QKEYSYS_TIMEOUTS_H

// 超时与重试参数（集中定义，线程内部使用）
#define QKS_ACK_TIMEOUT_MS        500
#define QKS_ACK_RETRY_MAX         3
#define QKS_ACT_WAIT_TIMEOUT_MS   300
#define QKS_QUEUE_IDLE_SLEEP_MS    50

// 预留：执行期超时（按事件类型细化，后续实现）
#define QKS_EXEC_TIMEOUT_AUTH_MS     3000
#define QKS_EXEC_TIMEOUT_CLOCK_MS    1000
#define QKS_EXEC_TIMEOUT_KEYREFILL_MS 5000

#endif /* QKEYSYS_TIMEOUTS_H */

