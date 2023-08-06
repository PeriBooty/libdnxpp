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
#ifndef LIBDIANNEX_BINARYREADER_HPP
#define LIBDIANNEX_BINARYREADER_HPP

#include "common.hpp"
#include <fstream>

namespace diannex
{
    template<class Type>
    struct make_shared_enabled : public Type
    {
        template<typename... Args>
        explicit make_shared_enabled(Args&& ... args)
            : Type(std::forward<Args>(args)...)
        {}
    };

    class BinaryReader
    {
    public:
        using Ptr = std::shared_ptr<BinaryReader>;

        virtual ~BinaryReader() = default;

        template<std::default_initializable T>
        auto read() -> T
        {
            if constexpr (std::is_same_v<T, DxStr>)
            {
                DxStrBuilder ss;
                for (char c = read<char>(); c != '\0'; c = read<char>())
                    ss << c;
                return ss.str();
            }
            else
            {
                T value;
                read_n(sizeof(T), &value);
                return value;
            }
        }

        DxByteBuf read_block();

        virtual void skip(size_t count) = 0;

        virtual void read_n(size_t count, void* val) = 0;
    };

    class BinaryFileReader : public BinaryReader
    {
        std::ifstream m_stream;
    public:
        void skip(size_t count) override;

        void read_n(size_t count, void* val) override;

        static BinaryReader::Ptr create(std::ifstream&& stream)
        {
            return std::make_shared<make_shared_enabled<BinaryFileReader>>(std::move(stream));
        }

    protected:
        explicit BinaryFileReader(std::ifstream&& stream);
    };

    class BinarySpanReader : public BinaryReader
    {
        std::span<const std::byte> m_span;

    public:
        void skip(size_t count) override;

        void read_n(size_t count, void* val) override;

        template<class T>
        static BinaryReader::Ptr create(const std::span<T>& span)
        {
            return std::make_shared<make_shared_enabled<BinarySpanReader>>(span);
        }

        static BinaryReader::Ptr create(const std::vector<std::byte>& vec)
        {
            return std::make_shared<make_shared_enabled<BinarySpanReader>>(vec);
        }

    protected:
        template<class T>
        explicit BinarySpanReader(const std::span<T>& span)
            : m_span(std::as_bytes(span))
        {}

        explicit BinarySpanReader(const std::vector<std::byte>& vec)
            : m_span(vec)
        {}
    };
}

#endif //LIBDIANNEX_BINARYREADER_HPP
