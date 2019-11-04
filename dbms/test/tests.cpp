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
    fs::create_directory("db");
    fs::current_path("db");
    Orange::create("test");
    Orange::use("test");

    // Table::create("test", {
    //     col_t("test", "INT", 1, {0, 0}),
    // }, {}, {});
}
