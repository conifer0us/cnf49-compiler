set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(irpasses ir.cpp vn.cpp ssa.cpp)

target_include_directories(irpasses PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
