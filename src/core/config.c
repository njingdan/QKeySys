// 配置模块（config）
// 作用：从环境变量与命令行参数装载运行参数（角色/模拟开关/模拟模式/自动退出时长）。
// 说明：
// - 环境变量优先预填，命令行参数用于覆盖（main 中先调用 load_env 再调用 parse_args）。
// - 角色（A/B）决定执行线程的角色位与模拟器生成事件的类型组合。
// - sim_enabled 决定是否启动模拟注入线程；sim_mode 预留（当前逻辑按混合模式工作）。
// - run_seconds>0 时主线程会在达到时长后置 stop=1 触发优雅退出。

#include "qkeysys/config.h"
#include <string.h>
#include <stdlib.h>

// 从环境变量装载配置；不存在的变量使用默认值：role=A，sim=off，sim_mode=mixed，run_seconds=0
// 环境变量：
//   QKS_ROLE=A|B
//   QKS_SIM=on|off
//   QKS_SIM_MODE=master|slave|mixed
//   QKS_RUN_SECONDS=<int>
int qks_config_load_env(qks_config_t* cfg){
    const char* r = getenv("QKS_ROLE");
    if(r && (strcmp(r,"B")==0 || strcmp(r,"b")==0)) cfg->role = QKS_ROLE_B; else cfg->role = QKS_ROLE_A;

    const char* sim = getenv("QKS_SIM");
    cfg->sim_enabled = (sim && (strcmp(sim,"on")==0 || strcmp(sim,"ON")==0)) ? 1 : 0;

    const char* mode = getenv("QKS_SIM_MODE");
    if(mode && (strcmp(mode,"master")==0 || strcmp(mode,"MASTER")==0)) cfg->sim_mode=QKS_SIM_MASTER;
    else if(mode && (strcmp(mode,"slave")==0 || strcmp(mode,"SLAVE")==0)) cfg->sim_mode=QKS_SIM_SLAVE;
    else cfg->sim_mode=QKS_SIM_MIXED;

    const char* dur = getenv("QKS_RUN_SECONDS");
    cfg->run_seconds = dur ? atoi(dur) : 0;
    return 0;
}

// 从命令行参数解析配置；支持两种形式：
//   --key value  或  --key=value
// 支持的键：
//   --role=A|B
//   --sim=on|off
//   --sim-mode=master|slave|mixed
//   --run-seconds=<int>
// 注意：解析顺序从 argv[1] 起，命令行参数会覆盖环境变量配置。
int qks_config_parse_args(qks_config_t* cfg, int argc, char** argv){
    for(int i=1;i<argc;i++){
        if(strcmp(argv[i],"--role")==0 && i+1<argc){
            const char* r=argv[++i];
            if(strcmp(r,"B")==0||strcmp(r,"b")==0) cfg->role=QKS_ROLE_B; else cfg->role=QKS_ROLE_A;
        } else if(strncmp(argv[i],"--role=",7)==0){
            const char* r=argv[i]+7; if(strcmp(r,"B")==0||strcmp(r,"b")==0) cfg->role=QKS_ROLE_B; else cfg->role=QKS_ROLE_A;

        } else if(strcmp(argv[i],"--sim")==0 && i+1<argc){
            const char* v=argv[++i]; cfg->sim_enabled = (strcmp(v,"on")==0||strcmp(v,"ON")==0)?1:0;
        } else if(strncmp(argv[i],"--sim=",6)==0){
            const char* v=argv[i]+6; cfg->sim_enabled = (strcmp(v,"on")==0||strcmp(v,"ON")==0)?1:0;

        } else if(strcmp(argv[i],"--sim-mode")==0 && i+1<argc){
            const char* v=argv[++i];
            if(strcmp(v,"master")==0) cfg->sim_mode=QKS_SIM_MASTER;
            else if(strcmp(v,"slave")==0) cfg->sim_mode=QKS_SIM_SLAVE;
            else cfg->sim_mode=QKS_SIM_MIXED;
        } else if(strncmp(argv[i],"--sim-mode=",12)==0){
            const char* v=argv[i]+12;
            if(strcmp(v,"master")==0) cfg->sim_mode=QKS_SIM_MASTER;
            else if(strcmp(v,"slave")==0) cfg->sim_mode=QKS_SIM_SLAVE;
            else cfg->sim_mode=QKS_SIM_MIXED;

        } else if(strcmp(argv[i],"--run-seconds")==0 && i+1<argc){
            cfg->run_seconds = atoi(argv[++i]);
        } else if(strncmp(argv[i],"--run-seconds=",14)==0){
            cfg->run_seconds = atoi(argv[i]+14);
        }
    }
    return 0;
}
