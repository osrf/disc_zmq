cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
INCLUDE (CheckIncludeFiles)
include(FindBoost)

#Google Test
add_subdirectory( gtest-1.7.0 )
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_executable(publisher publisher.cc)
add_executable(subscriber subscriber.cc)
add_executable(requester requester.cc)
add_executable(replier replier.cc)

target_link_libraries(publisher ${Boost_LIBRARIES} disczmq protobuf boost_program_options)
target_link_libraries(subscriber ${Boost_LIBRARIES} disczmq protobuf boost_program_options)
target_link_libraries(requester ${Boost_LIBRARIES} disczmq protobuf boost_program_options)
target_link_libraries(replier ${Boost_LIBRARIES} disczmq protobuf boost_program_options)

add_executable(UNIT_packet_TEST packet_TEST.cc)
add_executable(UNIT_topicsInfo_TEST topicsInfo_TEST.cc)
add_executable(UNIT_discZmq_TEST discZmq_TEST.cc)

target_link_libraries(UNIT_packet_TEST disczmq ${Boost_LIBRARIES} gtest gtest_main)
target_link_libraries(UNIT_topicsInfo_TEST disczmq gtest gtest_main)
target_link_libraries(UNIT_discZmq_TEST disczmq gtest gtest_main)