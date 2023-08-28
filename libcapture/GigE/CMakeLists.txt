cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME "udpcapture")

project(${PROJECT_NAME}
    DESCRIPTION "TCP and UDP source captuer for dcl-DCL"
    HOMEPAGE_URL "https://github.com/dcl-org/libcapture/GigE"
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

#misc

###############################################################################
# Add sources
###############################################################################

add_library(${PROJECT_NAME} SHARED
    #header
    include/${PROJECT_NAME}/iframe.h
    include/${PROJECT_NAME}/gvcp.h
    include/${PROJECT_NAME}/gvsp.h  #warning: no file gvsp.c, it's ok 
    include/${PROJECT_NAME}/device.h
    include/${PROJECT_NAME}/interface.h
    include/${PROJECT_NAME}/gige_camera_ops.h
    include/${PROJECT_NAME}/gige_camera.h
    #sources
    src/gvcp.c
    src/device.c
    src/interface.c
    src/gige_camera_ops.c
    src/gige_camera.c
)

target_link_libraries(${PROJECT_NAME} misc -pthread)
target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)

set(PUBLIC_HEADERS
    include/${PROJECT_NAME}/gvcp.h
    include/${PROJECT_NAME}/gvsp.h  #warning: no file gvsp.c, it's ok 
    include/${PROJECT_NAME}/device.h
    include/${PROJECT_NAME}/interface.h
    include/${PROJECT_NAME}/gige_camera.h
    #TODO: think about 'stream' by GenAPI
    include/${PROJECT_NAME}/iframe.h
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PUBLIC_HEADER "${CMAKE_BINARY_DIR}/${PUBLIC_HEADERS}"
    VERSION ${PROJECT_VERSION}
    LANGUAGE C
)

set(UDP_FINDER dcl-finder)
add_executable(${UDP_FINDER} tools/udpfinder.c)
target_link_libraries(${UDP_FINDER} ${PROJECT_NAME})
set_target_properties(${UDP_FINDER} PROPERTIES
    VERSION ${PROJECT_VERSION}
    LANGUAGE C
)

set(UDP_TEST dcl-test)
add_executable(${UDP_TEST} tools/udptest.c)
target_link_libraries(${UDP_TEST} ${PROJECT_NAME})
set_target_properties(${UDP_TEST} PROPERTIES
    VERSION ${PROJECT_VERSION}
    LANGUAGE C
)

###############################################################################
# Install
###############################################################################

foreach(HEADER ${PUBLIC_HEADERS})
    configure_file(${HEADER} ${CMAKE_BINARY_DIR}/${HEADER} COPYONLY)
    list(APPEND INSTALL_HEADERS ${CMAKE_BINARY_DIR}/${HEADER})
endforeach(HEADER)

if (UNIX)
    configure_file(${PROJECT_NAME}.pc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY)
    include(GNUInstallDirs)
    install(TARGETS ${PROJECT_NAME}
            EXPORT dcldcl
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
    install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
    install(FILES ${INSTALL_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
    install(TARGETS ${UDP_FINDER} ${UDP_TEST}
            EXPORT dcldcl
            RUNTIME DESTINATION bin
           )
    install(EXPORT dcldcl DESTINATION targets)
endif ()
