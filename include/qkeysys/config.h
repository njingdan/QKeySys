#ifndef QKEYSYS_CONFIG_H
#define QKEYSYS_CONFIG_H

#include "types.h"

typedef enum {
    QKS_SIM_OFF   = 0,
    QKS_SIM_MASTER= 1,
    QKS_SIM_SLAVE = 2,
    QKS_SIM_MIXED = 3
} qks_sim_mode_t;

typedef struct {
    qks_role_t    role;       // 运行角色：A/B
    int           sim_enabled; // 仿真注入器开关
    qks_sim_mode_t sim_mode;   // 仿真模式
    int           run_seconds; // 自动退出时长（秒），<=0 表示常驻
} qks_config_t;

// 从环境变量与命令行参数装载配置（仅角色选择）。
int qks_config_load_env(qks_config_t* cfg);
int qks_config_parse_args(qks_config_t* cfg, int argc, char** argv);

#endif /* QKEYSYS_CONFIG_H */
