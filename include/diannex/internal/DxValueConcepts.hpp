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
#ifndef LIBDIANNEX_DXVALUECONCEPTS_HPP
#define LIBDIANNEX_DXVALUECONCEPTS_HPP

#include "../DxValue.hpp"

namespace diannex
{
    template<class T>
    concept DxCopyCoercable = !std::integral<T> &&
                              !std::floating_point<T> &&
                              !DxStrLike<T> &&
                              !
                                  std::same_as<std::remove_cvref<T>, DxValue> &&
                              std::copy_constructible<T>
                              &&
                              !
                                  std::move_constructible<T>;

    template<class T>
    concept DxMoveCoercable = !std::integral<T> &&
                              !std::floating_point<T> &&
                              !DxStrLike<T> &&
                              !
                                  std::same_as<std::remove_cvref<T>, DxValue> &&
                              std::move_constructible<T>;

    template<typename Class, typename T>
    concept same_as_lax = std::same_as<std::remove_cvref<Class>, std::remove_cvref<T>>;

    template<class T>
    concept DxValueLike = std::common_with<DxValue, T>;

    inline DxValue coerce_to_value(std::integral auto value)
    {
        return DxValue{ (int)value, DxValueType::Integer };
    }

    inline DxValue coerce_to_value(std::floating_point auto value)
    {
        return DxValue{ (double)value, DxValueType::Double };
    }

    inline DxValue coerce_to_value(DxStrLike auto const& value)
    {
        return DxValue{ std::move(std::string(value)), DxValueType::String };
    }

    inline DxValue coerce_to_value(DxCopyCoercable auto const& value)
    {
        return DxValue{ DxAny(value), DxValueType::Reference };
    }

    inline DxValue coerce_to_value(DxMoveCoercable auto&& value)
    {
        return DxValue{ std::any(std::forward<decltype(value)>(value)), DxValueType::Reference };
    }

    inline DxValue coerce_to_value(DxValueLike auto const& value)
    {
        return value;
    }

    template<std::integral T>
    inline auto coerce_from_value(const DxValue& value) -> T
    {
        return (T)
            value.get<int>();
    }

    template<std::floating_point T>
    inline auto coerce_from_value(const DxValue& value) -> T
    {
        return (T)
            value.get<double>();
    }

    template<std::constructible_from<DxStr> T>
    inline auto coerce_from_value(const DxValue& value) -> T
    {
        return (T)
            value.get<std::string>();
    }

    template<DxCopyCoercable T>
    inline auto coerce_from_value(const DxValue& value) -> T
    {
        return std::any_cast<T>(value.get<DxAny>());
    }

    template<DxValueLike T>
    inline auto coerce_from_value(const DxValue& value) -> T
    {
        return value;
    }

    template<class T>
    concept DxCoercableTo =
    requires { same_as_lax<T, DxValue>; } ||
    requires { std::same_as<T, void>; } ||
    requires(T t) {{ coerce_to_value(t) } -> same_as_lax<DxValue>; } ||
    requires(const T& t) {{ coerce_to_value(t) } -> same_as_lax<DxValue>; } ||
    requires(T&& t) {{ coerce_to_value(std::move(t)) } -> same_as_lax<DxValue>; };

    template<class T>
    concept DxCoercableFrom =
    requires { same_as_lax<T, DxValue>; } ||
    requires(const DxValue& value)
    {
        {
        coerce_from_value<T>(value)
        }
        ->
        same_as_lax<T>;
    };

    template<typename T>
    struct is_coercable_to
    {
        static constexpr bool value = false;
    };

    template<DxCoercableTo T>
    struct is_coercable_to<T>
    {
        static constexpr bool value = true;
    };

    template<typename T>
    struct is_coercable_from
    {
        static constexpr bool value = false;
    };

    template<DxCoercableFrom T>
    struct is_coercable_from<T>
    {
        static constexpr bool value = true;
    };
}

#endif //LIBDIANNEX_DXVALUECONCEPTS_HPP
