cmake_minimum_required(VERSION 3.15)
project(QKeySys C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(qkeysys
    src/core/config.c
    src/core/log.c
    src/core/os.c
    src/core/queue.c
    src/core/scheduler.c
    src/core/exec.c
    src/core/init_master.c
    src/core/init_slave.c
    src/core/inject.c
    src/comm/zigbee/zigbee.c
    src/comm/usrp/usrp.c
    src/modules/auth/auth.c
    src/modules/keypool/keypool.c
    src/modules/clocksync/clocksync.c
    src/modules/crypto/crypto.c
    src/modules/keydist/keydist.c
    src/threads/scheduler_thread.c
    src/threads/init_master_mgr.c
    src/threads/zigbee_listener.c
    src/threads/clock_thread.c
    src/threads/storage_listener.c
    src/threads/threads_all.c
    src/sim/injector.c

    src/modules/keypool/GatewayComm.c
    src/modules/keypool/KeyConfig.c
    src/modules/keypool/KeyInfo.c
    src/modules/keypool/KeyManager.c
    src/modules/keypool/NTPClient.c
)

target_include_directories(qkeysys PUBLIC include include/modules/keypool)

if(WIN32)
    target_compile_definitions(qkeysys PRIVATE QKS_OS_WINDOWS)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(qkeysys PRIVATE Threads::Threads)
    target_compile_definitions(qkeysys PRIVATE QKS_OS_POSIX)
endif()

add_executable(qkeysys_app src/app/main.c)
target_link_libraries(qkeysys_app PRIVATE qkeysys)

# -------------------------- 新增：测试目标（qkeysys_test）--------------------------
# 测试代码：就是你之前写的“密钥管理器测试程序”（假设路径是 src/test/key_manager_test.c）
# 注意：把路径换成你实际的测试代码路径（比如你之前的 main.c 放在哪里，就写哪里）
add_executable(qkeysys_test 
    test/key_pool_test.c  # 你的测试代码（替换成实际路径）
)

# 测试目标链接核心库（获取所有模块功能，包括你的密钥管理器）
target_link_libraries(qkeysys_test PRIVATE qkeysys)

# 给测试目标添加头文件路径（确保能找到所有头文件）
target_include_directories(qkeysys_test PUBLIC 
    include 
    include/modules/keypool  # 确保能找到师姐的 keypool.h
)
