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
)

target_include_directories(qkeysys PUBLIC include)

if(WIN32)
    target_compile_definitions(qkeysys PRIVATE QKS_OS_WINDOWS)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(qkeysys PRIVATE Threads::Threads)
    target_compile_definitions(qkeysys PRIVATE QKS_OS_POSIX)
endif()

add_executable(qkeysys_app src/app/main.c)
target_link_libraries(qkeysys_app PRIVATE qkeysys)
