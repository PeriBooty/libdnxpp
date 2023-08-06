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
#ifndef LIBDIANNEX_DXVALUE_HPP
#define LIBDIANNEX_DXVALUE_HPP

#include "common.hpp"
#include "exceptions.hpp"

using namespace std::string_literals;

namespace diannex
{
    enum class [[maybe_unused]] DxValueType : int
    {
        Integer,
        Double,
        String,
        Undefined,
        Array,
        Reference,
        Unknown
    };

    constexpr DxStrRef type_name(DxValueType type)
    {
        constexpr DxStrRef names[] = {
            "Integer",
            "Double",
            "String",
            "Undefined",
            "Array",
            "Reference",
            "Unknown"
        };
        return names[(int)type];
    }

    class DxValue
    {
        using array_type = DxVec<DxValue>;
        using variant_type = DxVariant<
            int, double, DxStr,
            array_type, DxAny>;
        using value_type = DxOpt<variant_type>;

        value_type m_value;
        DxValueType m_type;

    public:
        explicit DxValue(value_type value = std::nullopt, DxValueType type = DxValueType::Undefined);

        explicit DxValue(bool value);

        [[nodiscard]] DxValue convert(DxValueType newType) const;

        [[nodiscard]] inline DxValueType type() const
        { return m_type; }

        template<class T>
        requires
        requires() { std::get<T>(*m_value); }
        [[nodiscard]] constexpr auto get() const -> decltype(std::get<T>(*m_value))
        { return std::get<T>(*m_value); }

        template<class T>
        requires
        requires() { std::get<T>(*m_value); }
        [[nodiscard]] auto get_mut() -> decltype(std::get<T>(*m_value))
        { return std::get<T>(*m_value); }

        template<DxValueType T>
        struct dx_safe_type;

        template<>
        struct dx_safe_type<DxValueType::Integer>
        {
            using raw = int;
        };

        template<>
        struct dx_safe_type<DxValueType::Double>
        {
            using raw = double;
        };

        template<>
        struct dx_safe_type<DxValueType::String>
        {
            using raw = std::string;
        };

        template<DxValueType type, typename T = typename dx_safe_type<type>::raw>
        [[nodiscard]] auto safe_get() const -> T
        {
            if constexpr (type == DxValueType::Integer)
                return this->convert(type).get<int>();
            else if constexpr (type == DxValueType::Double)
                return this->convert(type).get<double>();
            else if constexpr (type == DxValueType::String)
                return this->convert(type).get<DxStr>();
            else
                static_assert(type != DxValueType::Integer &&
                              type != DxValueType::Double &&
                              type != DxValueType::String, "Unable to safe cast this type");
        }

        #pragma region Operators
        #define binary_op(op) DxValue operator op(const DxValue& rhs)

        binary_op(+);

        binary_op(-);

        binary_op(*);

        binary_op(/);

        binary_op(%);

        binary_op(==);

        binary_op(!=);

        binary_op(>);

        binary_op(<);

        binary_op(>=);

        binary_op(<=);

        #undef binary_op
        #pragma endregion

    private:
        void detect_type();
    };

    class value_conversion_exception : public diannex_exception
    {
    public:
        value_conversion_exception(DxValueType srcType, DxValueType dstType)
            : diannex_exception("Cannot convert type {} to {}", type_name(srcType), type_name(dstType))
        {}
    };

    class value_invalid_operator_exception : public diannex_exception
    {
    public:
        value_invalid_operator_exception(std::string_view op, DxValueType type)
            : diannex_exception("Cannot perform '{}' with type {}", op, type_name(type))
        {}
    };

}

#endif //LIBDIANNEX_DXVALUE_HPP
