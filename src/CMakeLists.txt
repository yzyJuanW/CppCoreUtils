
# 指定源文件位置, 同时存入SRC_LIST这个列表中
file(GLOB_RECURSE SRC_LIST
  "./*.c" 
  "./*.cpp" 
  "./*.cc" 
  "./*.cxx"
)

# add_executable(main ${SRC_LIST})

add_library(myutils STATIC ${SRC_LIST})

set_target_properties(myutils PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib"
)

target_link_libraries(myutils fmt gtest_main gtest pthread)

# set_target_properties(main PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)