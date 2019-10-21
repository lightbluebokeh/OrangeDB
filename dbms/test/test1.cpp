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
    FileManager* fm = FileManager::get_instance();
    BufpageManager* bpm = BufpageManager::get_instance();

    fm->create_file("testfile.txt");  //新建文件
    fm->create_file("testfile2.txt");

    int f1, f2;
    fm->open_file("testfile.txt", f1);  //打开文件，fileID是返回的文件id
    fm->open_file("testfile2.txt", f2);
    cerr << "file opened" << endl;

    cerr << "writing..." << endl;
    for (int page_id = 0; page_id < TEST_PAGE_NUM; ++page_id) {
        Bufpage page = {f1, page_id};
        BufpageStream bps(page);
        bps.write(page_id).write(f1);

        page = {f2, page_id};
        bps = BufpageStream(page);
        bps.write(page_id).write(f2);
    }

    cerr << "checking buf..." << endl;
    for (int page_id = 0; page_id < TEST_PAGE_NUM; ++page_id) {
        Bufpage page = {f1, page_id};
        BufpageStream bps(page);
        ensure(bps.read<int>() == page_id, "unexpected result");
        ensure(bps.read<int>() == f1, "unexpected result");

        page = Bufpage{f2, page_id};
        bps = BufpageStream(page);
        ensure(bps.read<int>() == page_id, "unexpected result");
        ensure(bps.read<int>() == f2, "unexpected result");
    }
    cerr << GREEN << "success" << RESET << endl;
    cerr << "checking write back..." << endl;
    bpm->close();
    for (int page_id = 0; page_id < TEST_PAGE_NUM; ++page_id) {
        Bufpage page = {f1, page_id};
        BufpageStream bps(page);
        ensure(bps.read<int>() == page_id, "unexpected result");
        ensure(bps.read<int>() == f1, "unexpected result");

        page = Bufpage{f2, page_id};
        bps = BufpageStream(page);
        ensure(bps.read<int>() == page_id, "unexpected result");
        ensure(bps.read<int>() == f2, "unexpected result");
    }
    cerr << GREEN << "success" << RESET << endl;

    ensure(fm->close_file(f1) == 0, "close failed");
    ensure(fm->remove_file("testfile.txt") == 0, "remove failed");
    ensure(fm->close_file(f2) == 0, "close failed");
    ensure(fm->remove_file("testfile2.txt") == 0, "remove failed");

    cerr << "save your disk!" << endl;

    return 0;
}
