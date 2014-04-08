cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
INCLUDE (CheckIncludeFiles)

project (disc_zmq)

set (DISCZMQ_MAJOR_VERSION 0)
set (DISCZMQ_MINOR_VERSION 1)
set (DISCZMQ_PATCH_VERSION 2)

set (DISCZMQ_VERSION ${DISCZMQ_MAJOR_VERSION}.${DISCZMQ_MINOR_VERSION})
set (DISCZMQ_VERSION_FULL ${DISCZMQ_MAJOR_VERSION}.${DISCZMQ_MINOR_VERSION}.${DISCZMQ_PATCH_VERSION})

message (STATUS "${PROJECT_NAME} version ${SDF_VERSION_FULL}")

set (CMAKE_CXX_FLAGS "-std=c++11")

# Compiler-specific C++11 activation.
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
    if (NOT (GCC_VERSION VERSION_GREATER 4.7))
        message(FATAL_ERROR "${PROJECT_NAME} requires g++ 4.8 or greater.")
    endif ()
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
else ()
    message(FATAL_ERROR "Your C++ compiler does not support C++11.")
endif ()

add_subdirectory(tools)
add_subdirectory(test)

execute_process(COMMAND pkg-config --modversion protobuf
  OUTPUT_VARIABLE PROTOBUF_VERSION
  RESULT_VARIABLE protobuf_modversion_failed)

CHECK_INCLUDE_FILES(ifaddrs.h HAVE_IFADDRS_H)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.hh.in ${CMAKE_CURRENT_SOURCE_DIR}/config.hh)

########################################
if (PROTOBUF_VERSION LESS 2.3.0)
  BUILD_ERROR("Incorrect version: Transport requires protobuf version 2.3.0 or greater")
endif()

########################################
# The Google Protobuf library for message generation + serialization
find_package(Protobuf REQUIRED)
if (NOT PROTOBUF_FOUND)
  BUILD_ERROR ("Missing: Google Protobuf (libprotobuf-dev)")
endif()
if (NOT PROTOBUF_PROTOC_EXECUTABLE)
  BUILD_ERROR ("Missing: Google Protobuf Compiler (protobuf-compiler)")
endif()
if (NOT PROTOBUF_PROTOC_LIBRARY)
  BUILD_ERROR ("Missing: Google Protobuf Compiler Library (libprotoc-dev)")
endif()

# Create the transport shared library
add_library(disczmq SHARED discZmq.cc sockets/socket.cc netUtils.cc packet.cc topicsInfo.cc)
target_link_libraries(disczmq
  protobuf
  zmq
  uuid
)

# Unit tests
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_executable(UNIT_packet_TEST packet_TEST.cc)
add_executable(UNIT_topicsInfo_TEST topicsInfo_TEST.cc)
add_executable(UNIT_discZmq_TEST discZmq_TEST.cc)

target_link_libraries(UNIT_packet_TEST disczmq gtest gtest_main)
target_link_libraries(UNIT_topicsInfo_TEST disczmq gtest gtest_main)
target_link_libraries(UNIT_discZmq_TEST disczmq gtest gtest_main)

# Install the library
set_target_properties(disczmq PROPERTIES SOVERSION ${DISCZMQ_MAJOR_VERSION} VERSION ${DISCZMQ_VERSION_FULL})
install (TARGETS disczmq DESTINATION ${CMAKE_INSTALL_PREFIX}/lib COMPONENT shlib)