cmake_minimum_required(VERSION 2.8) 
project(test)

add_definitions("-std=c++1y")
add_definitions("-stdlib=libc++")
add_definitions("-Wall")
#add_definitions("-O3")
#add_definitions("-g")
add_definitions("-fexceptions")

#defines
#add_definitions(-DBOOST_ASIO_DYN_LINK)
#tests
add_executable(host_tests tests.cpp) 
	    add_custom_target(check DEPENDS host_tests COMMAND ./host_tests)
target_link_libraries(host_tests boost_unit_test_framework)
