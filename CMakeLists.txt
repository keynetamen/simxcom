cmake_minimum_required(VERSION 3.21)
project(simxcom VERSION 0.5)

add_executable(simxcom src/simxcom.c)

target_link_libraries(simxcom PRIVATE X11 cairo m)

target_compile_definitions(simxcom PUBLIC PROGRAM_NAME="${CMAKE_PROJECT_NAME}")
target_compile_definitions(simxcom PUBLIC VERSION="${CMAKE_PROJECT_VERSION}")