set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(compdef ir.cpp ssa.cpp)

target_include_directories(compdef PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
