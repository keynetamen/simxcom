cmake_minimum_required(VERSION 3.21)
project(simxcom VERSION 0.1.0)

add_executable(simxcom src/simxcom.c)

target_link_libraries(simxcom PRIVATE X11)