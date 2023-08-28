cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME "dclshmem")
set(DIR_NAME "shmem")

project(${PROJECT_NAME}
    DESCRIPTION "Engine for TMK_DCL system and client examples for this one"
    HOMEPAGE_URL "https://github.com/tmk-org/dclprocessor/${DIR_NAME}"
    VERSION 0.0.1
    LANGUAGES C)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
#file(MAKE_DIRECTORY ${${PROJECT_NAME}_BINARY_DIR}/include)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")
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

add_library(${PROJECT_NAME} SHARED
    #header
    include/${DIR_NAME}/shared_memory.h
    include/${DIR_NAME}/memory_map.h
    #sources
    src/shared_memory.c
)

target_link_libraries(${PROJECT_NAME} -lrt)
target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)

set(PUBLIC_HEADERS_SHMEM
    include/${DIR_NAME}/memory_map.h
    include/${DIR_NAME}/shared_memory.h
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PUBLIC_HEADER "${CMAKE_BINARY_DIR}/${PUBLIC_HEADERS_SHMEM}"
    VERSION ${PROJECT_VERSION}
    LANGUAGE C
)

set(TOOLS_NAME tools)
add_library(${PROJECT_NAME}_env SHARED
    #header
    ${TOOLS_NAME}/env_manipulations.h
    #sources
    ${TOOLS_NAME}/env_manipulations.cpp
)

target_link_libraries(${PROJECT_NAME}_env -lrt dclshmem)
set(PRIVATE_HEADER_ENV
    ${TOOLS_NAME}/env_manipulations.h
)
set(PUBLIC_HEADER_ENV
    include/${DIR_NAME}/env_manipulations.h
)
target_include_directories(${PROJECT_NAME}_env PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/${PRIVATE_HEADER_ENV}> $<INSTALL_INTERFACE:include>)
set_target_properties(${PROJECT_NAME}_env PROPERTIES
    PUBLIC_HEADER "${CMAKE_BINARY_DIR}/${PUBLIC_HEADER_ENV}"
    VERSION ${PROJECT_VERSION}
    LANGUAGE CXX
)

#add_subdirectory(misc)
#add_subdirectory(examples)

###############################################################################
# Install
###############################################################################
configure_file(${PRIVATE_HEADER_ENV} ${CMAKE_BINARY_DIR}/${PUBLIC_HEADER_ENV} COPYONLY)
list(APPEND INSTALL_HEADERS ${CMAKE_BINARY_DIR}/${PUBLIC_HEADER_ENV})

foreach(HEADER ${PUBLIC_HEADERS_SHMEM})
    configure_file(${HEADER} ${CMAKE_BINARY_DIR}/${HEADER} COPYONLY)
    list(APPEND INSTALL_HEADERS ${CMAKE_BINARY_DIR}/${HEADER})
endforeach(HEADER)

if (UNIX)
    configure_file(${PROJECT_NAME}.pc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY)
    include(GNUInstallDirs)
    install(TARGETS ${PROJECT_NAME}
            EXPORT tmkdcl
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
    install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
    install(FILES ${INSTALL_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
    install(EXPORT tmkdcl DESTINATION targets)
endif ()

###############################################################################
# Tests
###############################################################################
if (NOT WITHOUT_TESTING)
    enable_testing()
    find_package(GTest REQUIRED)
    set(PROJECT_TESTS ${PROJECT_NAME}-tests)

    add_executable(${PROJECT_TESTS}
            tests/DCLShmemTests.cpp
            tests/memory_map.cpp
            #tests/shared_memory.cpp  //TODO: problems with common part env_manipulations.h
            )

    target_link_libraries(${PROJECT_TESTS}
            ${GTEST_BOTH_LIBRARIES} -pthread rt
            dclshmem_env
            ${PROJECT_NAME}
            )
    set_target_properties(${PROJECT_TESTS} PROPERTIES
                            LANGUAGE CXX
                            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )
    add_test(NAME ${PROJECT_TESTS} COMMAND $<TARGET_FILE:${PROJECT_TESTS}>)
endif ()
