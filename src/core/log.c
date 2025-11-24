#include "qkeysys/log.h"
#include <stdarg.h>

// 当前阶段日志为空实现，仅保留接口，便于后续接入统一日志模块。
void qks_log_debug(const char* fmt, ...) { (void)fmt; }
void qks_log_info (const char* fmt, ...) { (void)fmt; }
void qks_log_warn (const char* fmt, ...) { (void)fmt; }
void qks_log_error(const char* fmt, ...) { (void)fmt; }

