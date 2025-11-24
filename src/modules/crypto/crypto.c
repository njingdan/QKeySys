#include "modules/crypto/crypto.h"

// 加/解密模块占位实现：
// - 当前仅返回成功，不做任何加解密；
// - TODO: 替换为真实算法与密钥管理。
int qks_crypto_encrypt(const void* in, int in_len, void* out, int* out_len){ (void)in; (void)in_len; (void)out; (void)out_len; return 0; }
int qks_crypto_decrypt(const void* in, int in_len, void* out, int* out_len){ (void)in; (void)in_len; (void)out; (void)out_len; return 0; }
