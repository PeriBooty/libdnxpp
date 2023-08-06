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
#ifndef LIBDIANNEX_EXCEPTIONS_HPP
#define LIBDIANNEX_EXCEPTIONS_HPP

#include <exception>

#ifndef USE_FMTLIB
#include <format>
#else
#include <fmt/core.h>
namespace std { using namespace fmt; }
#endif

#include "common.hpp"

namespace diannex
{
    class diannex_exception : public std::exception
    {
        DxStr m_what;
    public:
        template<class... Args>
        explicit diannex_exception(const DxStrRef fmt, const Args& ... args)
        #ifndef USE_FMTLIB
            : m_what(std::format(fmt, args...))
        #else
            : m_what(std::format(std::runtime(fmt), args...))
        #endif
        {}

        [[nodiscard]] const char* what() const noexcept override
        {
            return m_what.c_str();
        }
    };

    class data_processing_exception : public diannex_exception
    {
    public:
        data_processing_exception(const DxStrRef& filename, const DxStrRef& reason)
            : diannex_exception("An error occurred while processing '{}': {}", filename, reason)
        {}
    };

    template<class... Args>
    void dx_assert(bool condition, const std::string_view fmt, const Args& ... args)
    {
        if (!condition)
            throw diannex_exception(fmt, args...);
    }
}

#endif //LIBDIANNEX_EXCEPTIONS_HPP
