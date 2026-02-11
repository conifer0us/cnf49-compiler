set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(frontend tokenizer.cpp parser.cpp ASTtoIR.cpp irbuilder.cpp ASTNodes.cpp)

target_link_libraries(frontend PUBLIC compdef)
target_include_directories(frontend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
