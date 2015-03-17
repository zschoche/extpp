cmake_minimum_required(VERSION 2.8) 
project(etools)

add_definitions("-std=c++1y")
add_definitions("-Wall")
#add_definitions("-O3")
add_definitions("-g")
add_definitions("-fexceptions")

#defines
add_executable(etools main.cpp) 
target_link_libraries(etools boost_program_options boost_filesystem boost_system)
