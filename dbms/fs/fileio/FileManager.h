#pragma once

#include <fcntl.h>

#include <stack>
#include <unordered_map>
#include <unordered_set>

#include <mutex>

#include <defs.h>
#include <fs/utils/pagedef.h>

class BufPageManager;
class File;
class FileManager {
private:
    int fd[MAX_FILE_NUM];
    std::stack<int> id_pool;

    std::unordered_set<String> files;
    std::unordered_map<String, int> opened_files;
    String opened_filenames[MAX_FILE_NUM];

    FileManager() {
        for (int i = 0; i < MAX_FILE_NUM; i++) id_pool.push(i);
    }
    FileManager(const FileManager&) = delete;

    int write_page(page_t page, bytes_t bytes, int off = 0) {
        int f = fd[page.file_id];
        off_t offset = page.page_id;
        offset = (offset << PAGE_SIZE_IDX);
        off_t error = lseek(f, offset, SEEK_SET);
        if (error != offset) {
            return -1;
        }
        bytes_t b = bytes + off;
        error = write(f, b, PAGE_SIZE);
        return 0;
    }

    int read_page(page_t page, bytes_t bytes, int off = 0) {
        int f = fd[page.file_id];
        off_t offset = page.page_id;
        offset = (offset << PAGE_SIZE_IDX);
        off_t error = lseek(f, offset, SEEK_SET);
        if (error != offset) {
            return -1;
        }
        bytes_t b = bytes + off;
        error = read(f, (void*)b, PAGE_SIZE);
        return 0;
    }

    int close_file(int fileID) {
        id_pool.push(fileID);
        int f = fd[fileID];
        opened_files.erase(opened_filenames[fileID]);
        return close(f);
    }

    int create_file(const String& name) {
        if (file_exists(name)) return 0;
        FILE* f = fopen(name.c_str(), "ab+");
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
        int f = open(name.c_str(), O_RDWR | O_BINARY);
        if (f == -1) return -1;
        fileID = id_pool.top();
        id_pool.pop();
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
        static FileManager* instance = nullptr;
        if (instance == nullptr) {
            mutex.lock();
            if (instance == nullptr) instance = new FileManager;
            mutex.unlock();
        }
        return instance;
    }

    friend class BufPageManager;
    friend class File;
    friend int main();  // for test
};