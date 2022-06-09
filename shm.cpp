#include "shm.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <iostream>
#include <new>
#include <stdexcept>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace shm
{

struct ShmControl
{
    static constexpr int NBufs = 4;
    struct Valid { std::atomic<std::size_t> v{0}; };

    ShmControl(std::size_t s) : m_bufsz(s) {}

    std::tuple<char*, char*, char*> getRBuf()
    {
        if (m_ridx != -1)
        {
            m_dataValid[m_ridx].v = 0;
            m_ridx = (m_ridx + 1)%NBufs;
        }
        else
            m_ridx = 0;

        auto& rvalid = m_dataValid[m_ridx].v;

        while (1)
        {
            if (auto len = rvalid.load())
            {
                auto d = data(m_ridx);
                return {d, d+len, d+bufsize()};
            }
            if (eof)
                return {nullptr, nullptr, nullptr};
        }
    }

    std::tuple<char*, char*> getWBuf(std::size_t numBytesWrittenOnCurrentBuf)
    {
        if (numBytesWrittenOnCurrentBuf)
        {
            m_dataValid[m_widx].v = numBytesWrittenOnCurrentBuf;
            m_widx = (m_widx + 1)%NBufs;
        }

        auto& wvalid = m_dataValid[m_widx].v;
        while (wvalid != 0);

        auto d = data(m_widx);
        return {d, d+bufsize()};
    }

    void flush()
    {
        for (int i = 0; i < NBufs; ++i) while (m_dataValid[i].v);
    }

    char* begin() { return reinterpret_cast<char*>(this+1); } 
    char* data(size_t i) { return begin() + i*(m_bufsz/NBufs); }
    size_t bufsize() const { return m_bufsz/NBufs; }

    std::size_t m_bufsz;
    Valid m_dataValid[NBufs];
    std::atomic<bool> eof {false};

    int m_ridx {-1};
    int m_widx {0};
};

ShmBuffer::ShmBuffer(std::size_t sz)
{
    auto filename = std::string("/shm_") + std::to_string(getpid()) + std::to_string(gettid());
    ::shm_unlink(filename.c_str());
    m_fd = ::shm_open(filename.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    ::shm_unlink(filename.c_str());

    // Keep file descriptor open after exec so that child processes can mmap it
    fcntl(m_fd, F_SETFD, fcntl(m_fd, F_GETFD) & ~FD_CLOEXEC); 

    auto dataSize = sz;
    auto bufSize = sz + sizeof(ShmControl);

    ftruncate(m_fd, bufSize);
    void* addr = ::mmap(nullptr, bufSize, PROT_READ|PROT_WRITE, MAP_SHARED, m_fd, 0);

    m_control = new (addr) ShmControl(dataSize);
}

ShmBuffer::ShmBuffer(std::string_view fdstr) : m_fd(atoi(fdstr.begin()))
{
    struct stat st;
    fstat(m_fd, &st);
    auto sz = st.st_size;
    void* addr = ::mmap(nullptr, sz, PROT_READ|PROT_WRITE, MAP_SHARED, m_fd, 0);
    m_control = reinterpret_cast<ShmControl*>(addr);
}

ShmBuffer::~ShmBuffer()
{
    if (m_control)
    {
        auto sz = m_control->m_bufsz + sizeof(ShmControl);
        m_control->~ShmControl();
        ::munmap(m_control, sz);
        close(m_fd);
    }
}

std::string ShmBuffer::key() const { return std::to_string(m_fd); }
void ShmBuffer::flush() { m_control->flush(); }
void ShmBuffer::eof() { m_control->eof = true; }
std::size_t ShmBuffer::bufsize() { return m_control->bufsize(); }

long ShmBuffer::write(const char* buf, std::size_t len)
{
    auto [start, end, cap] = getWBuf(0);
    auto can_send = std::min(size_t(cap-start), len);
    std::memcpy(start, buf, can_send);
    getWBuf(can_send); // just to flush
    return can_send;
}

ShmBuffer::Buf ShmBuffer::getRBuf()
{
    auto [start, end, cap] = m_control->getRBuf();
    return {start, end, cap};
}

ShmBuffer::Buf ShmBuffer::getWBuf(std::size_t numBytesWrittenOnCurrentBuf)
{
    auto [start, cap] = m_control->getWBuf(numBytesWrittenOnCurrentBuf);
    return {start, start, cap};
}

}
