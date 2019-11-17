#include "allocator.h"

#include <cassert>
#include <fcntl.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

// 数据标记
struct _Tag {
    int size : 30;
    int af : 2;
};

// 初始化
void FileAllocator::init() {
    lseek(fd, DATA_SEEK, SEEK_SET);
    _Tag tag{0, 1};  // 可视为 header
    ::write(fd, &tag, sizeof(tag));
}

// 构造函数
FileAllocator::FileAllocator(const String& name) {
    if (!fs::exists(name)) {
        fd = open(name.c_str(), O_RDWR | O_BINARY);
        init();
    } else {
        fd = open(name.c_str(), O_RDWR | O_BINARY);
    }
}

// 分配空间
size_t FileAllocator::alloc(size_t size, void* data) {
    assert((int)size > 0);
    lseek(fd, DATA_SEEK + sizeof(_Tag), SEEK_SET);
    _Tag tag[1];

    // 对齐
    if (int m = size & (ALIGN_SIZE - 1)) {
        size ^= m;
        size += ALIGN_SIZE;
    }
    // 找到一块空闲的地方, 没用优化分配, 暴力找
    while (true) {
        if (::read(fd, tag, sizeof(tag)) < (int)sizeof(tag)) {
            tag[0].size = size;  // 找到文件结尾了
            break;
        }
        if ((size_t)tag[0].size >= size && tag[0].af == 0) {
            break;
        }
        lseek(fd, tag[0].size + 2 * sizeof(tag), SEEK_CUR);
    }
    long offset = tell(fd) + sizeof(tag);

    tag[0].af = 1;
    if ((size_t)tag[0].size < size + 2 * sizeof(tag) + ALIGN_SIZE) {
        ::write(fd, tag, sizeof(tag));
        if (data) {
            int count = ::write(fd, data, size);
            lseek(fd, tag[0].size - count, SEEK_CUR);
        } else {
            lseek(fd, tag[0].size, SEEK_CUR);
        }
        ::write(fd, tag, sizeof(tag));
    } else {
        int seg_size = tag[0].size;
        tag[0].size = size;
        ::write(fd, tag, sizeof(tag));
        if (data) {
            int count = ::write(fd, data, size);
            lseek(fd, size - count, SEEK_CUR);
        } else {
            lseek(fd, size, SEEK_CUR);
        }
        ::write(fd, tag, sizeof(tag));
        tag[0] = {seg_size - (int)size - 2 * (int)sizeof(tag), 0};
        ::write(fd, tag, sizeof(tag));
        lseek(fd, tag[0].size, SEEK_CUR);
        ::write(fd, tag, sizeof(tag));
    }

    return offset;
}

// 读取
int FileAllocator::read(size_t offset, void* dst, int max_count) {
    _Tag tag;
    lseek(fd, offset - sizeof(tag), SEEK_SET);
    ::read(fd, &tag, sizeof(tag));

    if (tag.af != 1) {
        return -1;  // offset是假的?
    }

    if (tag.size < max_count) max_count = tag.size;
    return ::read(fd, dst, max_count);
}

// 写入
int FileAllocator::write(size_t offset, void* data, int max_count) {
    _Tag tag;
    lseek(fd, offset - sizeof(tag), SEEK_SET);
    ::read(fd, &tag, sizeof(tag));

    if (tag.af != 1) {
        return -1;
    }

    if (tag.size < max_count) max_count = tag.size;
    return ::write(fd, data, max_count);
}

// 释放
bool FileAllocator::free(size_t offset) {
    _Tag tag[2];  // tag[0]: 上一个块的footer, tag[1]: 当前块
    lseek(fd, offset - 2 * sizeof(_Tag), SEEK_SET);
    ::read(fd, tag, 2 * sizeof(_Tag));

    if (tag[1].af == 0) {
        return false;  // double free
    }

    int size = tag[1].size;
    if (tag[0].af == 0) {  // 跟前一块合并
        size += tag[0].size + 2 * sizeof(_Tag);
    }

    lseek(fd, tag[1].size, SEEK_CUR);  // 看看后一块
    if (::read(fd, tag, 2 * sizeof(_Tag)) == 2 * sizeof(_Tag) && tag[1].af == 0) {
        // 跟后一块合并
        lseek(fd, -size - 3 * sizeof(_Tag), SEEK_CUR);
        size += tag[1].size + 2 * sizeof(_Tag);
        tag[1].size = size;
        ::write(fd, &tag[1], sizeof(tag[1]));
        lseek(fd, size, SEEK_CUR);
        ::write(fd, &tag[1], sizeof(tag[1]));
    } else {  // 文件尾或无法合并
        tag[0].size = size;
        tag[0].af = 0;
        lseek(fd, -(int)sizeof(tag[0]), SEEK_CUR);  // 只读了一个tag所以只回退一个
        ::write(fd, tag, sizeof(tag[0]));
        lseek(fd, -size - sizeof(_Tag), SEEK_CUR);
        ::write(fd, tag, sizeof(_Tag));
    }

    return true;
}

// 检查
bool FileAllocator::check() {
    lseek(fd, DATA_SEEK, SEEK_SET);
    _Tag tag_header, tag_footer;

    // 检查头部
    ::read(fd, &tag_footer, sizeof(tag_footer));
    if (tag_footer.af != 1) return false;  // 头部af错误

    while (true) {
        if (int c = ::read(fd, &tag_header, sizeof(tag_header)); c < (int)sizeof(tag_header)) {
            return c == 0;  // header不完整
        }
        if (tag_header.af == 0 && tag_footer.af == 0) {
            return false;  // 存在没有合并的连续空闲块
        }
        lseek(fd, tag_header.size, SEEK_CUR);
        if (::read(fd, &tag_footer, sizeof(tag_footer)) != sizeof(tag_footer)) {
            return false;  // footer消失了？
        }
        if (tag_header.size != tag_footer.size || tag_header.af != tag_footer.af) {
            return false;  // header和footer数据不一致
        }
    }
}

// 裁剪
int shrink() {
    // always return 0 and do nothing
    return 0;
}