#include "shm.h"

#include <chrono>
#include <cstring>
#include <iostream>

int main(int argc, char** argv)
{
    constexpr long _kb = 1UL*1024;
    constexpr long _mb = 1024 * _kb;
    constexpr long _gb = 1024 * _mb;

    if (argc != 2)
    {
        std::cout << "Wrong number of args" << std::endl;
        return 0;
    }

    auto sb = shm::ShmBuffer(argv[1]);

    std::cout << "Start reading" << std::endl;
    auto start = std::chrono::steady_clock::now();
    long br = 0;

    auto buf = new char[sb.bufsize()];

    while (1)
    {
        auto [start, end, cap] = sb.getRBuf();
        if (!start)
            break; // eof
        std::memcpy(buf, start, end - start); // do something with the buffer. In this case, copy it to emulate some processing
        br += (end - start);
    }

    auto end = std::chrono::steady_clock::now();
    auto elapsed = double(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count())/1000;
    std::cout << "Read " << double(br)/1024/1024/1024 << " GB in " << elapsed << " seconds | " << double(br)/1024/1024/1024/elapsed << " GB/s" << std::endl;
    std::cout << "    Exactly: " << br << " bytes" << std::endl;
} 
