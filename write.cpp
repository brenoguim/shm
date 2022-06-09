#include "shm.h"
#include "shm_std.h"

#include <string>
#include <cstring>

#include <iostream>
#include <tuple>

int main()
{
    constexpr long _kb = 1UL*1024;
    constexpr long _mb = 1024 * _kb;
    constexpr long _gb = 1024 * _mb;

    shm::ShmBuffer shmbuf(256*_kb);

    FILE* r = popen(("./read " + shmbuf.key()).c_str(), "w");
    {
        shm::ShmWriter sw(shmbuf);
#if 0
        size_t buf_sz = 4;
        char* buf = new char[buf_sz];
        for (int i = 0; i < buf_sz; ++i) buf[i] = (i%2 == 0) ? 'x' : '\n';

        long written = 0;
        while (written < 2*_gb)
        {
            size_t sent = 0;
            while (sent != buf_sz)
                sent += shmbuf.write(buf + sent, buf_sz - sent);

            written += sent;
        }
#elif 1
        size_t buf_sz = 16;
        char* buf = new char[buf_sz];
        for (int i = 0; i < buf_sz; ++i) buf[i] = (i%2 == 0) ? 'x' : '\n';

        auto [start, end, cap] = shmbuf.getWBuf(0);

        long written = 0;
        while (written < 2*_gb)
        {
            size_t sent = 0;
            while (sent != buf_sz)
            {
                size_t amount_to_copy = std::min(size_t(buf_sz - sent), size_t(cap-end));
                std::memcpy(end, buf+sent, amount_to_copy);
                sent += amount_to_copy;
                end += amount_to_copy;
                if (end == cap) {
                    auto [nstart,nend,ncap] = shmbuf.getWBuf(end-start);
                    start = nstart;
                    end = nend;
                    cap = ncap;
                }
            }
            written += sent;
        }
        shmbuf.getWBuf(end-start); // flush
#elif 0
        size_t buf_sz = shmbuf.bufsize();
        char* buf = new char[buf_sz];
        for (int i = 0; i < buf_sz; ++i) buf[i] = (i%2 == 0) ? 'x' : '\n';

        shm::ShmStreamBuf ssb(shmbuf);
        std::ostream os(&ssb);
        for (long i = 0; i < 50*_gb; i+=buf_sz)
            os << buf;
        os.flush();
#elif 1
        size_t buf_sz = 16;
        char* buf = new char[buf_sz];
        for (int i = 0; i < buf_sz; ++i) buf[i] = (i%2 == 0) ? 'x' : '\n';

        shm::ShmStreamBuf ssb(shmbuf);
        std::ostream os(&ssb);
        os.imbue(std::locale("C"));
        for (long i = 0; i < 2*_gb/buf_sz; ++i)
            os << buf;
        os.flush();
#endif
    }
    pclose(r);
}
