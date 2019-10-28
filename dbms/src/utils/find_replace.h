#pragma once

#include <defs.h>
#include <utils/list.h>


/*
 * FindReplace
 * 提供替换算法接口，这里实现的是栈式LRU算法
 */
class FindReplace {
private:
    List list;

public:
    /*
     * @函数名free
     * @参数index:缓存页面数组中页面的下标
     * 功能:将缓存页面数组中第index个页面的缓存空间回收
     *           下一次通过find函数寻找替换页面时，直接返回index
     */
    void free(int index) { list.move_to_last(index); }
    /*
     * @函数名access
     * @参数index:缓存页面数组中页面的下标
     * 功能:将缓存页面数组中第index个页面标记为访问
     */
    void access(int index) { list.move_to_first(index); }
    /*
     * @函数名find
     * 功能:根据替换算法返回缓存页面数组中要被替换页面的下标
     */
    int find() { return list.replace_last(); }
};
