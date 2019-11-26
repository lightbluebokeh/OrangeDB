#include <catch/catch.hpp>

#include <fs/allocator/allocator.h>

inline void test_alloc() {
    FileAllocator falloc("alloc.txt");

    int a[501], b[400], c[200];
    for (int i = 0; i < 501; i++) a[i] = 0xaaaaaaaa;
    for (int i = 0; i < 400; i++) b[i] = 0xbbbbbbbb;
    for (int i = 0; i < 200; i++) c[i] = 0xcccccccc;

    // 检查分配
    size_t a_off = falloc.allocate(sizeof(a), a);
    size_t b_off = falloc.allocate(sizeof(b), b);
    size_t a2_off = falloc.allocate(sizeof(a), a);
    size_t b2_off = falloc.allocate(sizeof(b));
    falloc.write(b2_off, b, sizeof(b));
    printf("after some allocation:\n");
    falloc.print();

    // 检查释放
    REQUIRE(falloc.free(b_off));
    REQUIRE(falloc.free(a2_off));
    printf("after some deallocation:\n");
    falloc.print();

    // 检查释放后再分配
    size_t c_off = falloc.allocate(sizeof(c));
    falloc.write(c_off, c, sizeof(c));
    b_off = falloc.allocate(sizeof(b), b);
    printf("allocate something again:\n");
    falloc.print();

    // 检查数据对不对
    int* buffer = new int[1000];
    falloc.read(a_off, buffer, sizeof(a));
    for (int i = 0; i < 501; i++) REQUIRE(buffer[i] == a[i]);
    falloc.read(c_off, buffer, sizeof(c));
    for (int i = 0; i < 200; i++) REQUIRE(buffer[i] == c[i]);
    delete[] buffer;

    // 检查空闲块的合并
    REQUIRE(falloc.free(b2_off));
    REQUIRE(falloc.free(a_off));
    REQUIRE(falloc.free(c_off));
    REQUIRE(falloc.free(b_off));
    printf("deallocate everything:\n");
    falloc.print();

    // 检查double free
    REQUIRE_FALSE(falloc.free(a_off));
    c_off = falloc.allocate(sizeof(c), c);
    a_off = falloc.allocate(sizeof(a), a);
    printf("allocate something again:\n");
    falloc.print();

    // 再清空一次
    falloc.free(a_off);
    falloc.free(c_off);
    printf("deallocate everything:\n");
    falloc.print();

    // 检查文件
    REQUIRE(falloc.check());
}

TEST_CASE("file allocator", "[fs]") {
    fs::create_directories("test_dir");
    fs::current_path("test_dir");
    test_alloc();
    fs::remove("alloc.txt");
}
