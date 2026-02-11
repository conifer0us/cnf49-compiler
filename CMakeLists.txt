cmake_minimum_required(VERSION 3.10.0)
project(sufferlang VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(${PROJECT_SOURCE_DIR}/compdef)
add_subdirectory(${PROJECT_SOURCE_DIR}/frontend)

add_executable(comp comp.cpp)
target_link_libraries(comp PUBLIC compdef frontend)
