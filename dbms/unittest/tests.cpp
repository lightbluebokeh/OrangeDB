#include <defs.h>
#include <fs/bufpage/bufpage.h>
#include <fs/bufpage/bufpage_stream.h>
#include <fs/file/file.h>
#include <fs/file/file_manage.h>
#include <orange/table/table.h>

#include <catch/catch.hpp>

using namespace std;

TEST_CASE("test fs io", "[fs]") {
    constexpr int TEST_PAGE_NUM = 100000;
    fs::create_directory("test_dir");
    fs::current_path("test_dir");

    String name1 = "testfile1.txt", name2 = "testfile2.txt";
    File::create(name1);
    File::create(name2);

    auto f1 = File::open(name1), f2 = File::open(name2);
    cerr << "file opened" << endl;

    cerr << "writing..." << endl;
    for (int page_id = 0; page_id < TEST_PAGE_NUM; ++page_id) {
        printf("\r page id: %d", page_id);
        f1->seek_pos(page_id << PAGE_SIZE_IDX)
            ->write(page_id + 666, std::vector<int>{page_id + 233, page_id + 2333});
        f2->seek_pos(page_id << PAGE_SIZE_IDX)
            ->write(page_id - 777, std::vector<int>{page_id - 62, page_id - 233, page_id - 2333});
    }

    cerr << "\nchecking buf..." << endl;
    for (int page_id = 0; page_id < TEST_PAGE_NUM; ++page_id) {
        f1->seek_pos(page_id << PAGE_SIZE_IDX);
        REQUIRE(f1->read<int>() == page_id + 666);
        REQUIRE(f1->read<std::vector<int>>() == std::vector<int>{page_id + 233, page_id + 2333});
        f2->seek_pos(page_id << PAGE_SIZE_IDX);
        REQUIRE(f2->read<int>() == page_id - 777);
        REQUIRE(f2->read<std::vector<int>>() ==
                std::vector<int>{page_id - 62, page_id - 233, page_id - 2333});
    }
    cerr << GREEN << "success" << RESET << endl;

    cerr << "checking write back..." << endl;
    BufpageManage::write_back_all();
    for (int page_id = 0; page_id < TEST_PAGE_NUM; ++page_id) {
        f1->seek_pos(page_id << PAGE_SIZE_IDX);
        REQUIRE(f1->read<int>() == page_id + 666);
        REQUIRE(f1->read<std::vector<int>>() == std::vector<int>{page_id + 233, page_id + 2333});
        f2->seek_pos(page_id << PAGE_SIZE_IDX);
        REQUIRE(f2->read<int>() == page_id - 777);
        REQUIRE(f2->read<std::vector<int>>() ==
                std::vector<int>{page_id - 62, page_id - 233, page_id - 2333});
    }
    cerr << GREEN << "success" << RESET << endl;

    ensure(f1->close(), "close failed");
    ensure(File::remove(name1), "remove failed");
    ensure(f2->close(), "close failed");
    ensure(File::remove(name2), "remove failed");

    fs::current_path("..");
    fs::remove("test_dir");
    cerr << "save your disk!" << endl;
}

TEST_CASE("table", "[table]") {
    Orange::paolu();
    Orange::setup();

    Orange::create("test");
    Orange::use("test");

    Table::create("test", {Column("test", "INT", 0, 0, 1, {DATA_NULL, 0, 0, 0, 0}, {})}, {}, {});
    cerr << "create table test" << endl;
    auto table = Table::get("test");

    std::mt19937 rng(time(0));
    constexpr int lim = 5000;
    static int a[lim];
    std::unordered_multiset<int> all, rm;
    for (int i = 0; i < lim; i++) {
        a[i] = rng() % 5000;
        all.insert(a[i]);
        if (rng() & 1) rm.insert(a[i]);
    }
    std::cerr << "test insert" << std::endl;
    for (int i = 0; i < lim; i++) {
        table->insert({{"test", to_bytes(a[i])}});
    }

    std::cerr << "testing remove" << std::endl;
    int i = 0;
    for (int x : rm) {
        auto pos = table->where("test", pred_t{to_bytes(x), 1, to_bytes(x), 1}, lim);
        REQUIRE(pos.size() == all.count(x));
        all.erase(all.find(x));
        table->remove({pos.front()});
        i++;
        std::cerr << '\r' << i << '/' << rm.size();
    }

    table->drop_index("test");
    std::cerr << endl;

    Orange::unuse();
    Orange::drop("test");

    Orange::paolu();
}

using namespace std;

TEST_CASE("btree", "[btree]") {
    Orange::paolu();
    Orange::setup();

    Orange::create("test");
    Orange::use("test");

    Table::create("test", {Column("test", "INT", 0, 0, 1, {DATA_NULL, 0, 0, 0, 0}, {})}, {}, {});
    cerr << "create table test" << endl;
    auto table = Table::get("test");
    table->create_index("test");

    std::mt19937 rng(time(0));
    constexpr int lim = 50000;
    static int a[lim];
    std::unordered_multiset<int> all, rm;

    std::cerr << "test insert" << std::endl;
    for (int i = 0; i < lim; i++) {
        a[i] = rng() % 5000;
        all.insert(a[i]);
        int x = a[i];
        table->insert({{"test", to_bytes(a[i])}});
        auto pos = table->where("test", pred_t{to_bytes(x), 1, to_bytes(x), 1}, lim);
        REQUIRE(pos.size() == all.count(x));
        if (rng() & 1) rm.insert(a[i]);
        std::cerr << '\r' << i + 1 << '/' << lim;
    }
    cerr << endl;

    std::cerr << "testing remove" << std::endl;
    int i = 0;
    for (int x : rm) {
        auto pos = table->where("test", pred_t{to_bytes(x), 1, to_bytes(x), 1}, lim);
        REQUIRE(pos.size() == all.count(x));
        all.erase(all.find(x));
        table->remove({pos.front()});
        i++;
        std::cerr << '\r' << i << '/' << rm.size();
    }
    std::cerr << endl;

    Orange::drop("test");

    Orange::paolu();
}

#include <fs/allocator/allocator.h>

TEST_CASE("file allocator", "[fs]") {
    FileAllocator falloc("data.txt");

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
