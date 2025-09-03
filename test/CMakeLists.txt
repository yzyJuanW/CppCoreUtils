
# 指定源文件位置, 同时存入SRC_LIST这个列表中
aux_source_directory(./ TEST_LIST)

add_executable(test ${TEST_LIST})

target_link_libraries(test fmt gtest_main gtest pthread myutils)

set_target_properties(test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)