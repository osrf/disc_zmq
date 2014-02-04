cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
INCLUDE (CheckIncludeFiles)
include(FindBoost)

project (disc_zmq)

add_subdirectory(test)

CHECK_INCLUDE_FILES(ifaddrs.h HAVE_IFADDRS_H)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.hh.in ${CMAKE_CURRENT_SOURCE_DIR}/config.hh)

add_library(SOCKETS sockets/socket.cc)

add_executable(publisher publisher.cc)
add_executable(subscriber subscriber.cc)
add_executable(requester requester.cc)
add_executable(replier replier.cc)

target_link_libraries(${Boost_LIBRARIES} publisher zmq boost_program_options SOCKETS)
target_link_libraries(${Boost_LIBRARIES} subscriber zmq boost_program_options SOCKETS)
target_link_libraries(${Boost_LIBRARIES} requester zmq boost_program_options SOCKETS)
target_link_libraries(${Boost_LIBRARIES} replier zmq boost_program_options SOCKETS)
