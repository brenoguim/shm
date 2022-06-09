#pragma once

#include <cstdint>
#include <string_view>

namespace shm
{
struct ShmControl;

struct ShmBuffer
{
    explicit ShmBuffer(std::size_t sz);
    explicit ShmBuffer(std::string_view fdstr);

    ShmBuffer(const ShmBuffer&) = delete;
    ShmBuffer& operator=(const ShmBuffer&) = delete;

    ShmBuffer(ShmBuffer&& rhs) { *this = std::move(rhs); }
    ShmBuffer& operator=(ShmBuffer&& rhs)
    {
        std::swap(m_control, rhs.m_control);
        std::swap(m_fd, rhs.m_fd);
        return *this;
    }

    ~ShmBuffer();

    std::string key() const;

    void flush();
    void eof();
    std::size_t bufsize();

    long write(const char* buf, std::size_t len);

    struct Buf { char *start, *end, *cap; };
    Buf getRBuf();
    Buf getWBuf(std::size_t bytesOnCurrBuf);

  private:
    ShmControl* m_control {nullptr};
    int m_fd {-1};
};

struct ShmWriter
{
    ShmWriter(ShmBuffer& b) : m_buf(b) {}
    ~ShmWriter() { flush(); close(); }

    void flush() { m_buf.flush(); }
    void close() { m_buf.eof(); }
  private:
    ShmBuffer& m_buf;
};

}
