#include <fs/bufpage/bufpage_manage.h>
#include <fs/bufpage/bufpage_map.h>
#include <fs/file/file_manage.h>
#include <utils/find_replace.h>

// FileManager* fileManager;
static FindReplace replace;
static BufpageMap bufpage_map;
static bool dirty[BUF_CAP];
static bytes_t addr[BUF_CAP];
static bytes_t alloc_page() {
    return new byte_t[PAGE_SIZE];
}

static void access(int buf_id) {
    static int last = -1;
    if (buf_id == last) return;
    replace.access(buf_id);
    last = buf_id;
}

static void write_back(int buf_id) {
    if (dirty[buf_id]) {
        auto page = bufpage_map.get_page(buf_id);
        FileManage::write_page(page, addr[buf_id]);
        dirty[buf_id] = false;
        access(buf_id);
    }
}

// 为 page 分配新的缓存编号
static buf_t fetch_page(page_t page) {
    bytes_t b;
    int buf_id = replace.find();
    b = addr[buf_id];
    if (b == nullptr) {
        b = alloc_page();
        addr[buf_id] = b;
    } else {
        write_back(buf_id);
    }

    bufpage_map.set_map(page, buf_id);
    return {b, buf_id};
}

static buf_t get_page_buf(page_t page) {
    int buf_id = bufpage_map.get_buf_id(page);
    if (buf_id == -1) {
        auto buf = fetch_page(page);
        FileManage::read_page(page, buf.bytes);
        return buf;
    } else {
        access(buf_id);
        return {addr[buf_id], buf_id};
    }
}

namespace BufpageManage {
    void write_back() {
        for (int i = 0; i < BUF_CAP; ++i) {
            ::write_back(i);
        }
    }

    // 要确保 buf_id 被使用过
    page_t get_page(int buf_id) { return bufpage_map.get_page(buf_id); }

    bool tracking(page_t page) { return bufpage_map.contains(page); }

    void write_back_file(int file_id) {
        for (int buf_id : bufpage_map.get_all(file_id)) {
            ::write_back(buf_id);
        }
    }

    buf_t get_page_buf(page_t page) {
        int buf_id = bufpage_map.get_buf_id(page);
        if (buf_id == -1) {
            auto buf = fetch_page(page);
            FileManage::read_page(page, buf.bytes);
            return buf;
        } else {
            access(buf_id);
            return {addr[buf_id], buf_id};
        }
    }


    void mark_dirty(int buf_id) { dirty[buf_id] = true; }
}  // namespace BufpageManage