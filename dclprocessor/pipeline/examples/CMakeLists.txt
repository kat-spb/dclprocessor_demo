cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME "pipeline_examples")

project(${PROJECT_NAME}
    DESCRIPTION "Components of TMK_DCL system (services and modules)"
    HOMEPAGE_URL "https://github.com/tmk-org/TMK-DCL2/dclprocessor/pipeline/examples"
    VERSION 1.0.0
    LANGUAGES C)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")
set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -g")

#TODO: find dclplconfig if want to use it

add_executable(psource psource.c)
target_link_libraries(psource dclpipeline)

add_executable(phandler phandler.c)
target_link_libraries(phandler dclpipeline)

add_executable(pserver pipeline_examples.c pserver.c)
#target_link_libraries(pserver dclpipeline dclplconfig)
target_link_libraries(pserver dclpipeline)

#if(${JSONC_FOUND})
#    add_executable(clear_pljson pipeline_big_json.c)
#    target_link_libraries(clear_pljson -ljson-c)
#    target_include_directories(clear_pljson PUBLIC ${JSONC_INCLUDE_DIRS})

#    add_executable(test_pljson rw_pl_descriptor.c)
#    target_link_libraries(test_pljson dclpipeline -ljson-c)
#    target_include_directories(test_pljson PUBLIC ${JSONC_INCLUDE_DIRS})
#endif()




