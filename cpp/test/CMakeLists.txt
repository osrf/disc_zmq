cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
INCLUDE (CheckIncludeFiles)
include(FindBoost)

#Google Test
add_subdirectory( gtest-1.7.0 )
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_executable(UNIT_packet_TEST packet_TEST.cc)
add_executable(UNIT_topicsInfo_TEST topicsInfo_TEST.cc)
add_executable(UNIT_discZmq_TEST discZmq_TEST.cc)

target_link_libraries(${Boost_LIBRARIES} UNIT_packet_TEST zmq boost_program_options SOCKETS gtest gtest_main)
target_link_libraries(${Boost_LIBRARIES} UNIT_topicsInfo_TEST zmq boost_program_options SOCKETS gtest gtest_main)
target_link_libraries(${Boost_LIBRARIES} UNIT_discZmq_TEST zmq boost_program_options SOCKETS gtest gtest_main)
