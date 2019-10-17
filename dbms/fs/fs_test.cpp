/*
 * testfilesystem.cpp
 *
 *  Created on: 2015年10月6日
 *      Author: lql
 *          QQ: 896849432
 * 各位同学十分抱歉，之前给的样例程序不支持同时打开多个文件
 * 是因为初始化没有做，现在的程序加上了初始化（main函数中的第一行）
 * 已经可以支持多个文件的维护
 *
 * 但是还是建议大家只维护一个文件，因为首先没有必要，我们可以把数据库中
 * 的索引和数据都放在同一个文件中，当我们开启一个数据库时，就关掉上一个
 * 其次，多个文件就要对应多个fileID，在BufPageManager中是利用一个hash函数
 * 将(fileID,pageID)映射为一个整数，但由于我设计的hash函数过于简单，就是fileID和
 * pageID的和，所以不同文件的页很有可能映射为同一个数，增加了hash的碰撞率，影响效率
 * 
 * 还有非常重要的一点，bytes_t b = bpm->allocPage(...)
 * 在利用上述allocPage函数或者getPage函数获得指向申请缓存的指针后，
 * 不要自行进行类似的delete[] b操作，内存的申请和释放都在BufPageManager中做好
 * 如果自行进行类似free(b)或者delete[] b的操作，可能会导致严重错误
 */
#include "bufmanager/BufPageManager.h"
#include "fileio/FileManager.h"
#include "utils/pagedef.h"
#include <iostream>
#include <fs/bufmanager/buf_page.h>

using namespace std;

int main() {
    MyBitMap::initConst();   //新加的初始化
    FileManager* fm = FileManager::get_instance();
    BufPageManager* bpm = BufPageManager::get_instance();
    fm->create_file("testfile.txt"); //新建文件
    fm->create_file("testfile2.txt");
    int fileID, f2;
    fm->open_file("testfile.txt", fileID); //打开文件，fileID是返回的文件id
        fm->open_file("testfile2.txt", f2);
    for (int pageID = 0; pageID < 1000; ++ pageID) {
        // bytes_t b = bpm->allocPage(fileID, pageID, index, false);
        BufPage buf_page = {fileID, pageID};

        // b[0] = pageID; //对缓存页进行写操作
        // b[1] = fileID;
        // bpm->markDirty(index); //标记脏页
        buf_page.write_obj(pageID);
        buf_page.write_obj(fileID, sizeof(int));

        //在重新调用allocPage获取另一个页的数据时并没有将原先b指向的内存释放掉
        //因为内存管理都在BufPageManager中做好了
        // b = bpm->allocPage(f2, pageID, index, false);
        // b[0] = pageID;
        // b[1] = f2;
        // bpm->markDirty(index);
        buf_page = {f2, pageID};
        buf_page.write_obj(pageID, sizeof(int));

    }
    for (int pageID = 0; pageID < 1000; ++ pageID) {
        int* b = (int*)BufPage{fileID, pageID}.get_bytes();
        cout << b[0] << ":" << b[1] << endl;
        b = (int*)BufPage{f2, pageID}.get_bytes();
        cout << b[0] << ":" << b[1] << endl;
    }
    bpm->close();
    return 0;
}
