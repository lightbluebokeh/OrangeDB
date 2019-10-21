cmake_minimum_required(VERSION 3.15)
project(OrangeDBMS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin)

if(UNIX)
    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        add_compile_options("-fsanitize=address")
        target_link_libraries(test "-fsanitize=address")
        add_definitions("-DDEBUG")   # Release
    else()
        add_compile_options("-Ofast")
        add_definitions("-DNDEBUG")
    endif()
elseif(MSVC)
    add_compile_options("/utf-8")
endif()

include_directories(
    "./"
)

add_library(dbms SHARED dbms.cpp)
add_executable(test test/test.cpp)

