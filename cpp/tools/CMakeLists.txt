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

# Install the binaries
set_target_properties(publisher PROPERTIES VERSION ${DISCZMQ_VERSION_FULL})
install (TARGETS publisher DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
set_target_properties(subscriber PROPERTIES VERSION ${DISCZMQ_VERSION_FULL})
install (TARGETS subscriber DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
set_target_properties(requester PROPERTIES VERSION ${DISCZMQ_VERSION_FULL})
install (TARGETS requester DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
set_target_properties(replier PROPERTIES VERSION ${DISCZMQ_VERSION_FULL})
install (TARGETS replier DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)