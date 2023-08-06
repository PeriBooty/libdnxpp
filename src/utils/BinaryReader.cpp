/*======================================================================================================================
 * Copyright © 2023 PeriBooty and Contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the “Software”), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *====================================================================================================================*/
#include "utils/BinaryReader.hpp"

namespace diannex
{
    DxByteBuf BinaryReader::read_block()
    {
        auto size = this->read<uint32_t>();
        DxByteBuf buff(size);
        read_n(size, buff.data());
        return std::move(buff);
    }

    BinaryFileReader::BinaryFileReader(std::ifstream&& stream)
        : m_stream(std::move(stream))
    {}

    void BinaryFileReader::skip(size_t count)
    {
        m_stream.seekg((std::ifstream::off_type)count, std::ios::cur);
    }

    void BinaryFileReader::read_n(size_t count, void* val)
    {
        if (!val)
            throw std::invalid_argument("destination pointer was null!");

        m_stream.read((char*)val, (std::streamsize)count);
    }

    void BinarySpanReader::skip(size_t count)
    {
        m_span = m_span.subspan(count);
    }

    void BinarySpanReader::read_n(size_t count, void* val)
    {
        auto data = m_span.first(count);
        auto out = (std::byte*)val;
        for (size_t i = 0; i < count; ++i, ++out)
            *out = data[i];
        skip(count);
    }
}