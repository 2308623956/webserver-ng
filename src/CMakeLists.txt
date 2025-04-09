# 获取当前目录下所有 .cc 文件
file(GLOB SRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/*cpp)

# 从 `SRC_FILE` 列表中移除 `main.cpp&test.cpp`
list(REMOVE_ITEM SRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

# # 创建可执行文件 `main`（只包含 `main.cc`）
# add_executable(main main.cpp)

# 5️⃣ 链接 `main` 所需的库，包括 `src_lib`、`memory_lib`、`log_lib` 和 `LIBS`
add_library(src_lib SHARED ${SRC_FILE})

