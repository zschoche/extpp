cmake_minimum_required(VERSION 2.8) 
project(ext2fuse)

add_definitions("-std=c++1y")
add_definitions("-Wall")
#add_definitions("-O3")
add_definitions("-g")
add_definitions("-fexceptions")

add_definitions(-D_FILE_OFFSET_BITS=64)
add_definitions(-DFUSE_USE_VERSION=26)

add_executable(e2mount main.cpp ext2.cpp) 
add_custom_target(check DEPENDS e2mount COMMAND ./ext2fuse)
#target_link_libraries(host_tests boost_unit_test_framework)
target_link_libraries(e2mount osxfuse)

