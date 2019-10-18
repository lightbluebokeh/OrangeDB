#pragma once

#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <defs.h>

#include "fs/utils/pagedef.h"
#include <mutex>
#include <cassert>
#include <unordered_set>
#include <unordered_map>
#include <fs/fileio/IdPool.h>

class BufPageManager;
class File;
class FileManager {
private:
    // FileTable ftable;
    int fd[MAX_FILE_NUM];
    IdPool pool;

    std::unordered_set<String> files;
    std::unordered_map<String, int> opened_files;
    String opened_filenames[MAX_FILE_NUM];

    FileManager() {}
    FileManager(const FileManager&) = delete;
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

    int close_file(int fileID) {
        pool.add(fileID);
        int f = fd[fileID];
        opened_files.erase(opened_filenames[fileID]);
        return close(f);
    }

    int create_file(const String& name) {
        if (file_exists(name)) return 0;
        FILE* f = fopen(name.c_str(), "a+");
        if (f == nullptr) {
            std::cout << "fail" << std::endl;
            return -1;
        }
        files.insert(name);
        fclose(f);
        return 0;
    }

    int open_file(const String& name, int& fileID) {
        if (!file_exists(name)) return -1;
        if (file_opened(name)) {
            fileID = opened_files[name];
            return 0;
        }
        int f = open(name.c_str(), O_RDWR);
        if (f == -1) return -1;
        // fileID = ftable.newFileID(name);
        fileID = pool.get();
        fd[fileID] = f;
        opened_files[name] = fileID;
        opened_filenames[fileID] = name;
        return 0;
    }

    int remove_file(const String& name) {
        if (!file_exists(name)) return 0;
        if (file_opened(name)) {
            return -1;
        }
        files.erase(name);
        int ret = remove(name.c_str());
        return ret;
    }
public:
    bool file_opened(const String& name) { return opened_files.count(name); }
    bool file_exists(const String& name) { return files.count(name); }

    static FileManager* get_instance() {
        static std::mutex mutex;
        static FileManager *instance = nullptr;
        if (instance == nullptr) {
            mutex.lock();
            if (instance == nullptr)
                instance = new FileManager;
            mutex.unlock();
        }
        return instance;
    }

    friend class BufPageManager;
    friend class File;
    friend int main();  // for test
};