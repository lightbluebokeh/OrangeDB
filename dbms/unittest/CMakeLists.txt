enable_testing()

add_executable(sometest tests.cpp main.cpp ${SRCS})

add_executable(allocator.test main.cpp allocator.cpp ${SRCS})
add_test(NAME allocator COMMAND allocator.test)

add_executable(parser.test main.cpp parser.cpp ${SRCS})
# target_compile_definitions(parser.test PUBLIC DEBUG_PARSER) # 调试解析器开这个
add_test(NAME parser COMMAND parser.test)
