#include "modules/keydist/keydist.h"
#include <stdio.h>
#include <string.h>

// 纯占位实现：不依赖其他模块，便于结构清晰；
// 仅生成/解析可读字符串并打印“已调用”，后续可替换为真实算法。

int qks_keydist_get_quantum_key(char* out, size_t cap, const char* role_hint){
    if(!out || cap==0) return -1;
    printf("[KeyDist] 请求量子密钥 已调用 (role=%s)\n", role_hint?role_hint:"?");
    snprintf(out, cap, "QK_%s", role_hint?role_hint:"X");
    return 0;
}

int qks_keydist_get_agreed_key(char* out, size_t cap, int session_no){
    if(!out || cap==0) return -1;
    printf("[KeyDist] 协商密钥生成 已调用 (session=%d)\n", session_no);
    snprintf(out, cap, "K_AGREE_%d", session_no);
    return 0;
}

int qks_keydist_encrypt(const char* agreed_key, const char* quantum_key, char* out_cipher, size_t cap){
    if(!out_cipher || cap==0) return -1;
    printf("[KeyDist] 加密 已调用\n");
    snprintf(out_cipher, cap, "CIPH(%s+%s)", agreed_key?agreed_key:"K", quantum_key?quantum_key:"QK");
    return 0;
}

int qks_keydist_decrypt(const char* agreed_key, const char* cipher, char* out_quantum_key, size_t cap){
    if(!out_quantum_key || cap==0) return -1;
    printf("[KeyDist] 解密 已调用\n");
    (void)agreed_key;
    const char* p = cipher ? strstr(cipher, "+") : NULL;
    const char* q = cipher ? strrchr(cipher, ')') : NULL;
    if(cipher && p && q && q>p){
        size_t len = (size_t)(q - (p+1));
        if(len >= cap) len = cap-1;
        memcpy(out_quantum_key, p+1, len);
        out_quantum_key[len] = '\0';
    } else {
        snprintf(out_quantum_key, cap, "QK_RECOVERED");
    }
    return 0;
}
