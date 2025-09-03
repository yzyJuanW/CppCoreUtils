cmake_minimum_required(VERSION 3.16)
project(CppCoreUtils)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


if (CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
  add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
  message(STATUS "the mode is debug")
elseif(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Release"))
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
  message(STATUS "the mode is release")
else()
  message(FATAL_ERROR "error!!!!!!!!!!!!!!!!!!!!")
endif()

find_package(fmt REQUIRED)
include_directories(${PROJECT_SOURCE_DIR}/include)
set(HEAD_PATH ${PROJECT_SOURCE_DIR}/include)


add_subdirectory(src)
add_subdirectory(test)

option(ENABLE_ASSERTIONS "Enable assertions" ON)