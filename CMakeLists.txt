cmake_minimum_required(VERSION 3.10)
project(DBProject-backend)

set(CXX_STANDARD 17)

find_package(RapidJSON CONFIG REQUIRED)

add_library(dbms SHARED src/dbms.cpp)
target_include_directories(dbms PRIVATE ${RAPIDJSON_INCLUDE_DIRS})