
#include <defs.h>

// 文件分配器, 线程不安全, 异常也不安全
// 如果发生越界就会破坏链表结构和覆盖之后的数据, 这将不可修复, 因此尽量用接口来读写
// 只支持一次性读完一个结点中的数据
// 会进行数据填充, 按8字节对齐, 也就是实际分配的数量是8的倍数
// 分配完全是暴力的从前往后找, 不会根据块大小优化空间 (要用可加)
class FileAllocator {
    // 文件句柄
    FILE* fd;

    // 数据起始偏移量, 前面是预留空间暂时还没使用
    static const int DATA_OFFSET = 1024;

    // 块对齐大小
    static const int ALIGN_SIZE = 8;

    // 初始化
    void init();

public:
    // 构造函数传入文件名
    explicit FileAllocator(const String& name);
    // 随便移动和复制, 反正不要在多线程和异常中使用就行

    // 析构
    ~FileAllocator();

    // 返回偏移量, 如果有数据还能顺便写进去
    size_t allocate(size_t size, void* data = nullptr);

    // 读取某个地方的值, max_size单位是字节, 返回具体读了多少个字节.
    // 写这个函数是因为fd只存在这个对象里, 否则外面需要fopen数据文件
    int read(size_t offset, void* dst, size_t max_size = -1);

    // 和上面差不多
    int write(size_t offset, void* data, size_t max_size = -1);

    // 释放某个块, 如果返回false说明文件结构被破坏了
    bool free(size_t offset);

    // 刷新文件, 成功时返回0
    int flush();

    // 检查数据文件的结构, false说明被破坏了
    bool check() const;

    // 在check的基础上打印链表结构, 用来玩的
    void print() const;

    // 删除末尾未使用的区域, 返回值是删了多少
    // 不会移动中间的空闲区域, 这样会改变指针; 连续的空闲段理论上不会存在
    int shrink();
};
