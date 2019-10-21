cmake_minimum_required(VERSION 3.10)
project(OrangeDBMS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin)

include_directories(
    "./"
)

if(UNIX)
    add_library(dbms SHARED dbms.cpp)
    add_executable(test1 test/test1.cpp)
    if(CMAKE_BUILD_TYPE STREQUAL Debug)
        add_compile_options("-fsanitize=address")
        target_link_libraries(test1 "-fsanitize=address")
        add_definitions("-DDEBUG")
    else()
        add_compile_options("-Ofast")
        add_definitions("-DNDEBUG")
    endif()
    add_compile_options("-Wall")
    add_compile_options("-Wextra")
elseif(MSVC)
    add_compile_options("/utf-8")
    add_compile_options("/W4")
    add_library(dbms SHARED dbms.cpp)
    add_executable(test1 test/test1.cpp)
endif()

