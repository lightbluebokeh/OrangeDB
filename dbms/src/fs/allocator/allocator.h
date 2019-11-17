
#include <defs.h>

// 文件分配器, 线程不安全, 异常也不安全
// 如果发生越界就会破坏链表结构和覆盖之后的数据, 这将不可修复
// 会进行数据填充, 但不会根据块大小优化空间
class FileAllocator {
    // 句柄
    int fd;

    // 数据起始
    static const int DATA_SEEK = 1024;

    // 块对齐大小
    static const int ALIGN_SIZE = 8;

    // 初始化
    void init();

public:
    // 构造函数传入文件名
    FileAllocator(const String& name);
    // 随便移动和复制, 反正不要多线程和在异常中使用就行

    // 返回偏移量, 如果有数据还能顺便写进去
    size_t alloc(size_t size, void* data = nullptr);

    // 读取某个地方的值, 返回具体读了多少. 写这个函数是因为fd只存在这个对象里, 否则外面需要open一次
    int read(size_t offset, void* dst, int max_count);

    // 和上面差不多
    int write(size_t offset, void* data, int max_count);

    // 释放某个块, 如果返回 false 说明文件结构被破坏了并且多半是不可修复的
    bool free(size_t offset);

    // 检查这个数据文件是否正常
    bool check();

    // 删除末尾未使用的区域, 返回值是删了多少
    // 不会移动中间的空闲区域, 这样会改变指针; 连续的空闲段理论上不会存在
    int shrink();
};
