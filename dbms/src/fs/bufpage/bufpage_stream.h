#pragma once

#include <utils/bytes_stream.h>
#include <fs/bufpage/bufpage.h>
#include <fs/bufpage/bufpage_manager.h>

class BufpageStream : public BytesStream {
private:
    Bufpage page;

protected:
    void before_IO(size_t n) override {
        BytesStream::before_IO(n);
        page.ensure_buf();
    }
    void after_O() override {
        BytesStream::after_O();
        auto bfm = BufpageManager::get_instance();
        bfm->mark_dirty(page.buf.buf_id);
    }

public:
    BufpageStream(BufpageStream&& bps) : BufpageStream(bps.page) {}
    BufpageStream(const BufpageStream&) = delete;
    explicit BufpageStream(const Bufpage& page) :
        BytesStream(this->page.buf.bytes, PAGE_SIZE), page(page) {}

    BufpageStream& operator=(const BufpageStream&) = delete;
    BufpageStream& operator=(BufpageStream&& bps) {
        page = bps.page;
        return (BufpageStream&)BytesStream::operator=(std::move(bps));
    }
};