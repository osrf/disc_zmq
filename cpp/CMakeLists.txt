cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
INCLUDE (CheckIncludeFiles)
include(FindBoost)

project (disc_zmq)

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
)

# Unit tests
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_executable(UNIT_packet_TEST packet_TEST.cc)
add_executable(UNIT_topicsInfo_TEST topicsInfo_TEST.cc)
add_executable(UNIT_discZmq_TEST discZmq_TEST.cc)

target_link_libraries(UNIT_packet_TEST disczmq ${Boost_LIBRARIES} gtest gtest_main)
target_link_libraries(UNIT_topicsInfo_TEST disczmq gtest gtest_main)
target_link_libraries(UNIT_discZmq_TEST disczmq gtest gtest_main)