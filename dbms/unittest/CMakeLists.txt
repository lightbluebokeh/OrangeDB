add_executable(test tests.cpp main.cpp ${SRCS})
add_executable(allocator.test main.cpp allocator.cpp ${SRCS})
add_executable(parser.test main.cpp parser.cpp ${SRCS})

enable_testing()
# 这两个还不能用，有空再修
add_test(NAME allocator COMMAND allocator.test)
add_test(NAME parser COMMAND parser.test)
