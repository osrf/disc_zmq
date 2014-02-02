cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
INCLUDE (CheckIncludeFiles)
include(FindBoost)

#Google Test
add_subdirectory( gtest-1.7.0 )
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib)

add_executable(UNIT_packet_TEST packet_TEST.cc)
add_executable(UNIT_topicsInfo_TEST topicsInfo_TEST.cc)
add_executable(UNIT_discZmq_TEST discZmq_TEST.cc)
add_executable(dns-sd_TEST dns-sd_TEST.cc)
add_executable(DNSServiceBrowse DNSServiceBrowse.cc)
add_executable(clientBrowse clientBrowse.cc)
add_executable(clientPublish clientPublish.cc)

target_link_libraries(${Boost_LIBRARIES} dns-sd_TEST avahi-client avahi-common)
target_link_libraries(${Boost_LIBRARIES} UNIT_packet_TEST zmq boost_program_options SOCKETS gtest gtest_main)
target_link_libraries(${Boost_LIBRARIES} UNIT_topicsInfo_TEST zmq boost_program_options SOCKETS gtest gtest_main)
target_link_libraries(${Boost_LIBRARIES} UNIT_discZmq_TEST zmq boost_program_options SOCKETS dns_sd gtest gtest_main avahi-client avahi-common boost_thread)
target_link_libraries(DNSServiceBrowse dns_sd)
target_link_libraries(clientBrowse avahi-client avahi-common)
target_link_libraries(clientPublish avahi-client avahi-common)