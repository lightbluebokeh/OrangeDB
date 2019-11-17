#include <cassert>
#include <defs.h>
#include <fcntl.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

// 数据标记
struct _Tag {
    int size : 30;
    int af : 2;
};

// 文件分配器, 线程不安全, 异常也不安全
// 如果发生越界就会破坏链表结构和覆盖之后的数据, 这将不可修复
// 会进行数据填充, 但不会根据块大小优化空间
class FileAllocater {
    // 句柄
    int fd;

    // 数据起始
    static const int DATA_SEEK = 1024;

    // 块对齐大小
    static const int ALIGN_SIZE = 8;

    // 初始化
    void init() {
        lseek(fd, DATA_SEEK, SEEK_SET);
        _Tag tag{0, 1};  // 可视为 header
        write(fd, &tag, sizeof(tag));
    }

public:
    // 保存在数据文件
    FileAllocater(const String& name) {
        if (!fs::exists(name)) {
            fd = open(name.c_str(), O_RDWR | O_BINARY);
            init();
        } else {
            fd = open(name.c_str(), O_RDWR | O_BINARY);
        }
    }

    // 返回偏移量
    size_t alloc(size_t size) {
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
            if (read(fd, tag, sizeof(tag)) < (int)sizeof(tag)) {
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
            write(fd, tag, sizeof(tag));
            lseek(fd, tag[0].size, SEEK_CUR);
            write(fd, tag, sizeof(tag));
        } else {
            int seg_size = tag[0].size;
            tag[0].size = size;
            write(fd, tag, sizeof(tag));
            lseek(fd, size, SEEK_CUR);
            write(fd, tag, sizeof(tag));
            tag[0] = {seg_size - (int)size - 2 * (int)sizeof(tag), 0};
            write(fd, tag, sizeof(tag));
            lseek(fd, tag[0].size, SEEK_CUR);
            write(fd, tag, sizeof(tag));
        }

        return offset;
    }

    // 释放某个块, 如果返回 false 说明文件结构被破坏了并且多半是不可修复的
    bool free(size_t offset) {
        _Tag tag[2];  // tag[0]: 上一个块的footer, tag[1]: 当前块
        lseek(fd, offset - 2 * sizeof(_Tag), SEEK_SET);
        read(fd, tag, 2 * sizeof(_Tag));

        if (tag[1].af == 0) {
            return false;  // double free
        }

        int size = tag[1].size;
        if (tag[0].af == 0) {  // 跟前一块合并
            size += tag[0].size + 2 * sizeof(_Tag);
        }

        lseek(fd, tag[1].size, SEEK_CUR);  // 看看后一块
        if (read(fd, tag, 2 * sizeof(_Tag)) == 2 * sizeof(_Tag) && tag[1].af == 0) {
            // 跟后一块合并
            lseek(fd, -size - 3 * sizeof(_Tag), SEEK_CUR);
            size += tag[1].size + 2 * sizeof(_Tag);
            tag[1].size = size;
            write(fd, &tag[1], sizeof(tag[1]));
            lseek(fd, size, SEEK_CUR);
            write(fd, &tag[1], sizeof(tag[1]));
        } else {  // 文件尾或无法合并
            tag[0].size = size;
            tag[0].af = 0;
            lseek(fd, -(int)sizeof(tag[0]), SEEK_CUR);  // 只读了一个tag所以只回退一个
            write(fd, tag, sizeof(tag[0]));
            lseek(fd, -size - sizeof(_Tag), SEEK_CUR);
            write(fd, tag, sizeof(_Tag));
        }

        return true;
    }

    // 删除末尾未使用的区域, 返回值是删了多少
    // 不会移动中间的空闲区域, 这样会改变指针; 连续的空闲段理论上不会存在
    int shrink() {
        // always return 0
        return 0;
    }
};
