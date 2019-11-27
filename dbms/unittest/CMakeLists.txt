add_executable(sometest tests.cpp main.cpp ${SRCS})
add_executable(allocator.test main.cpp allocator.cpp ${SRCS})
add_executable(parser.test main.cpp parser.cpp ${SRCS})

enable_testing()
add_test(NAME allocator COMMAND allocator.test)
add_test(NAME parser COMMAND parser.test)
