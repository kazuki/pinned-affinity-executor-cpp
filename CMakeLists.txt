cmake_minimum_required(VERSION 3.12)
project(pinned_affinity_executor CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")

find_package(PkgConfig)

pkg_check_modules(LIBNUMA numa)
include_directories(${LIBNUMA_INCLUDE_DIRS})

add_library(pinned_affinity_executor STATIC src/executor.cpp)
target_include_directories(pinned_affinity_executor INTERFACE src)
target_link_libraries(pinned_affinity_executor ${LIBNUMA_LIBRARIES})

add_executable(pinned_affinity_executor_test src/main.cpp)
target_link_libraries(pinned_affinity_executor_test pinned_affinity_executor ${LIBNUMA_LIBRARIES})
