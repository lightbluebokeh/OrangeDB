#pragma once

#include <defs.h>
#include <utils/YourLinkList.h>

// template <int CAP_>
/*
 * FindReplace
 * 提供替换算法接口，这里实现的是栈式LRU算法
 */
class FindReplace {
private:
    YourLinkList* list;
    // int CAP_;

public:
    /*
     * @函数名free
     * @参数index:缓存页面数组中页面的下标
     * 功能:将缓存页面数组中第index个页面的缓存空间回收
     *           下一次通过find函数寻找替换页面时，直接返回index
     */
    void free(int index) { list->insertFirst(0, index); }
    /*
     * @函数名access
     * @参数index:缓存页面数组中页面的下标
     * 功能:将缓存页面数组中第index个页面标记为访问
     */
    void access(int index) { list->insert(0, index); }
    /*
     * @函数名find
     * 功能:根据替换算法返回缓存页面数组中要被替换页面的下标
     */
    int find() {
        int index = list->getFirst(0);
        list->del(index);
        list->insert(0, index);
        return index;
    }
    /*
     * 构造函数
     * @参数c:表示缓存页面的容量上限
     */
    FindReplace() {
        // CAP_ = c;
        list = new YourLinkList(BUF_CAP, 1);
        for (int i = 0; i < BUF_CAP; ++i) {
            list->insert(0, i);
        }
    }
};