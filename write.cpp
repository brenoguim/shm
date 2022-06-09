#include "shm.h"
#include "shm_std.h"

#include <string>
#include <cstring>

#include <iostream>
#include <tuple>

static constexpr long _kb = 1UL*1024;
static constexpr long _mb = 1024 * _kb;
static constexpr long _gb = 1024 * _mb;

static void throwUsage() {
    const char* usage = "\nUsage: write [--block-size <N=16>] [--gbs <N=1>] [--repeat <N=1>] [--manual] [--iostream] [--disable-read-memcpy]\n"
                        "    --block-size                 Configure the amount of bytes provided by the application at a time.\n"
                        "                                   This is usually small since applications usually serialize numbers and small strings (names)\n"
                        "    --gbs                        The total amount of memory to be transfered. Can be increased for better precision\n"
                        "    --repeat                     Number of times to run each experiment\n"
                        "    --iostream                   Include experiment using iostream\n"
                        "    --manual                     Include experiment using manual API calls\n"
                        "    --disable-read-memcpy        Disable the \"memcpy\" on the read side, designed to emulate some processing done by the reader.\n"
                        "                                   This is useful to increase the throughput on the reader side to make sure it's not the bottleneck\n"
                        "\n"
                        ;
    throw std::runtime_error(usage);
}

void run(int gbs, int block_size, bool disable_read_memcpy, bool manual)
{
    shm::ShmBuffer shmbuf(256*_kb);
    auto procStr = "./read " + shmbuf.key() + " " + std::to_string(disable_read_memcpy ? 1 : 0);

    FILE* r = popen(procStr.c_str(), "w");
    {
        shm::ShmWriter sw(shmbuf);

        // Chunks of data to be pushed at a time. Filled with x\nx\nx\n...
        char* buf = new char[block_size];
        for (int i = 0; i < block_size; ++i) buf[i] = (i%2 == 0) ? 'x' : '\n';

        if (!manual)
        {
            std::cout << "[write] Using iostreams" << std::endl;
            shm::ShmStreamBuf ssb(shmbuf);
            std::ostream os(&ssb);
            for (long i = 0; i < gbs*_gb; i+=block_size)
                os << buf;
            os.flush();
        }
        else
        {
            std::cout << "[write] Using manual writing" << std::endl;
            auto [start, end, cap] = shmbuf.getWBuf(0);

            long written = 0;
            while (written < gbs*_gb)
            {
                size_t sent = 0;
                while (sent != block_size)
                {
                    size_t amount_to_copy = std::min(size_t(block_size - sent), size_t(cap-end));
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
        }
    }
    pclose(r);
    std::cout << std::endl;
}

int main(int argc, char** argv)
{
    int gbs = 1;
    int block_size = 16;
    int repeat = 1;
    bool manual = false;
    bool iostream = false;
    bool disable_read_memcpy = false;
    for (int i = 1; i < argc; ++i)
    {
        auto arg = std::string(argv[i]);

        auto parseInt = [&] (const char* sw, int& out) {
            if (arg != sw) return false;
            if (++i >= argc) throwUsage();
            out = atoi(argv[i]);
            return true;
        };

        if (parseInt("--block-size", block_size));
        else if (parseInt("--gbs", gbs));
        else if (parseInt("--repeat", repeat));
        else if (arg == "--iostream") iostream = true;
        else if (arg == "--manual") manual = true;
        else if (arg == "--disable-read-memcpy") disable_read_memcpy = true;
        else
            throwUsage();
    }
    if (!manual && !iostream) throwUsage();

    for (int rr = 0; rr < repeat; ++rr)
    {
        if (manual)
            run(gbs, block_size, disable_read_memcpy, true);

        if (iostream)
            run(gbs, block_size, disable_read_memcpy, false);
    }
}
