#pragma once

#include <defs.h>
#include <cassert>

class IdPool {
    static constexpr int CAP = MAX_FILE_NUM;

    // seems a queue will be okay?
    // your bitmap sucks
    int a[CAP], he, ta; 
    void push(int id) {
        assert(size() <= CAP);
        if (ta == CAP) ta = 0;
        a[ta++] = id;
    }

    int front() {
        assert(!empty());
        return a[he];
    }

    int pop() {
        assert(!empty());
        int ret = a[he++];
        if (he == CAP) he = 0;
        return ret;
    }
public:
    bool empty() { return he == ta; }

    size_t size() {
        if (he <= ta) return ta - he;
        return ta + CAP - he;
    }

    int add(int id) { this->push(id); }
    int get() { return this->pop(); }

    IdPool() {
        he = 0, ta = CAP;
        for (int i = 0; i < CAP; i++) a[i] = i;
    }
};