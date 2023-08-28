cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME "components")

project(${PROJECT_NAME}
    DESCRIPTION "Components of DCL system (services and modules)"
    HOMEPAGE_URL "https://github.com/kat_spb/dclprocessor"
    VERSION 1.0.0
    LANGUAGES C CXX)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")
set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -g")

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
    pkg_check_modules(libpqxx libpqxx)
endif()


set_component_default_state(pipelinedemo ON)
add_component(pipelinedemo)

add_executable(pl-first examples_c/first_block.c)
target_link_libraries(pl-first dclprocessor misc storage)

add_executable(pl-middle examples_c/middle_block.c)
target_link_libraries(pl-middle dclprocessor misc storage)

if (libpqxx_FOUND)
    add_executable(pl-pg examples_cpp/pgmiddle.cpp)
    target_link_libraries(pl-pg dclprocessor misc storage glog::glog pqxx)
endif()

add_executable(pl-last examples_c/last_block.c)
target_link_libraries(pl-last dclprocessor misc storage)

add_executable(pl-server examples_c/server_block.c)
target_link_libraries(pl-server dclprocessor misc)

#add_executable(pl-manager manager.cpp)
#target_link_libraries(pl-manager dclmessenger)

###############################################################################
# Components
###############################################################################
set_component_default  n_state(pgmiddle ON)
add_component(pgmiddle)

#set_component_default_state(savemiddle OFF)
#add_component(savemiddle)

#set_component_default_state(defectmiddle OFF)
#add_component(defectmiddle)

#set_component_default_state(streamoutput ON)
#add_component(streamoutput)


