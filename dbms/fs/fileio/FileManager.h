#pragma once

#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <defs.h>

#include "fs/utils/MyBitMap.h"
#include "fs/utils/pagedef.h"
#include "fs/fileio/FileTable.h"
#include <mutex>

class FileManager {
private:
    FileTable ftable;
    int fd[MAX_FILE_NUM];

    FileManager() {}
    FileManager(const FileManager&) = delete;
public:
    /*
     * @函数名writePage
     * @参数fileID:文件id，用于区别已经打开的文件
     * @参数pageID:文件的页号
     * @参数buf:存储信息的缓存(4字节无符号整数数组)
     * @参数off:偏移量
     * 功能:将buf+off开始的8kb字节写入fileID和pageID指定的文件页中
     * 返回:成功操作返回0
     */
    int write_page(page_t page, bytes_t buf, int off = 0) {
        int f = fd[page.file_id];
        off_t offset = page.page_id;
        offset = (offset << PAGE_SIZE_IDX);
        off_t error = lseek(f, offset, SEEK_SET);
        if (error != offset) {
            return -1;
        }
        bytes_t b = buf + off;
        error = write(f, b, PAGE_SIZE);
        return 0;
    }
    /*
     * @函数名readPage
     * @参数fileID:文件id，用于区别已经打开的文件
     * @参数pageID:文件页号
     * @参数buf:存储信息的缓存(4字节无符号整数数组)
     * @参数off:偏移量
     * 功能:将fileID和pageID指定的文件页中 8192 字节(8kb)读入到buf+off开始的内存中
     * 返回:成功操作返回0
     */
    int read_page(page_t page, bytes_t buf, int off) {
        // int f = fd[fID[type]];
        int f = fd[page.file_id];
        off_t offset = page.page_id;
        offset = (offset << PAGE_SIZE_IDX);
        off_t error = lseek(f, offset, SEEK_SET);
        if (error != offset) {
            return -1;
        }
        bytes_t b = buf + off;
        error = read(f, (void*)b, PAGE_SIZE);
        return 0;
    }
    /*
     * @函数名closeFile
     * @参数fileID:用于区别已经打开的文件
     * 功能:关闭文件
     * 返回: 返回 close 返回值
     */
    int close_file(int fileID) {
        // fm->setBit(fileID, 1);
        ftable.freeFileID(fileID);
        int f = fd[fileID];
        return close(f);
    }
    /*
     * @函数名createFile
     * @参数name:文件名
     * 功能:新建name指定的文件名
     * 返回:操作成功，返回 0，失败 -1
     */
    int create_file(const char* name) {
        // return _createFile(name);
        if (ftable.file_exists(name)) return 1;
        FILE* f = fopen(name, "a+");
        if (f == nullptr) {
            std::cout << "fail" << std::endl;
            return -1;
        }
        fclose(f);
        return 0;
    }
    /*
     * @函数名openFile
     * @参数name:文件名
     * @参数fileID:函数返回时，如果成功打开文件，那么为该文件分配一个id，记录在fileID中
     * 功能:打开文件，不存在就创建
     * 返回:如果成功打开，在fileID中存储为该文件分配的id，返回true，否则返回false
     */
    int open_file(const char* name, int& fileID) {
        // fileID = fm->findLeftOne();
        // fm->setBit(fileID, 0);
        if (!ftable.file_exists(name)) return -1;
        if (ftable.file_opened(name)) {
            fileID = ftable.getFileID(name);
            return 0;
        }
        int f = open(name, O_RDWR);
        if (f == -1) return -1;
        fileID = ftable.newFileID(name);
        fd[fileID] = f;
        return 0;
    }

    int remove_file(const String& name) {
        if (!file_exists(name)) return 0;
        if (file_opened(name)) {
            return -1;
        }
        return remove(name.c_str());
    }

    int newType() {
        // int t = tm->findLeftOne();
        // tm->setBit(t, 0);
        // return t;
        return ftable.newTypeID();
    }
    void closeType(int typeID) {
        // tm->setBit(typeID, 1);
        ftable.freeTypeID(typeID);
    }

    bool file_opened(const String& name) { return ftable.file_exists(name); }
    bool file_exists(const String& name) { return ftable.file_opened(name); }

    static FileManager* get_instance() {
        static std::mutex mutex;
        static FileManager* instance = nullptr;
        if (instance == nullptr) {
            mutex.lock();
            if (instance == nullptr)
                instance = new FileManager;
            mutex.unlock();
        }
        return instance;
    }
};