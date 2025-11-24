#ifndef QKEYSYS_LOG_H
#define QKEYSYS_LOG_H

// 日志占位接口：当前阶段不输出，仅保留函数，便于后续接入。
void qks_log_debug(const char* fmt, ...);
void qks_log_info(const char* fmt, ...);
void qks_log_warn(const char* fmt, ...);
void qks_log_error(const char* fmt, ...);

#endif /* QKEYSYS_LOG_H */

