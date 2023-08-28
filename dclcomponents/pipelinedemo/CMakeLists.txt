cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME "pipelinedemo")

project(${PROJECT_NAME}
    DESCRIPTION "Pipeline demonstration"
    HOMEPAGE_URL "https://github.com/tmk-org/TMK-DCL2/pipelinedemo"
    VERSION 1.0.0
    LANGUAGES C)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")
set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -g")

add_executable(tmk-demo-capturer src/capturer.c)
target_link_libraries(tmk-demo-capturer dclpipeline)

add_executable(tmk-demo-filter src/filter.c)
target_link_libraries(tmk-demo-filter dclpipeline)
