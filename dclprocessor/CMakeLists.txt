cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME "dclengine")

project(${PROJECT_NAME}
    DESCRIPTION "Engine for TMK_DCL system and client examples for this one"
    HOMEPAGE_URL "https://github.com/tmk-org/dclprocessor"
    VERSION 0.0.1
    LANGUAGES C CXX)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
#file(MAKE_DIRECTORY ${${PROJECT_NAME}_BINARY_DIR}/include)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")
set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -g")

###############################################################################
# Include CMake dependencies
###############################################################################

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

###############################################################################
# Options
###############################################################################

###############################################################################
# Find packages
###############################################################################

###############################################################################
# Add sources
###############################################################################

#set(PUBLIC_HEADERS
#    collector/include/collector/collector.h
#    collector/include/collector/collector_description.h
#    collector/include/collector/source.h
#    collector/include/collector/source_description.h

#    pipeline/include/pipeline/pipeline.h
#    pipeline/include/pipeline/pipeline_server.h
#    pipeline/include/pipeline/pipeline_client.h
#    pipeline/include/pipeline/pipeline_description.h
#    pipeline/include/pipeline/pipeline_module.h
#    pipeline/include/pipeline/pipeline_module_description.h

#    config/include/config/config.h

#    tcp/include/tcp/tcp_process.h

#    messenger/include/messenger/messenger.h
#    messenger/include/messenger/api.h
#)

#set_target_properties(${PROJECT_NAME} PROPERTIES
#    PUBLIC_HEADER "${CMAKE_BINARY_DIR}/${PUBLIC_HEADERS}"
#)

#target_include_directories (${PROJECT_NAME} PUBLIC
#    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#    $<INSTALL_INTERFACE:include>
#)

#add_subdirectory(misc)
#add_subdirectory(libcapture)

#set_component_default_state(tcp ON)
#add_component(tcp)
add_subdirectory(tcp)

#set_component_default_state(shmem ON)
#add_component(shmem)
add_subdirectory(shmem)

#set_component_default_state(process ON)
#add_component(process)
add_subdirectory(process)

#set_component_default_state(description ON)
#add_component(description)
add_subdirectory(description)

#set_component_default_state(pipeline ON)
#add_component(pipeline)
add_subdirectory(pipeline)

#set_component_default_state(collector ON)
#add_component(collector)
add_subdirectory(collector)

#set_component_default_state(processor ON)
#add_component(processor)
add_subdirectory(processor)

#set_component_default_state(DCLProcessor ON)
#add_component(DCLProcessor)
# !!! DCLProcessor make broken build now
#add_subdirectory(DCLProcessor)

###############################################################################
# Install
###############################################################################

#TODO: think about install config





