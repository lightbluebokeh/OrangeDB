cmake_minimum_required(VERSION 3.10)
project(OrangeDBMS)

set(CMAKE_CXX_STANDARD 17)

include_directories(
    "./"
)

if (UNIX)
    add_compile_options(-Ofast)
    # add_compile_options(-Wall Wextra)
endif()

add_library(dbms SHARED dbms.cpp)
target_include_directories(dbms PRIVATE)