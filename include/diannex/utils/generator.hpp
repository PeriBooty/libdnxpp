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
#ifndef LIBDIANNEX_GENERATOR_HPP
#define LIBDIANNEX_GENERATOR_HPP

#include <coroutine>
#include <variant>
#include <vector>

namespace diannex
{
    template<class T>
    struct generator
    {
        struct promise_type;
        using handle_type = std::coroutine_handle<promise_type>;

        struct promise_type
        {
            friend generator;

            generator get_return_object()
            {
                return generator(handle_type::from_promise(*this));
            }

            std::suspend_always initial_suspend()
            { return {}; }

            std::suspend_always final_suspend() noexcept
            { return {}; }

            void unhandled_exception()
            { m_exception = std::current_exception(); }

            template<std::convertible_to<T> From>
            std::suspend_always yield_value(From&& from)
            {
                m_value = std::forward<From>(from);
                return {};
            }

            void return_void()
            {}

        private:
            T m_value;
            std::exception_ptr m_exception;
        };

        ~generator()
        { m_handle.destroy(); }

        explicit operator bool()
        {
            fill();

            return !m_handle.done();
        }

        T operator()()
        {
            fill();
            m_full = false;

            return std::move(m_handle.promise().m_value);
        }

    private:
        generator(handle_type handle)
            : m_handle(handle)
        {}

        handle_type m_handle;
        bool m_full{ false };

        void fill()
        {
            if (!m_full)
            {
                m_handle();
                if (auto exception = m_handle.promise().m_exception)
                    std::rethrow_exception(exception);

                m_full = true;
            }
        }
    };

    template<class T>
    std::vector<T> generate_vector(size_t count, generator <T>& generator)
    {
        std::vector<T> result;
        result.reserve(count);
        for (int i = 0; i < count && generator; i++)
            result.emplace_back(generator());
        return std::move(result);
    }
}

#endif //LIBDIANNEX_GENERATOR_HPP
