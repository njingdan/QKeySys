#ifndef QKEYSYS_MODULES_KEYDIST_H
#define QKEYSYS_MODULES_KEYDIST_H

#include <stddef.h>

// 密钥分发（执行期）相关占位接口
// 备注：仅提供占位方法以理清执行逻辑；返回0表示成功。
// - 获取量子密钥占位：根据角色提示生成可读字符串，如 "QK_A_1"
// - 获取协商密钥占位：根据会话号生成可读字符串，如 "K_AGREE_1"
// - 加密占位：根据协商密钥与量子密钥生成可读密文，如 "CIPH(K+QK)"
// - 解密占位：根据协商密钥与密文恢复量子密钥字符串（演示场景下直接解析）

int qks_keydist_get_quantum_key(char* out, size_t cap, const char* role_hint);
int qks_keydist_get_agreed_key(char* out, size_t cap, int session_no);
int qks_keydist_encrypt(const char* agreed_key, const char* quantum_key, char* out_cipher, size_t cap);
int qks_keydist_decrypt(const char* agreed_key, const char* cipher, char* out_quantum_key, size_t cap);

#endif /* QKEYSYS_MODULES_KEYDIST_H */

