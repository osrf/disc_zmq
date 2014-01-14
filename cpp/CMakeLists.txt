cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
INCLUDE (CheckIncludeFiles)
include(FindBoost)

#Google Test
add_subdirectory( test/gtest-1.7.0 )
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

CHECK_INCLUDE_FILES(ifaddrs.h HAVE_IFADDRS_H)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.hh.in ${CMAKE_CURRENT_BINARY_DIR}/config.hh)

add_library(SOCKETS sockets/socket.cc)

add_executable(publisher publisher.cc)
add_executable(subscriber subscriber.cc)
add_executable(requester requester.cc)
add_executable(replier replier.cc)
add_executable(test/UNIT_packet_TEST test/packet_TEST.cc)
target_link_libraries(${Boost_LIBRARIES} publisher zmq boost_program_options SOCKETS)
target_link_libraries(${Boost_LIBRARIES} subscriber zmq boost_program_options SOCKETS)
target_link_libraries(${Boost_LIBRARIES} requester zmq boost_program_options SOCKETS)
target_link_libraries(${Boost_LIBRARIES} replier zmq boost_program_options SOCKETS)
target_link_libraries(${Boost_LIBRARIES} test/UNIT_packet_TEST zmq boost_program_options SOCKETS gtest gtest_main)
