#include <stack>
#include <fcntl.h>
#include <unordered_set>
#include <unordered_map>

#include <fs/file/file_manage.h>
#include <fs/bufpage/bufpage_manage.h>

static int fd[MAX_FILE_NUM];
static std::stack<int> id_pool;

static std::unordered_set<String> files;
static std::unordered_map<String, int> opened_files;
// 其实就是上面的逆映射
static String filenames[MAX_FILE_NUM];

namespace FileManage {
    void init() {
        while (!id_pool.empty()) id_pool.pop();
        for (int i = 0; i < MAX_FILE_NUM; i++) {
            id_pool.push(i);
        }
    }

    int write_page(page_t page, bytes_t bytes, int off) {
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
    int read_page(page_t page, bytes_t bytes, int off) {
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
        opened_files.erase(filenames[file_id]);
        BufpageManage::wirte_back_file(file_id);
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
        if (id_pool.empty()) return -1;
        file_id = id_pool.top();
        id_pool.pop();
        ::fd[file_id] = fd;
        opened_files[name] = file_id;
        filenames[file_id] = name;
        return 0;
    }
    int remove_file(const String& name) {
        if (!file_exists(name)) return 0;
        if (file_opened(name)) close_file(opened_files[name]);
        files.erase(name);
        return remove(name.c_str());
    }
    bool file_opened(const String& name) { return opened_files.count(name); }
    bool file_exists(const String& name) { return files.count(name); }
}
