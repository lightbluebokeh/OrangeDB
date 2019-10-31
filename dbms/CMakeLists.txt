cmake_minimum_required(VERSION 3.10)
project(OrangeDBMS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin)

include_directories(
    "src"
    "thirdparty"
    "thirdparty/mpfr"
)

set(SRCS 
    "src/defs.cpp"
    "src/orange/orange.cpp"
    "src/fs/file/file_manage.cpp"
    "src/fs/bufpage/bufpage_manage.cpp"
    # "thirdparty/mpfr/gmpfrxx.cpp"
    # "thirdparty/mpfr/mpfr_mul_d.c"
)
 
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
    link_libraries(-lstdc++fs)
    # link_libraries(-lgmp)
    # link_libraries(-lmpfr)
    add_compile_options("-Wall")
    add_compile_options("-Wextra")
elseif(MSVC)
    add_compile_options("/utf-8")
    add_compile_definitions(_CRT_NONSTDC_NO_WARNINGS)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

    
add_library(dbms SHARED src/dbms.cpp ${SRCS})
add_executable(test test/tests.cpp test/main.cpp ${SRCS})
