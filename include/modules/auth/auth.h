#ifndef QKEYSYS_MODULES_AUTH_H
#define QKEYSYS_MODULES_AUTH_H
#include <stddef.h>

// 设备认证模块占位：
// TODO: 定义认证执行期所需的输入/输出与回调接口。

int qks_auth_execute(void* payload);
// 设备认证执行相关占位方法（仅打印“已调用”）
int qks_auth_feature_compare(void);
int qks_auth_token_generate(char* out_token, size_t cap);

#endif /* QKEYSYS_MODULES_AUTH_H */
