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
#ifndef LIBDIANNEX_DXSTACK_HPP
#define LIBDIANNEX_DXSTACK_HPP

#include <deque>

namespace diannex
{
    template<class Container, class T>
    concept allocator_for =
    requires { std::uses_allocator_v<Container, T>; };

    template<class T>
    concept valid_container =
    requires {
        typename T::value_type;
        typename T::size_type;
        typename T::reference;
        typename T::const_reference;
    } &&
    requires(T t) {{ t.empty() } -> std::same_as<bool>; } &&
    requires(T t) {{ t.size() } -> std::same_as<typename T::size_type>; } &&
    requires(T t) {{ t.back() } -> std::same_as<typename T::reference>; } &&
    requires(const T& t) {{ t.back() } -> std::same_as<typename T::const_reference>; } &&
    requires(T t, const typename T::value_type& val) {{ t.push_back(val) }; } &&
    requires(T t, typename T::value_type&& val) {{ t.push_back(std::move(val)) }; } &&
    requires(T t, typename T::value_type&& val) {{ t.emplace_back(std::move(val)) }; } &&
    requires(T t) {{ t.pop_back() }; };

    template<class T, valid_container Container = std::deque<T>>
    class DxStack
    {
    public:
        using container_type [[maybe_unused]] = Container;
        using value_type = Container::value_type;
        using size_type = Container::size_type;
        using reference = Container::reference;
        using const_reference = Container::const_reference;

        static_assert(std::is_same_v<T, value_type>, "container adaptors require consistent types");

        [[maybe_unused]] DxStack() = default;

        [[maybe_unused]] explicit DxStack(const Container& container)
            : c(container)
        {}

        [[maybe_unused]] explicit DxStack(Container&& container) noexcept(std::is_nothrow_move_constructible_v<Container>)
            : c(std::move(container))
        {}

        template<std::input_iterator Iter>
        [[maybe_unused]] DxStack(Iter first, Iter last) noexcept(std::is_nothrow_constructible_v<Container, Iter, Iter>)
            : c(std::move(first), std::move(last))
        {}

        template<allocator_for<Container> Alloc>
        [[maybe_unused]] explicit DxStack(const Alloc& alloc) noexcept(std::is_nothrow_constructible_v<Container, const Alloc&>)
            : c(alloc)
        {}

        template<allocator_for<Container> Alloc>
        [[maybe_unused]] DxStack(const Container& container, const Alloc& alloc)
            : c(container, alloc)
        {}

        template<allocator_for<Container> Alloc>
        [[maybe_unused]] DxStack(
            Container&& container,
            const Alloc& alloc
        ) noexcept(std::is_nothrow_constructible_v<Container, Container, const Alloc&>)
            : c(std::move(container), alloc)
        {}

        template<allocator_for<Container> Alloc>
        [[maybe_unused]] DxStack(const DxStack& other, const Alloc& alloc)
            : c(other.c, alloc)
        {}

        template<allocator_for<Container> Alloc>
        [[maybe_unused]] DxStack(
            DxStack&& other,
            const Alloc& alloc
        ) noexcept(std::is_nothrow_constructible_v<Container, Container, const Alloc&>)
            : c(std::move(other.c), alloc)
        {}

        template<std::input_iterator Iter, allocator_for<Container> Alloc>
        [[maybe_unused]] DxStack(
            Iter first,
            Iter last,
            const Alloc& alloc
        ) noexcept(std::is_nothrow_constructible_v<Container, Iter, Iter, const Alloc&>)
            : c(std::move(first), std::move(last), alloc)
        {}

        [[nodiscard, maybe_unused]] bool empty() const
        { return c.empty(); }

        [[nodiscard, maybe_unused]] size_type size() const
        { return c.size(); }

        [[nodiscard, maybe_unused]] reference peek()
        { return c.back(); }

        [[nodiscard, maybe_unused]] const_reference peek() const
        { return c.back(); }

        void push(const value_type& val)
        { c.push_back(val); }

        void push(value_type&& val)
        { c.push_back(std::move(val)); }

        template<class... Args>
        [[nodiscard, maybe_unused]] decltype(auto) emplace(Args&& ... args)
        {
            return c.emplace_back(std::forward<Args>(args)...);
        }

        [[nodiscard, maybe_unused]] value_type pop()
        {
            value_type result = std::move(c.back());
            c.pop_back();
            return std::move(result);
        }

        [[maybe_unused]] void clear()
        requires
        requires(Container container) {{ container.clear() }; }
        {
            c.clear();
        }

        [[maybe_unused]] void clear()
        {
            while (!empty())
                pop();
        }

    protected:
        Container c{};
    };
}

#endif //LIBDIANNEX_DXSTACK_HPP
