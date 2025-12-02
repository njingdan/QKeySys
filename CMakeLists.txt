cmake_minimum_required(VERSION 3.15)
project(QKeySys C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# -------------------------- 自动收集所有 .c 源文件 --------------------------
# GLOB_RECURSE：递归查找指定目录下的所有匹配文件
# CONFIGURE_DEPENDS：当文件添加/删除时，CMake 会自动重新配置（需 CMake 3.12+）
file(GLOB_RECURSE QKEYSYS_SOURCES
    CONFIGURE_DEPENDS  # 关键：支持文件变动自动更新
    "src/*.c"          # 递归收集 src/ 下所有 .c 文件
)

# 可选：如果有需要排除的文件（比如某些不需要编译的示例文件），可以用 list(FILTER)
# 示例：排除 src/sim/ 目录下的文件（如果不需要的话）
# list(FILTER QKEYSYS_SOURCES EXCLUDE REGEX "src/sim/.*\\.c$")

# -------------------------- 核心库目标 --------------------------
add_library(qkeysys ${QKEYSYS_SOURCES})


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

