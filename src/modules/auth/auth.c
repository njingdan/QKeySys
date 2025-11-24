#include "modules/auth/auth.h"
#include <stdio.h>
#include <string.h>

// 设备认证执行期占位实现：
// - qks_auth_feature_compare: 特征比对占位，打印“已调用”；
// - qks_auth_token_generate: 令牌生成占位，输出固定可读令牌字符串；
// - qks_auth_execute: 兼容旧接口，占位返回成功（未使用）。

int qks_auth_feature_compare(void){
    printf("[Auth] 特征比对 已调用\n");
    return 0;
}

int qks_auth_token_generate(char* out_token, size_t cap){
    if(out_token && cap>0){
        snprintf(out_token, cap, "AUTH_TOKEN");
    }
    printf("[Auth] 身份令牌生成 已调用\n");
    return 0;
}

int qks_auth_execute(void* payload){ (void)payload; return 0; }
