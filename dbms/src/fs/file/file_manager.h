#pragma once

#include <fcntl.h>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

#include <defs.h>

class BufpageManager;
class File;

class FileManager {
private:
    int fd[MAX_FILE_NUM];
    std::stack<int> id_pool;

    std::unordered_set<String> files;
    std::unordered_map<String, int> opened_files;
    String opened_filenames[MAX_FILE_NUM];

    FileManager() {
        for (int i = 0; i < MAX_FILE_NUM; i++) {
            id_pool.push(i);
        }
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

    int close_file(int file_id) {
        id_pool.push(file_id);
        int f = fd[file_id];
        opened_files.erase(opened_filenames[file_id]);
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

    // return fd if success
    int open_file(const String& name, int& file_id, int& fd) {
        if (!file_exists(name)) return -1;
        if (file_opened(name)) {
            file_id = opened_files[name];
            return 0;
        }
#if __unix__
        fd = open(name.c_str(), O_RDWR);
#else
        fd = open(name.c_str(), O_RDWR | O_BINARY);
#endif
        if (fd == -1) return -1;
        file_id = id_pool.top();
        id_pool.pop();
        this->fd[file_id] = fd;
        opened_files[name] = file_id;
        opened_filenames[file_id] = name;
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

    friend class BufpageManager;
    friend class File;
    friend int main();  // for test
};