cmake_minimum_required(VERSION 3.10)
project(OrangeDBMS)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin)

add_library(dbms SHARED dbms.cpp)
add_executable(fs_test fs/fs_test.cpp)

set(LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_HOME_DIRECTORY}/bin)

include_directories(
    "./"
)

if (UNIX)
    if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        add_compile_options("-ggdb")
        add_compile_options("-fsanitize=address")
        target_link_libraries(fs_test "-fsanitize=address")
        add_definitions("-DDEBUG")
    else()
        add_compile_options("-Ofast")
        add_definitions("-DNDEBUG")
    endif()
else()
    # WIN32
endif()

