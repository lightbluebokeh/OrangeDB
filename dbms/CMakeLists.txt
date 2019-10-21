cmake_minimum_required(VERSION 3.10)
project(OrangeDBMS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin)

include_directories("./")

message("CMAKE_CXX_FLAGS_DEBUG is ${CMAKE_CXX_FLAGS_DEBUG}")
# CMAKE_CXX_FLAGS_DEBUG is -g

message("CMAKE_CXX_FLAGS_RELEASE is ${CMAKE_CXX_FLAGS_RELEASE}")
# CMAKE_CXX_FLAGS_RELEASE is -O3 -DNDEBUG

message("CMAKE_CXX_FLAGS_RELWITHDEBINFO is ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
# CMAKE_CXX_FLAGS_RELWITHDEBINFO is -O2 -g -DNDEBUG

message("CMAKE_CXX_FLAGS_MINSIZEREL is ${CMAKE_CXX_FLAGS_MINSIZEREL}")
# CMAKE_CXX_FLAGS_MINSIZEREL is -Os -DNDEBUG

# 设置编译方式
if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, set CMAKE_BUILD_TYPE to Release")
    set(CMAKE_BUILD_TYPE "Release")
else()
    message(STATUS "CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
endif()

if(UNIX)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
    set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address")

    add_compile_options("-Wall")
elseif(MSVC)
    add_compile_options("/utf-8")
    add_compile_definitions(_CRT_NONSTDC_NO_WARNINGS)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()
    
add_library(dbms SHARED dbms.cpp)
add_executable(test1 test/test1.cpp)
