// 执行分发 + 专用工作线程模型
#include "qkeysys/exec.h"
#include "comm/zigbee/zigbee.h"
#include "modules/keydist/keydist.h"
#include "comm/usrp/usrp.h"
#include "modules/auth/auth.h"
#include "modules/keypool/keypool.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 将“事件类型 × 角色(A/B)”映射为 0..5 的工作线程索引：
// 0/1: AUTH-A/B，2/3: CLOCK-A/B，4/5: KEYREFILL-A/B。
static int worker_index(qks_event_type_t t, qks_role_t r){
    int ti = (t==QKS_EVENT_DEVICE_AUTH?0:(t==QKS_EVENT_CLOCK_SYNC?1:2));
    int ri = (r==QKS_ROLE_B?1:0);
    return ti*2 + ri; // 0..5
}

// 专用工作线程：
// - 每个线程负责一种“事件类型 × 角色”的执行占位流程；
// - 被分发线程唤醒后，读取当前执行区事件并执行占位步骤，最后通知分发线程“完成”。
static void* worker_thread(void* arg){
    // 线程启动参数说明：
    // 这里通过一个简易打包的指针数组传入 {s, idx}，避免单独定义结构体文件；
    // idx 用于从 0..5 解码出 (事件类型×角色) 的专用工作线程身份。
    void** arr = (void**)arg;
    qks_sched_ctx_t* s = (qks_sched_ctx_t*)arr[0];
    int idx = (int)(size_t)arr[1];
    free(arr);
    // 从 idx 解码事件类型与角色（A/B）
    int ti = idx/2; int ri = idx%2;
    qks_event_type_t type = (ti==0?QKS_EVENT_DEVICE_AUTH:(ti==1?QKS_EVENT_CLOCK_SYNC:QKS_EVENT_KEY_REFILL));
    qks_role_t role = (ri==0?QKS_ROLE_A:QKS_ROLE_B);

    for(;;){
        // 等待被分发线程唤醒
        qks_mutex_lock(&s->worker_sig[idx].mu);
        while(!s->worker_sig[idx].wake && !s->stop){ qks_cond_wait(&s->worker_sig[idx].cv, &s->worker_sig[idx].mu); }
        if(s->stop){ qks_mutex_unlock(&s->worker_sig[idx].mu); break; }
        s->worker_sig[idx].wake = 0;
        qks_mutex_unlock(&s->worker_sig[idx].mu);

        // 根据事件类型执行占位流程（仅打印/睡眠，便于观察时序）
        if(type == QKS_EVENT_DEVICE_AUTH){
            printf("[执行-认证-%s] 开始执行\n", role==QKS_ROLE_A?"A":"B");
            char token[64] = {0};
            if(role == QKS_ROLE_A){
                // A 侧：USRP 采集 → 文件确认 → 特征比对 → 令牌生成 → Zigbee 发送令牌
                qks_usrp_collect_info("A");
                qks_usrp_file_confirm();
                qks_auth_feature_compare();
                qks_auth_token_generate(token, sizeof(token));
                qks_zigbee_send((const uint8_t*)token, strlen(token));
            } else {
                // B 侧：USRP 采集 → 文件确认 → 特征比对 → （占位）令牌生成 → Zigbee 接收令牌
                qks_usrp_collect_info("B");
                qks_usrp_file_confirm();
                qks_auth_feature_compare();
                qks_auth_token_generate(token, sizeof(token)); // 占位：本地生成用于演示流程对称性
                qks_zigbee_recv(NULL, 0, NULL);
            }
            printf("[执行-认证-%s] 完成\n", role==QKS_ROLE_A?"A":"B");
        } else if(type == QKS_EVENT_CLOCK_SYNC){
            printf("[执行-时钟同步-%s] 开始执行\n", role==QKS_ROLE_A?"A":"B");
            qks_sleep_ms(50);
            printf("[执行-时钟同步-%s] 完成\n", role==QKS_ROLE_A?"A":"B");
        } else if(type == QKS_EVENT_KEY_REFILL){
            static int session_no = 0; session_no++;
            char qk[64]={0}, k[64]={0}, cipher[128]={0}, qk_out[64]={0};
            if(role == QKS_ROLE_A){
                qks_keydist_get_quantum_key(qk, sizeof(qk), "A");
                qks_keydist_get_agreed_key(k, sizeof(k), session_no);
                qks_keydist_encrypt(k, qk, cipher, sizeof(cipher));
                qks_zigbee_send((const uint8_t*)"QK_READY", 8);
                qks_zigbee_recv(NULL, 0, NULL); // 占位：等待 QK_ACK
                qks_zigbee_send((const uint8_t*)cipher, (size_t)strlen(cipher));
                qks_zigbee_recv(NULL, 0, NULL); // 占位：等待 DEC_OK
                qks_keypool_update(qk); // A 侧密钥池更新（占位）
                qks_zigbee_send((const uint8_t*)"KP_ACK", 6);
                qks_zigbee_recv(NULL, 0, NULL); // 占位：等待 DONE
                qks_zigbee_send((const uint8_t*)"FINAL_ACK", 9);
            } else { // B 侧流程
                qks_zigbee_recv(NULL, 0, NULL); // 占位：等待 QK_READY
                qks_zigbee_send((const uint8_t*)"QK_ACK", 6);
                qks_keydist_get_agreed_key(k, sizeof(k), session_no);
                qks_zigbee_recv(NULL, 0, NULL); // 占位：等待 ENC_MSG
                qks_keydist_decrypt(k, "CIPH(K+QK)", qk_out, sizeof(qk_out));
                qks_zigbee_send((const uint8_t*)"DEC_OK", 6);
                qks_keypool_update(qk_out); // B 侧密钥池更新（占位）
                qks_zigbee_send((const uint8_t*)"DONE", 4);
                qks_zigbee_recv(NULL, 0, NULL); // 占位：等待 FINAL_ACK
            }
            printf("[执行-密钥分发-%s] 完成\n", role==QKS_ROLE_A?"A":"B");
        }

        // 通知分发线程：本次执行已完成
        qks_mutex_lock(&s->exec_sig.mu);
        s->exec_sig.worker_done = 1;
        qks_cond_signal(&s->exec_sig.cv_done);
        qks_mutex_unlock(&s->exec_sig.mu);
    }
    return NULL;
}

// 执行分发线程：
// - 接收 qks_exec_signal 唤醒，选择对应工作线程唤醒；
// - 等待工作线程完成后，统一回调调度器“事件完成”；
// - 不直接执行业务逻辑，仅做分发与同步。
static void* dispatcher_thread(void* arg){
    qks_sched_ctx_t* s=(qks_sched_ctx_t*)arg;
    for(;;){
        qks_mutex_lock(&s->exec_sig.mu);
        while(!s->exec_sig.wake && !s->stop){ qks_cond_wait(&s->exec_sig.cv, &s->exec_sig.mu); }
        if(s->stop){ qks_mutex_unlock(&s->exec_sig.mu); break; }
        qks_event_type_t t = s->exec_sig.pending_type;
        qks_role_t r = s->exec_sig.pending_role;
        s->exec_sig.wake=0;
        qks_mutex_unlock(&s->exec_sig.mu);

        int idx = worker_index(t, r);
        printf("[执行-分发] 收到事件: type=%d role=%d -> 唤醒工作线程idx=%d\n", (int)t, (int)r, idx);
        qks_mutex_lock(&s->worker_sig[idx].mu);
        s->worker_sig[idx].wake = 1;
        qks_cond_signal(&s->worker_sig[idx].cv);
        qks_mutex_unlock(&s->worker_sig[idx].mu);

        // wait worker done
        qks_mutex_lock(&s->exec_sig.mu);
        while(!s->exec_sig.worker_done && !s->stop){ qks_cond_wait(&s->exec_sig.cv_done, &s->exec_sig.mu); }
        s->exec_sig.worker_done = 0;
        qks_mutex_unlock(&s->exec_sig.mu);

        // notify scheduler that execution is done
        qks_scheduler_notify_event_done(s, t, 0);
        printf("[执行-分发] 事件结束: type=%d; 已通知调度器\n", (int)t);
    }
    return NULL;
}

// 启动执行分发线程与 6 个专用工作线程；初始化相关互斥量与条件变量。
int qks_exec_threads_start(qks_sched_ctx_t* s){
    // 分发器的条件变量/互斥量初始化
    qks_mutex_init(&s->exec_sig.mu);
    qks_cond_init(&s->exec_sig.cv);
    qks_cond_init(&s->exec_sig.cv_done);
    s->exec_sig.wake=0; s->exec_sig.pending_type=0; s->exec_sig.pending_role=0; s->exec_sig.worker_done=0;
    // 各专用工作线程的条件变量与线程创建
    for(int i=0;i<6;i++){
        qks_mutex_init(&s->worker_sig[i].mu);
        qks_cond_init(&s->worker_sig[i].cv);
        s->worker_sig[i].wake=0;
        // 打包参数 {s, idx} 传入工作线程
        void** pack = (void**)malloc(sizeof(void*)*2);
        pack[0]=s; pack[1]=(void*)(size_t)i;
        qks_thread_create(&s->th_worker[i], worker_thread, pack);
    }
    // 创建分发器线程
    qks_thread_create(&s->th_exec, dispatcher_thread, s);
    return 0;
}

// 停止执行分发与所有工作线程：
// - 置全局 stop；
// - 广播唤醒等待；
// - 等待并回收所有线程资源。
int qks_exec_threads_stop(qks_sched_ctx_t* s){
    qks_mutex_lock(&s->exec_sig.mu);
    s->stop=1; qks_cond_broadcast(&s->exec_sig.cv); qks_cond_broadcast(&s->exec_sig.cv_done);
    qks_mutex_unlock(&s->exec_sig.mu);
    for(int i=0;i<6;i++){
        qks_mutex_lock(&s->worker_sig[i].mu);
        qks_cond_broadcast(&s->worker_sig[i].cv);
        qks_mutex_unlock(&s->worker_sig[i].mu);
    }
    if(s->th_exec) qks_thread_join(s->th_exec);
    for(int i=0;i<6;i++) if(s->th_worker[i]) qks_thread_join(s->th_worker[i]);
    return 0;
}

// 分发唤醒信号：设置本次要执行的事件类型与当前角色，并唤醒分发线程。
int qks_exec_signal(qks_sched_ctx_t* s, qks_event_type_t type){
    qks_mutex_lock(&s->exec_sig.mu);
    s->exec_sig.pending_type = type;
    s->exec_sig.pending_role = s->cfg.role; // 使用当前系统角色
    s->exec_sig.wake = 1;
    qks_cond_signal(&s->exec_sig.cv);
    qks_mutex_unlock(&s->exec_sig.mu);
    return 0;
}
