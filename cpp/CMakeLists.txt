cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include(FindBoost)
#find_package(Boost ${MIN_BOOST_VERSION} REQUIRED system filesystem regex)

add_library(SOCKETS sockets/socket.cc)

include_directories(/home/caguero/local/include)
link_directories(/home/caguero/local/lib)
add_executable(publisher publisher.cc)
add_executable(subscriber subscriber.cc)
target_link_libraries(${Boost_LIBRARIES} publisher zmq boost_thread-mt boost_program_options SOCKETS)
target_link_libraries(${Boost_LIBRARIES} subscriber zmq boost_thread-mt boost_program_options SOCKETS)

