#ifndef QKEYSYS_MODULES_CRYPTO_H
#define QKEYSYS_MODULES_CRYPTO_H

// 加/解密模块占位：
// TODO: 定义加解密与完整性校验接口，后续替换为真实算法实现。

int qks_crypto_encrypt(const void* in, int in_len, void* out, int* out_len);
int qks_crypto_decrypt(const void* in, int in_len, void* out, int* out_len);

#endif /* QKEYSYS_MODULES_CRYPTO_H */

