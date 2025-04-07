
# 创建可执行文件 `main`（只包含 `main.cc`）
add_executable(test test.cpp)

# 5️⃣ 链接 `main` 所需的库，包括 `src_lib`、`memory_lib`、`log_lib` 和 `LIBS`
target_link_libraries(test  util_lib log_lib threadpool_lib  ${LIBS})
