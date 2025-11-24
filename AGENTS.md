# Repository Guidelines

## Project Structure & Module Organization
- Root: `CMakeLists.txt`, `README.md`, `项目需求.md` (PRD).
- Core: `src/core/` (scheduler, queue, init_master/slave, exec, os, config), headers in `include/qkeysys/`.
- Modules (placeholders): `src/modules/{auth,keypool,clocksync,crypto,keydist}/` with public headers under `include/modules/...`.
- Comm (placeholders): `src/comm/{zigbee,usrp}/` with headers in `include/comm/...`.
- Simulation: `src/sim/` (event injector); can be disabled via flags.
- App entry: `src/app/main.c`.

### Event Injection (Core API)
- Prefer using the unified core API to inject events instead of touching the queue directly:
  - Header: `include/qkeysys/inject.h`
  - Function: `int qks_inject_event(qks_sched_ctx_t* s, qks_event_type_t type, qks_event_dir_t dir, void* payload);`
- This constructs a minimal event with default priority, pushes it into the FIFO, and signals the scheduler.
- The simulator and future listeners should call this API.

## Build, Test, and Development Commands
- Linux (GCC)
  - `mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && cmake --build . -j`
  - Run: `./qkeysys_app --role=A --sim=on --run-seconds=10`
- Windows (MinGW)
  - `mkdir build && cd build && cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release && cmake --build . -j`
  - Run: `./qkeysys_app.exe --role=A --sim=on --run-seconds=10`
- Env overrides: `QKS_ROLE`, `QKS_SIM`, `QKS_SIM_MODE`, `QKS_RUN_SECONDS`.
- Quick one-shot build (no CMake, Windows shown):
  - `gcc -std=c11 -O2 -Wall -Wextra -DQKS_OS_WINDOWS -Iinclude <sources...> -o build/qkeysys_app.exe`
  - Remember to include `src/core/inject.c` in the sources if building without CMake.

## Coding Style & Naming Conventions
- Language: C11 only; avoid C++.
- Indentation: 4 spaces; max line ~100 chars.
- Naming: `qks_` prefix for public C APIs; snake_case for functions/vars; UPPER_SNAKE for macros/guards.
- File names: lowercase with underscores; headers mirror directories.
- Do not manipulate queue/scheduler internals directly—use the exported APIs.

## Testing Guidelines
- No formal unit test suite yet. Use the simulation runner as smoke test:
  - `--sim=on --run-seconds=10` and verify 中文步骤打印（调度“选择候选/抢占/SLAVE 丢弃”，初始化“IWAIT/EXEC”，执行“认证/时钟/密钥分发 完成”）。
- When adding tests, place in `tests/`, name `test_<module>_*.c`, and prefer CTest/Unity.

## Commit & Pull Request Guidelines
- Keep commits focused; suggested prefixes: `feat:`, `fix:`, `refactor:`, `docs:`, `build:`.
- PRs must include: summary, motivation, key changes, how you validated (commands/output), and links to `项目需求.md` sections if applicable.
- Before opening a PR: build on Linux and MinGW, run the simulated smoke test, and ensure no compiler warnings introduced.

## Architecture & Safety Notes
- Scheduler is event-driven with soft/hard blocking; execution is single-event (no concurrent Exec).
- Placeholders (payloads, comm, crypto) are stubbed—document any temporary behavior in comments.
- Avoid long blocking I/O on the scheduler path; use simulation or background threads.

### Queueing & Preemption (Updated)
- Dual-queue design:
  - FIFO ingress buffer: all new events go here first and wake the scheduler.
  - Waiting heap (min-heap by priority asc, seq asc): only MASTER events may wait here; preempted MASTER returns here preserving seq.
- SLAVE events never queue: SLAVE participates only in immediate decision; if it cannot execute immediately, it is dropped (no heap).
- Selection: when exec slot is free, the scheduler chooses between FIFO head and heap top; if SLAVE is chosen it is accepted (IWAIT+hard block) and switched to EXEC via ACT.
- Preemption: only during MASTER Init window; new arrival with higher priority can preempt. If SLAVE, accept immediately; if MASTER, install as usual. Previous MASTER goes back to heap.

### Execution Threads
- One dispatcher thread plus six dedicated worker threads (AUTH/CLOCK/KEY_REFILL × A/B). Dispatcher wakes the proper worker and waits for completion, then notifies the scheduler.

### Key Distribution Module (Placeholder)
- `modules/keydist/` exposes four stub methods: get quantum key, get agreed key, encrypt, decrypt. Each prints a success line; no real crypto.
- After mutual confirmation, both A and B call `qks_keypool_update` (stub) to update key pool; prints success.

### Simulator (A/B Roles)
- Simulator is a pure event generator thread; toggled via `--sim=on/off`. Stopping it does not affect other threads.
- Role A: generates MASTER AUTH/CLOCK (high prio) and SLAVE KEY_REFILL (low prio). SLAVE only immediate decision; dropped if not executable.
- Role B: generates MASTER KEY_REFILL (low prio) and SLAVE AUTH/CLOCK (high prio). SLAVE can preempt MASTER during MASTER Init, exercising SLAVE accept_and_claim(IWAIT+hard block) → on_act → EXEC.
