# QKeySys（事件驱动安全平台）

本项目为“基于量子密钥与RSSI协商密钥”的安全防护平台的代码骨架，采用纯C语言实现，支持 Linux（GCC）与 Windows（MinGW）编译。

## 目标与范围
- 角色划分：A（网关，唯一）与 B（储能柜，多个）；运行时选择角色。
- 范围：主事件调度线程 + 主/从初始化线程的流程框架；通信与执行逻辑仅占位。
- 并发规则：
  - 同优先级严格 FIFO；仅更高优先级可在“主初始化 Init 阶段”抢占。
  - 从初始化一旦接受并生成占位，立即进入“硬阻塞”（不可打断）。
  - 执行阶段不可被打断。
- 优先级默认：设备认证=1、时钟同步=1、密钥分发=4。
- Payload 字段：预留（TODO：根据后续需求完善）。

## 架构概览
- 核心层：事件模型、双队列（FIFO 接入缓冲 + 等待堆，仅 MASTER）、执行区、软/硬阻塞守卫；条件变量驱动：
  - 新事件入 FIFO 时 signal 唤醒；执行完成后 broadcast 唤醒；
  - 执行区空闲：等待“FIFO 或 等待堆”有事件；执行区占用：仅等待 FIFO（新到达可能触发抢占）。
- 线程层：跨平台线程/锁/条件变量抽象；主/从初始化线程；执行分发线程 + 六个专用工作线程。
- 通信层：Zigbee/USRP 占位接口（打印+sleep）。
- 业务层：认证、密钥分发、密钥池、时钟同步、加/解密占位接口。
- 配置：集中超时参数（`include/config/timeouts.h`），运行参数（`include/qkeysys/config.h`）。
- 日志：中文步骤打印，便于观察流程。

## 目录结构
- include/
  - qkeysys/: types.h, config.h, log.h, os.h, event.h, queue.h, scheduler.h, init_master.h, init_slave.h
  - config/: timeouts.h
  - modules/: auth/, keypool/, clocksync/, crypto/
  - comm/: zigbee/, usrp/
- src/
  - core/: config.c, log.c, os.c, queue.c, scheduler.c, init_master.c, init_slave.c, inject.c
  - modules/: auth/, keypool/, clocksync/, crypto/
  - comm/: zigbee/, usrp/
  - app/: main.c

## 构建
### Windows（MinGW）
在 MinGW 环境中：
```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
./qkeysys_app.exe --role=A --run-seconds=10   # 或 --role=B
```

### 快速构建（无 CMake）
- Windows（MinGW）：
```bash
gcc -std=c11 -O2 -Wall -Wextra -DQKS_OS_WINDOWS -Iinclude `
  src/app/main.c `
  src/core/config.c src/core/log.c src/core/os.c src/core/queue.c src/core/scheduler.c src/core/exec.c src/core/init_master.c src/core/init_slave.c src/core/inject.c `
  src/comm/zigbee/zigbee.c src/comm/usrp/usrp.c `
  src/modules/auth/auth.c src/modules/keypool/keypool.c src/modules/clocksync/clocksync.c src/modules/crypto/crypto.c `
  src/modules/keydist/keydist.c `
  src/threads/scheduler_thread.c src/threads/init_master_mgr.c src/threads/zigbee_listener.c src/threads/clock_thread.c src/threads/storage_listener.c src/threads/threads_all.c `
  src/sim/injector.c `
  -o build/qkeysys_app.exe

```

## 运行说明
- 自动退出：`--run-seconds=N` 指定N秒后自动优雅退出；不指定或<=0则常驻运行。
- 模拟注入器：当 `--sim=on` 时（A/B 通用），内置模拟器周期性入队事件，触发调度/抢占/阻塞流程并输出中文步骤打印。

### 运行示例
- A 侧（开启模拟，观察 10 秒）：
```bash
./qkeysys_app.exe --role=A --sim=on --run-seconds=10
```
- B 侧（开启模拟，观察 10 秒）：
```bash
./qkeysys_app.exe --role=B --sim=on --run-seconds=10
```

关键中文打印示例：
- 调度：`[调度] 选择候选 …`、`[调度] 抢占 …`、`[调度] 不可抢占: MASTER 转等待堆/SLAVE 丢弃 …`
- 从初始化：`[从初始化] 接受并占位(IWAIT+硬阻塞)`、`[从初始化] 收到 ACT; 切换到 EXEC`
- 执行：`[执行-分发] 收到事件 …`、`[执行-认证/时钟同步/密钥分发-A/B] 完成`
- 模块：`[KeyDist] 请求/协商/加密/解密 已调用`、`[KeyPool] qks_keypool_update 调用成功`

模拟器说明：
- 模拟器是“仅生成事件”的独立线程（`--sim=on/off`）；关闭不影响其他线程。
- A 侧：生成 MASTER(AUTH/CLOCKSYNC 高优) 与 SLAVE(KEY_REFILL 低优)。SLAVE 只参与“立即决策”，不可执行时丢弃，不入等待堆。
- B 侧：生成 MASTER(KEY_REFILL 低优) 与 SLAVE(AUTH/CLOCKSYNC 高优)。在 MASTER Init 窗口，SLAVE 可抢占 MASTER，验证从初始化 IWAIT→EXEC 流程。

## 设计要点
- 事件结构最小化，仅用于“明确类型/方向、进入执行区、唤醒对应线程”，不存储超时计数与对端信息。
- 抢占仅在“主初始化 Init 阶段”，使用“取消标志+代次 token”安全终止初始化线程（占位设计）。
- 从初始化接纳后立即硬阻塞（包含 InitWaitAct 与 Exec），直到事件完成释放。
- 事件完成后不重新入队；从端无法立即执行统一回复“忙碌”。

### 调度与队列（
- 双队列：
  - FIFO 接入缓冲：所有新事件统一入 FIFO，唤醒调度器。
  - 等待堆（小顶堆）：仅主事件（MASTER）可进入等待。被抢占的 MASTER 也回到等待堆（保留 seq）。
- 从事件（SLAVE）不排队：
  - 只参与“立即决策”；能立即执行则接受（IWAIT→EXEC），否则直接丢弃（不入等待堆）。
- 完成后：仅清空执行区并唤醒调度器；不做 flush。

### 执行架构
- 调度线程：条件变量驱动，执行区空时在 FIFO 与 等待堆 顶之间择优；MASTER Init 窗口支持更高优先级新到达的抢占。
- 初始化线程：
  - 主初始化线程（占位）：INIT→ACK→ACT→切 EXEC。
  - 从初始化线程（占位）：被调度器接受后 accept_and_claim(IWAIT+硬阻塞)，随后 on_act 切 EXEC。
- 执行线程：分发线程 + 六个专用工作线程（AUTH/CLOCKSYNC/KEY_REFILL × A/B）。分发线程唤醒对应工作线程，完成后统一通知调度器。
- 监听线程（占位）：Zigbee/时钟/储能柜（B）。
  

