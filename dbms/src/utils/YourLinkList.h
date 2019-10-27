#pragma once

// template <int LIST_NUM, int cap>
class YourLinkList {
private:
    struct ListNode {
        int next;
        int prev;
    };
    int cap;
    int LIST_NUM;
    ListNode* a;
    void link(int prev, int next) {
        a[prev].next = next;
        a[next].prev = prev;
    }

public:
    void del(int index) {
        if (a[index].prev == index) {
            return;
        }
        link(a[index].prev, a[index].next);
        a[index].prev = index;
        a[index].next = index;
    }
    void insert(int listID, int ele) {
        del(ele);
        int node = listID + cap;
        int prev = a[node].prev;
        link(prev, ele);
        link(ele, node);
    }
    void insertFirst(int listID, int ele) {
        del(ele);
        int node = listID + cap;
        int next = a[node].next;
        link(node, ele);
        link(ele, next);
    }
    int getFirst(int listID) { return a[listID + cap].next; }
    int next(int index) { return a[index].next; }
    bool isHead(int index) { return index >= cap; }
    bool isAlone(int index) { return (a[index].next == index); }
    YourLinkList(int c, int n) {
        cap = c;
        LIST_NUM = n;
        a = new ListNode[n + c];
        for (int i = 0; i < cap + LIST_NUM; ++i) {
            a[i].next = i;
            a[i].prev = i;
        }
    }
};
