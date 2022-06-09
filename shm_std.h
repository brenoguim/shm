#pragma once

#include "shm.h"

#include <streambuf>
#include <cassert>

#include <iostream>

namespace shm
{

struct ShmStreamBuf : public std::streambuf
{
    using std::streambuf::char_type;
    using std::streambuf::traits_type;
    using std::streambuf::int_type;
    explicit ShmStreamBuf(shm::ShmBuffer& sb) : m_sb(sb)
    {
        //setp(nullptr, nullptr);
        //setg(nullptr, nullptr, nullptr);
    }

    int overflow(int c) override
    {
        auto [start, end, cap] = m_sb.getWBuf(pptr() - pbase());
        setp(start, cap);
        if (c != EOF)
        {
            char rc = c;
            xsputn(&rc, 1);
        }
        return traits_type::to_int_type(c);
    }

    int underflow() override
    {
        auto [start, end, cap] = m_sb.getRBuf();
        if (start != end)
        {
            setg(start, end, cap);
            return traits_type::to_int_type(*start);
        }
        return EOF;
    }

    int sync() override
    {
        auto [start, end, cap] = m_sb.getWBuf(pptr() - pbase());
        setp(start, cap);
        return 0;
    }

  private:
    shm::ShmBuffer& m_sb;
};

}
