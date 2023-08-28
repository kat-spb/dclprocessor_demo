cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME "tcpcapture")

project(${PROJECT_NAME}
    DESCRIPTION "TCP source capturer for dcl-DCL"
    HOMEPAGE_URL "https://github.com/dcl-org/libcapture/FakeTCP"
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

# TCP_camera
find_package(OpenCV REQUIRED)
#libdclfilters
#libstorage
#misc -- TODO

###############################################################################
# Add sources
###############################################################################

add_library(${PROJECT_NAME} SHARED
    #headers
    include/tcpcapture/cvcamera.h
    include/tcpcapture/cvcollector.h
    include/tcpcapture/cvgenerator.h
    #sources
    src/cvcollector.cpp
    src/cvgenerator.cpp
)

target_link_libraries(${PROJECT_NAME} opencv_core opencv_highgui opencv_imgproc -pthread misc dclfilters storage)
target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)

set(PUBLIC_HEADERS
    include/tcpcapture/cvgenerator.h
    include/tcpcapture/cvcollector.h
    include/tcpcapture/cvcamera.h
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PUBLIC_HEADER "${CMAKE_BINARY_DIR}/${PUBLIC_HEADERS}"
    VERSION ${PROJECT_VERSION}
    LANGUAGE CXX
)

set(TCP_FAKE_CAMERA dcl-camera)
add_executable(${TCP_FAKE_CAMERA} tools/tcpcamera.cpp)
target_link_libraries(${TCP_FAKE_CAMERA} tcpcapture)
set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    LANGUAGE CXX
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
    install(TARGETS ${TCP_FAKE_CAMERA} RUNTIME DESTINATION bin)
    install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
    install(FILES ${INSTALL_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
    install(EXPORT dcldcl DESTINATION targets)
endif ()
