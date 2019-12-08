#include "allocator.h"

#include <cassert>
#include <cstdio>

// 数据标记
struct _Tag {
    int size : 30;
    int af : 2;
};

// 初始化
void FileAllocator::init() const {
    fseek(fd, DATA_OFFSET, SEEK_SET);
    _Tag tag{0, 1};
    fwrite(&tag, 1, sizeof(tag), fd);
}

// 构造函数
FileAllocator::FileAllocator(const String& name) {
    if (!fs::exists(name)) {
        fd = fopen(name.c_str(), "wb+");
        init();
    } else {
        fd = fopen(name.c_str(), "rb+");
    }
}

// 析构
FileAllocator::~FileAllocator() {
    fclose(fd);
}

// 分配空间
size_t FileAllocator::allocate(size_t size, void* data) const {
    assert((int)size > 0);
    fseek(fd, DATA_OFFSET + sizeof(_Tag), SEEK_SET);
    _Tag tag[1];

    // 对齐
    const size_t data_size = size;
    if (size_t m = size & (ALIGN_SIZE - 1U)) {
        size ^= m;
        size += ALIGN_SIZE;
    }
    // 找到一块空闲的地方, 没用优化分配, 暴力找
    while (true) {
        if (fread(tag, 1, sizeof(tag), fd) < (int)sizeof(tag)) {
            tag[0].size = size;  // 找到文件结尾了
            break;
        }
        if ((size_t)tag[0].size >= size && tag[0].af == 0) {
            fseek(fd, -(int)sizeof(_Tag), SEEK_CUR);
            break;
        }
        fseek(fd, tag[0].size + sizeof(tag), SEEK_CUR);
    }
    unsigned long long offset = ftell(fd) + sizeof(tag);
    fseek(fd, 0, SEEK_CUR);

    tag[0].af = 1;
    if ((size_t)tag[0].size < size + 2 * sizeof(tag) + ALIGN_SIZE) {
        fwrite(tag, 1, sizeof(tag), fd);
        if (data) {
            int count = fwrite(data, 1, data_size, fd);
            fseek(fd, tag[0].size - count, SEEK_CUR);
        } else {
            fseek(fd, tag[0].size, SEEK_CUR);
        }
        fwrite(tag, 1, sizeof(tag), fd);
    } else {
        int seg_size = tag[0].size;
        tag[0].size = size;
        fwrite(tag, 1, sizeof(tag), fd);
        if (data) {
            int count = fwrite(data, 1, data_size, fd);
            fseek(fd, size - count, SEEK_CUR);
        } else {
            fseek(fd, size, SEEK_CUR);
        }
        fwrite(tag, 1, sizeof(tag), fd);
        tag[0] = {seg_size - (int)size - 2 * (int)sizeof(tag), 0};
        fwrite(tag, 1, sizeof(tag), fd);
        fseek(fd, tag[0].size, SEEK_CUR);
        fwrite(tag, 1, sizeof(tag), fd);
    }

    return offset;
}

// 读取
int FileAllocator::read(size_t offset, void* dst, size_t max_size) const {
    _Tag tag;
    fseek(fd, offset - sizeof(tag), SEEK_SET);
    fread(&tag, 1, sizeof(tag), fd);

    if (tag.af != 1) {
        return -1;  // offset是假的?
    }

    if ((size_t)tag.size < max_size) max_size = tag.size;
    return fread(dst, 1, max_size, fd);
}

std::pair<bytes_t, size_t> FileAllocator::read_block(size_t offset) const {
    _Tag tag;
    fseek(fd, offset - sizeof(tag), SEEK_SET);
    fread(&tag, 1, sizeof(tag), fd);

    if (tag.af != 1) {
        return std::make_pair(nullptr, 0);  // offset是假的!
    }
    auto size = tag.size;
    auto ret = new byte_t[size];

    auto read_size = fread(ret, 1, size, fd);
    orange_assert(int(read_size) == size, "IO error");
    return std::make_pair(ret, size);
}

// 写入
int FileAllocator::write(size_t offset, void* data, size_t max_size) const {
    _Tag tag;
    fseek(fd, offset - sizeof(tag), SEEK_SET);
    fread(&tag, sizeof(tag), 1, fd);

    if (tag.af != 1) {
        return -1;
    }

    fseek(fd, 0, SEEK_CUR);
    if ((size_t)tag.size < max_size) max_size = tag.size;
    return fwrite(data, 1, max_size, fd);
}

// 释放

bool FileAllocator::free(size_t offset) const {
    _Tag tag[2];  // tag[0]: 上一个块的footer, tag[1]: 当前块
    fseek(fd, offset - 2 * sizeof(_Tag), SEEK_SET);
    fread(tag, 1, 2 * sizeof(_Tag), fd);

    if (tag[1].af == 0) {
        return false;  // double free
    }

    int size = tag[1].size;
    if (tag[0].af == 0) {  // 跟前一块合并
        size += tag[0].size + 2 * sizeof(_Tag);
    }

    fseek(fd, tag[1].size, SEEK_CUR);  // 看看后一块
    if (fread(tag, sizeof(_Tag), 2, fd) == 2) {
        if (tag[1].af == 0) {
            // 跟后一块合并
            fseek(fd, -size - 3 * sizeof(_Tag), SEEK_CUR);
            size += tag[1].size + 2 * sizeof(_Tag);
            tag[1].size = size;

            fflush(fd);
            fwrite(&tag[1], 1, sizeof(_Tag), fd);
            fseek(fd, size, SEEK_CUR);
            fwrite(&tag[1], 1, sizeof(_Tag), fd);
        } else {
            tag[0].size = size;
            tag[0].af = 0;
            fseek(fd, -2 * (int)sizeof(_Tag), SEEK_CUR);
            fwrite(&tag[0], sizeof(_Tag), 1, fd);
            fseek(fd, -size - 2 * sizeof(_Tag), SEEK_CUR);
            fwrite(&tag[0], sizeof(_Tag), 1, fd);
        }
    } else {  // 文件尾或无法合并
        tag[0].size = size;
        tag[0].af = 0;
        fseek(fd, -(int)sizeof(_Tag), SEEK_CUR);  // 只读了一个tag所以只回退一个
        fwrite(&tag[0], sizeof(_Tag), 1, fd);
        fseek(fd, -size - 2 * sizeof(_Tag), SEEK_CUR);
        fwrite(&tag[0], sizeof(_Tag), 1, fd);
    }

    return true;
}

// 刷新
int FileAllocator::flush() const {
    return fflush(fd);
}

// 检查
bool FileAllocator::check() const {
    fseek(fd, DATA_OFFSET, SEEK_SET);
    _Tag tag_header, tag_footer;

    // 检查头部
    fread(&tag_footer, 1, sizeof(tag_footer), fd);
    if (tag_footer.af != 1) return false;  // 头部af错误

    while (true) {
        if (int c = fread(&tag_header, 1, sizeof(tag_header), fd); c < (int)sizeof(tag_header)) {
            return c == 0;  // header不完整
        }
        if (tag_header.af == 0 && tag_footer.af == 0) {
            return false;  // 存在没有合并的连续空闲块
        }
        fseek(fd, tag_header.size, SEEK_CUR);
        if (fread(&tag_footer, 1, sizeof(tag_footer), fd) != sizeof(tag_footer)) {
            return false;  // footer消失了？
        }
        if (tag_header.size != tag_footer.size || tag_header.af != tag_footer.af) {
            return false;  // header和footer数据不一致
        }
    }
}

// 打印
void FileAllocator::print() const {
    fseek(fd, DATA_OFFSET, SEEK_SET);
    _Tag tag_header, tag_footer;
    long offset;

    // 检查头部
    offset = ftell(fd);
    fread(&tag_footer, 1, sizeof(tag_footer), fd);
    printf("[0x%08lx]  HEAD ", offset);
    if (tag_footer.af != 1) {
        printf("<error>");
        return;
    }
    printf("\n");

    while (true) {
        offset = ftell(fd);
        if (int c = fread(&tag_header, 1, sizeof(tag_header), fd); c < (int)sizeof(tag_header)) {
            if (c != 0) {
                printf("*** file broken: missing header ***\n");
            }
            return;
        }
        if (tag_header.af == 0 && tag_footer.af == 0) {
            printf("<not merged>\n");
        }
        printf("[0x%08lx]  allocated: %d, data length: %d\n", offset, tag_header.af,
               tag_header.size);
        fseek(fd, tag_header.size, SEEK_CUR);
        if (fread(&tag_footer, 1, sizeof(tag_footer), fd) != sizeof(tag_footer)) {
            printf("*** file broken: missing footer ***\n");
            return;
        }
        if (tag_header.size != tag_footer.size || tag_header.af != tag_footer.af) {
            printf("*** file broken: header and footer are not matched ***\n");
            return;
        }
    }
}

// 裁剪
int shrink() {
    // always return 0 and do nothing
    return 0;
}