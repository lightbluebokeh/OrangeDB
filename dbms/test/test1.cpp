// warning: 这个测试会产生两个总大小约 1G 的文件 /cy

#include <defs.h>

#include <fs/file/file.h>
#include <fs/file/file_manager.h>
#include <fs/bufpage/bufpage.h>
#include <fs/bufpage/bufpage_stream.h>
#include <utils/pagedef.h>

using namespace std;

constexpr int TEST_PAGE_NUM = BUF_CAP;

int main() {
    fs::create_directory("test_dir");
    fs::current_path("test_dir");
    cout << fs::current_path() << endl;
    fs::current_path("..");
    cout << fs::current_path() << endl;

    String name1 = "testfile1.txt", name2 = "testfile2.txt";
    File::create(name1);
    File::create(name2);

    auto f1 = File::open(name1), f2 = File::open(name2);
    cerr << "file opened" << endl;

    cerr << "writing..." << endl;
    for (int page_id = 0; page_id < TEST_PAGE_NUM; ++page_id) {
        f1->seek_pos(page_id << PAGE_SIZE_IDX);
        f1->write<int, 0>(page_id);
        f2->seek_pos(page_id << PAGE_SIZE_IDX);
        f2->write<int, 1>(page_id);
    }

    cerr << "checking buf..." << endl;
    for (int page_id = 0; page_id < TEST_PAGE_NUM; ++page_id) {
        f1->seek_pos(page_id << PAGE_SIZE_IDX);
        ensure(f1->read<int, 0>() == page_id, "unexpected result");
        f2->seek_pos(page_id << PAGE_SIZE_IDX);
        ensure(f2->read<int, 1>() == page_id, "unexpected result");
    }
    cerr << GREEN << "success" << RESET << endl;
    cerr << "checking write back..." << endl;
    BufpageManager::get_instance()->write_back();
    for (int page_id = 0; page_id < TEST_PAGE_NUM; ++page_id) {
        f2->seek_pos(page_id << PAGE_SIZE_IDX);
        ensure(f2->read<int, 0>() == page_id, "unexpected result");
    }
    cerr << GREEN << "success" << RESET << endl;

    ensure(f1->close(), "close failed");
    ensure(File::remove(name1), "remove failed");
    ensure(f2->close(), "close failed");
    ensure(File::remove(name2), "remove failed");

    cerr << "save your disk!" << endl;

    return 0;
}
