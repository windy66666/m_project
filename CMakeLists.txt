cmake_minimum_required(VERSION 3.0)
project(M_PROJECT)

# # 设置C++标准
# set(CMAKE_CXX_STANDARD 11)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 GLib
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)

# 直接链接 SQLite3（最简单的方法）
find_library(SQLITE3_LIB sqlite3)
if(NOT SQLITE3_LIB)
    message(FATAL_ERROR "SQLite3 library not found. Install with: sudo apt-get install libsqlite3-dev")
endif()

# 包含目录
include_directories(include ${GLIB_INCLUDE_DIRS})

# 添加可执行文件
add_executable(main 
    main.cpp 
    src/business.cpp 
    src/protocol.cpp 
    src/tcp_server.cpp
    src/data_handler.cpp
)

# 链接库
target_link_libraries(main ${GLIB_LIBRARIES} ${SQLITE3_LIB})

message(STATUS "Linked SQLite3: ${SQLITE3_LIB}")