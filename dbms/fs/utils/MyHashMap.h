#ifndef MY_HASH_MAP
#define MY_HASH_MAP
#include "pagedef.h"
#include <unordered_map>

/*
 * 两个键的hash表
 * hash表的value是自然数，在缓存管理器中，hash表的value用来表示缓存页面数组的下标
 */
// your hash map sucks

namespace std {
template <>
struct hash<pair<int, int>> {
    size_t operator()(pair<int, int> x) const { return x.first ^ x.second; }
};
}  // namespace std

class MyHashMap {
private:
    using DataNode = std::pair<int, int>;
    DataNode map_inv[CAP];
    // std::pair<int, int> *a;
    std::unordered_map<std::pair<int, int>, int> map;


    // /*
    //  * hash函数
    //  */
    // int hash(int k1, int k2) {
    //     return (k1 * A + k2 * B) % MOD_;
    // }
public:
    /*
     * @函数名findIndex
     * @参数k1:第一个键
     * @参数k2:第二个键
     * 返回:根据k1和k2，找到hash表中对应的value
     *           这里的value是自然数，如果没有找到，则返回-1
     */
    int findIndex(int k1, int k2) {
        if (map.count({k1, k2})) return map[{k1, k2}];
        return -1;
    }
    /*
     * @函数名replace
     * @参数index:指定的value
     * @参数k1:指定的第一个key
     * @参数k2:指定的第二个key
     * 功能:在hash表中，将指定value对应的两个key设置为k1和k2
     */
    void replace(int index, int k1, int k2) {
        // int h = hash(k1, k2);
        // //cout << h << endl;
        // list->insertFirst(h, index);
        map[{k1, k2}] = index;
        map_inv[index] = {k1, k2};
        // a[index].file_id = k1;
        // a[index].page_id = k2;
    }
    /*
     * @函数名remove
     * @参数index:指定的value
     * 功能:在hash表中，将指定的value删掉
     */
    void remove(int index) {
        // list->del(index);
        // map.erase({a[index].file_id, a[index].page_id});
        map.erase(map_inv[index]);
        map_inv[index] = {-1, -1};
        // a[index].file_id = -1;
        // a[index].page_id = -1;
    }
    /*
     * @函数名getKeys
     * @参数index:指定的value
     * @参数k1:存储指定value对应的第一个key
     * @参数k2:存储指定value对应的第二个key
     */
    void getKeys(int index, int& k1, int& k2) {
        // k1 = a[index].file_id;
        // k2 = a[index].page_id;
        k1 = map_inv[index].first;
        k2 = map_inv[index].second;
    }
    /*
     * 构造函数
     * @参数c:hash表的容量上限
     * @参数m:hash函数的mod
     */
    MyHashMap() {
        // a = new DataNode[c];
        for (int i = 0; i < CAP; ++i) {
            // a[i].file_id = -1;
            // a[i].page_id = -1;
            map_inv[i] = {-1, -1};
        }
    }
};
#endif
