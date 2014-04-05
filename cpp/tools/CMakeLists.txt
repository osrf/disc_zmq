cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
INCLUDE (CheckIncludeFiles)
include(FindBoost)

# Binaries
add_executable(publisher publisher.cc)
add_executable(subscriber subscriber.cc)
add_executable(requester requester.cc)
add_executable(replier replier.cc)

target_link_libraries(publisher ${Boost_LIBRARIES} disczmq protobuf boost_program_options)
target_link_libraries(subscriber ${Boost_LIBRARIES} disczmq protobuf boost_program_options)
target_link_libraries(requester ${Boost_LIBRARIES} disczmq protobuf boost_program_options)
target_link_libraries(replier ${Boost_LIBRARIES} disczmq protobuf boost_program_options)