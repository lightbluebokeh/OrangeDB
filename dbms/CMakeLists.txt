cmake_minimum_required(VERSION 3.15)
project(OrangeDBMS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin)

find_package(Boost REQUIRED)

include_directories(
    "src"
    "thirdparty"
    ${Boost_INCLUDE_DIR}
)

set(SRCS
    "src/defs.cpp"
    "src/orange/orange.cpp"
    "src/fs/file/file_manage.cpp"
    "src/fs/bufpage/bufpage_manage.cpp"
    "src/orange/index/b_tree.cpp"
    "src/orange/index/index.cpp"
    "src/fs/allocator/allocator.cpp"
    "src/orange/parser/parser.cpp"
)

# 设置编译方式
if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, set CMAKE_BUILD_TYPE to Release")
    set(CMAKE_BUILD_TYPE "Release")
else()
    message(STATUS "CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
endif()

if(UNIX)
    # set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
    # set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address")
    link_libraries(-lstdc++fs)
    # link_libraries(-lgmp)
    # link_libraries(-lmpfr)
    add_compile_options("-Wno-parentheses")
    add_compile_options("-Wall")
    add_compile_options("-Wextra")
elseif(MSVC)
    add_compile_options("/utf-8")
    add_compile_options("/bigobj")
    add_compile_definitions(_CRT_NONSTDC_NO_WARNINGS)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

add_library(dbms SHARED src/dbms.cpp ${SRCS})
add_executable(test unittest/tests.cpp unittest/main.cpp ${SRCS})
