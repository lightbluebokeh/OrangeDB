#include "defs.h"
#include "fs/file/file.h"
#include "fs/bufpage/bufpage.h"
#include "fs/file/file_manage.h"
#include "orange/table/table.h"

#include <catch/catch.hpp>

using namespace std;

namespace parser = Orange::parser;
using op_t = parser::op;

TEST_CASE("test fs io", "[fs]") {
    constexpr int TEST_PAGE_NUM = 50000;
    fs::create_directories("test_dir");
    fs::current_path("test_dir");

    String name1 = "testfile1.txt", name2 = "testfile2.txt";
    File::create(name1);
    File::create(name2);

    auto f1 = File::open(name1), f2 = File::open(name2);
    cerr << "file opened" << endl;

    cerr << "writing..." << endl;
    for (int page_id = 0; page_id < TEST_PAGE_NUM; ++page_id) {
        printf("\r page id: %d", page_id);
        f1->write(page_id << PAGE_SIZE_IDX, page_id + 666, std::vector<int>{page_id + 233, page_id + 2333});
        f2->write(page_id << PAGE_SIZE_IDX, page_id - 777, std::vector<int>{page_id - 62, page_id - 233, page_id - 2333});
    }

    cerr << "\nchecking buf..." << endl;
    for (int page_id = 0; page_id < TEST_PAGE_NUM; ++page_id) {
        size_t offset = page_id << PAGE_SIZE_IDX;
        int x;
        offset += f1->read(offset, x);
        REQUIRE(x == page_id + 666);
        std::vector<int> v;
        f1->read(offset, v);
        REQUIRE(v == std::vector<int>{page_id + 233, page_id + 2333});
        offset = page_id << PAGE_SIZE_IDX;
        offset += f2->read(offset, x);
        REQUIRE(x == page_id - 777);
        f2->read(offset, v);
        REQUIRE(v == std::vector<int>{page_id - 62, page_id - 233, page_id - 2333});
    }
    cerr << GREEN << "success" << RESET << endl;

    cerr << "checking write back..." << endl;
    BufpageManage::write_back_all();
    for (int page_id = 0; page_id < TEST_PAGE_NUM; ++page_id) {
        size_t offset = page_id << PAGE_SIZE_IDX;
        int x;
        offset += f1->read(offset, x);
        REQUIRE(x == page_id + 666);
        std::vector<int> v;
        f1->read(offset, v);
        REQUIRE(v == std::vector<int>{page_id + 233, page_id + 2333});
        offset = page_id << PAGE_SIZE_IDX;
        offset += f2->read(offset, x);
        REQUIRE(x == page_id - 777);
        f2->read(offset, v);
        REQUIRE(v == std::vector<int>{page_id - 62, page_id - 233, page_id - 2333});
    }
    cerr << GREEN << "success" << RESET << endl;

    orange_check(f1->close(), "close failed");
    orange_check(File::remove(name1), "remove failed");
    orange_check(f2->close(), "close failed");
    orange_check(File::remove(name2), "remove failed");

    cerr << "save your disk!" << endl;
}

TEST_CASE("table", "[fs]") {
    fs::remove_all("db");
    Orange::setup();

    Orange::create("test");
    Orange::use("test");

    SavedTable::create("test", {Column("test", 0, ast::data_type::int_type(), 0, ast::data_value::from_int(5))}, {}, {});
    cerr << "create table test" << endl;
    auto table = SavedTable::get("test");

    std::mt19937 rng(time(0));
    constexpr int lim = 5000;
    static int a[lim];
    std::multiset<int> all, rm;
    for (int i = 0; i < lim; i++) {
        a[i] = rng() % 5000;
        all.insert(a[i]);
        if (rng() & 1) rm.insert(a[i]);
    }
    std::cerr << "test insert" << std::endl;
    for (int i = 0; i < lim; i++) {
        table->insert({"test"}, {{ast::data_value::from_int(a[i])}});
    }

    std::cerr << "testing remove" << std::endl;
    int i = 0;
    for (int x : rm) {
        parser::single_where where = {parser::single_where_op{"test", parser::op::Eq, parser::data_value{x}}};
        auto delete_size = table->delete_where({where});
        REQUIRE(delete_size == all.count(x));
        while (all.count(x)) all.erase(x);
        // cerr << "x: " << x << endl;

        i++;
        std::cerr << '\r' << i << '/' << rm.size();
    }

    std::cerr << endl;

    Orange::unuse();

    Orange::paolu();
}

using namespace std;
static std::mt19937 rng(time(0));

TEST_CASE("btree", "[index]") {
    fs::remove_all("db");
    Orange::setup();

    Orange::create("test");
    Orange::use("test");

    SavedTable::create("test", {Column("test", 0, ast::data_type::int_type(), 0, ast::data_value::from_int(5))}, {}, {});
    cerr << "create table test" << endl;
    auto table = SavedTable::get("test");

    table->create_index("test", {"test"}, 0, 0);

    std::mt19937 rng(time(0));
    constexpr int lim = 50000;
    static int a[lim];
    std::multiset<int> all, rm;
    for (int i = 0; i < lim; i++) {
        a[i] = rng() % 5000;
        all.insert(a[i]);
        if (rng() % 2 == 0) rm.insert(a[i]);
    }
    std::cerr << "test insert" << std::endl;
    for (int i = 0; i < lim; i++) {
        table->insert({"test"}, {{ast::data_value::from_int(a[i])}});
        std::cerr << '\r' << i + 1 << '/' << lim;
    }
    std::cerr << endl;

    std::cerr << "testing remove" << std::endl;
    int i = 0;
    for (int x : rm) {
        parser::single_where where = {parser::single_where_op{"test", parser::op::Eq, parser::data_value{x}}};
        auto delete_size = table->delete_where({where});
        REQUIRE(delete_size == all.count(x));
        all.erase(x);
        i++;
        std::cerr << '\r' << i << '/' << rm.size();
    }
    table->drop_index("test");
    std::cerr << endl;

    Orange::unuse();

    Orange::paolu();
}

static String rand_str(int l, int r) {
    int len = l + rng() % (r - l + 1);
    String ret;
    for (int i = 0; i < len; i++) {
        ret.push_back(rng());
    }
    return ret;
}

TEST_CASE("varchar", "[index]") {
    fs::remove_all("db");
    Orange::setup();

    Orange::create("test");
    Orange::use("test");
    SavedTable::create("varchar", {Column("test", 0, ast::data_type::varchar_type(2333), 1, ast::data_value::from_string("2333"))}, {}, {});
    auto table = SavedTable::get("varchar");
    int lim = 1000;
    for (int i = 0; i < lim; i++) {
        table->insert({"test"}, {{ast::data_value::from_string(rand_str(3, 2333))}});
        std::cerr << '\r' << i << '/' << lim;
    }
    cerr << endl;
    table->create_index("test", {"test"}, 0, 0);
    for (int i = 0; i < lim; i++) {
        table->insert({"test"}, {{ast::data_value::from_string(rand_str(3, 2333))}});
        std::cerr << '\r' << i << '/' << lim;
    }
    cerr << endl;
    Orange::paolu();
}