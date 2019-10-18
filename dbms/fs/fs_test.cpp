#include "bufmanager/BufPageManager.h"
#include "fileio/FileManager.h"
#include "utils/pagedef.h"
#include <iostream>
#include <record/file.h>
#include <fs/bufmanager/buf_page.h>

using namespace std;

constexpr int TEST_PAGE_NUM = BUF_CAP;

int main() {
    FileManager* fm = FileManager::get_instance();
    BufPageManager* bpm = BufPageManager::get_instance();

    fm->create_file("testfile.txt"); //新建文件
    fm->create_file("testfile2.txt");
    
    int f1, f2;
    fm->open_file("testfile.txt", f1); //打开文件，fileID是返回的文件id
    fm->open_file("testfile2.txt", f2);
    cerr << "file opened" << endl;

    cerr << "writing..." << endl;
    for (int page_id = 0; page_id < TEST_PAGE_NUM; ++ page_id) {
        BufPage buf_page = {f1, page_id};
        buf_page.write_obj(page_id);
        buf_page.write_obj(f1, sizeof(int));

        buf_page = {f2, page_id};
        buf_page.write_obj(page_id);
        buf_page.write_obj(f2, sizeof(int));
    }

    cerr << "checking buf..." << endl;
    for (int page_id = 0; page_id < TEST_PAGE_NUM; ++ page_id) {
        auto page = BufPage{f1, page_id};
        assert(page.get<int>() == page_id);
        assert(page.get<int>(sizeof(int)) == f1);

        page = BufPage{f2, page_id};
        assert(page.get<int>() == page_id);
        assert(page.get<int>(sizeof(int)) == f2);
    }
    cerr << "success" << endl;
    cerr << "checking write back..." << endl;
    bpm->close();
    for (int page_id = 0; page_id < TEST_PAGE_NUM; ++ page_id) {
        auto page = BufPage{f1, page_id};
        assert(page.get<int>() == page_id);
        assert(page.get<int>(sizeof(int)) == f1);

        page = BufPage{f2, page_id};
        assert(page.get<int>() == page_id);
        assert(page.get<int>(sizeof(int)) == f2);
    }
    cerr << "success" << endl;

    return 0;
}
