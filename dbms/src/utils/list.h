#pragma once

#include <defs.h>

class List {
private:
    struct ListNode {
        int prev;
        int next;
    };

    ListNode a[BUF_CAP + 1];
    static constexpr int head = BUF_CAP;

    void del(int index) {
        if (a[index].next != index) {
            auto [prev, next] = a[index];
            a[prev].next = next;
            a[next].prev = prev;
        }
    }

public:
    List() : a() {
        for (int i = 0; i < BUF_CAP + 1; ++i) {
            a[i] = {i + 1, i - 1};
        }
        a[0].next = head;
        a[head].prev = 0;
    }

    void move_to_first(int index) {
        del(index);
        int next = a[head].next;
        a[index] = {head, next};
        a[head].next = a[next].prev = index;
    }

    void move_to_last(int index) {
        del(index);
        int prev = a[head].prev;
        a[index] = {prev, head};
        a[prev].next = a[head].prev = index;
    }

    // 把最后一个移到第一个，返回该元素
    int replace_last() {
        int index = a[head].prev;
        int prev = a[index].prev;
        int next = a[head].next;
        a[prev].next = head;
        a[head] = {prev, index};
        a[index] = {head, next};
        a[next].prev = index;
        return index;
    }
};
