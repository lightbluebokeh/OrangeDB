#pragma once

#include <bytes/bytes_stream.h>
#include <fs/bufmanager/BufPageManager.h>
#include <fs/bufmanager/buf_page.h>

class BufPageStream : public BytesStream {
private:
    BufPage page;

    BufPageStream(const BufPageStream&) = delete;
    BufPageStream& operator = (const BufPageStream&) = delete;
protected:
    void before_IO(size_t n) override {
        BytesStream::before_IO(n);
        page.ensure_buf();
    }
    void after_O() override {
        BytesStream::after_O();
        auto bfm = BufPageManager::get_instance();
        bfm->mark_dirty(page.buf.buf_id);
    }
public:
    BufPageStream(BufPageStream&& bps) : BufPageStream(bps.page) {}
    explicit BufPageStream(const BufPage& page) : page(page), BytesStream(this->page.buf.bytes, PAGE_SIZE) {}
    BufPageStream& operator = (BufPageStream&& bps) {
        page = bps.page;
        return (BufPageStream&)BytesStream::operator=(std::move(bps));
    }
};