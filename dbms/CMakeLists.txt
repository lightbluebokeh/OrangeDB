cmake_minimum_required(VERSION 3.10)
project(dbms)

set(CXX_STANDARD 17)

include_directories(
    "./"
)

if (UNIX)
    add_compile_options(-Ofast)
endif()

add_library(dbms SHARED dbms.cpp)
target_include_directories(dbms PRIVATE ${RAPIDJSON_INCLUDE_DIRS})