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
#include "DxValue.hpp"

namespace diannex
{
    DxValue::DxValue(DxValue::value_type value, DxValueType type)
        : m_value(std::move(value)), m_type(type)
    {
        if (m_value && m_type == DxValueType::Undefined)
            detect_type();
    }

    DxValue::DxValue(bool value)
        : m_value((int)value), m_type(DxValueType::Integer)
    {}

    void DxValue::detect_type()
    {
        auto inner = m_value.value();
        if (std::holds_alternative<array_type>(inner))
            m_type = DxValueType::Array;
        else if (std::holds_alternative<DxStr>(inner))
            m_type = DxValueType::String;
        else if (std::holds_alternative<int>(inner))
            m_type = DxValueType::Integer;
        else if (std::holds_alternative<double>(inner))
            m_type = DxValueType::Double;
        else
            m_type = DxValueType::Reference;
    }

    DxValue DxValue::convert(diannex::DxValueType newType) const
    {
        if (m_type == newType || newType == DxValueType::Undefined)
            return *this;

        switch (m_type)
        {
            case DxValueType::Double:
                switch (newType)
                {
                    case DxValueType::Integer:
                        return DxValue{ (int)floor(std::get<double>(*m_value)), DxValueType::Integer };
                    case DxValueType::String:
                        return DxValue{ std::to_string(std::get<double>(*m_value)), DxValueType::String };
                    default:
                        break;
                }
                break;
            case DxValueType::Integer:
                switch (newType)
                {
                    case DxValueType::Double:
                        return DxValue{ (double)std::get<int>(*m_value), DxValueType::Double };
                    case DxValueType::String:
                        return DxValue{ std::to_string(std::get<int>(*m_value)), DxValueType::String };
                    default:
                        break;
                }
                break;
            case DxValueType::String:
                switch (newType)
                {
                    case DxValueType::Double:
                        return DxValue{ std::stod(std::get<std::string>(*m_value)), DxValueType::Double };
                    case DxValueType::Integer:
                        return DxValue{ std::stoi(std::get<std::string>(*m_value)), DxValueType::Integer };
                    default:
                        break;
                }
                break;
            case DxValueType::Undefined:
                switch (newType)
                {
                    case DxValueType::String:
                        return DxValue{ "undefined"s, DxValueType::String };
                    default:
                        break;
                }
                break;
            default:
                break;
        }

        throw value_conversion_exception(m_type, newType);
    }

    #pragma region Operators

    #define op_convert_cond(op, cond) \
    if (this->m_type != rhs.m_type)   \
        return (cond) ?               \
            (*this op rhs.convert(this->m_type)) : \
            (this->convert(rhs.m_type) op rhs) \

    #define op_nullcheck(op, value) \
    [[unlikely]]                    \
    if ((this->m_type == DxValueType::Undefined) != (rhs.m_type == DxValueType::Undefined)) \
        return DxValue{value}

    #define op_branch(op, type, enum_value) \
    case DxValueType::enum_value:           \
        return DxValue{this->get<type>() op rhs.get<type>(), DxValueType::enum_value}

    #define op_null_branch(value) \
    case DxValueType::Undefined: \
        return DxValue{value}

    #define op_throw(op) \
    default: throw value_invalid_operator_exception(#op, this->m_type)

    #define op_signature_cond(op, cond, ...) \
    DxValue DxValue::operator op(const DxValue& rhs) \
    {                                        \
        op_convert_cond(op, cond);           \
                                             \
        switch (this->m_type)                \
        {                                    \
            __VA_ARGS__;                     \
            op_throw(op);                    \
        }                                    \
    }

    #define op_nullcheck_signature_cond(op, if_both_null, if_both_not_null, cond, ...) \
    DxValue DxValue::operator op(const DxValue& rhs)                                   \
    {                                                                                  \
        op_nullcheck(op, if_both_not_null);                                            \
        op_convert_cond(op, cond);                                                     \
                                                                                       \
        switch (this->m_type)                                                          \
        {                                                                              \
            __VA_ARGS__;                                                               \
            op_null_branch(if_both_null);                                              \
            op_throw(op);                                                              \
        }                                                                              \
    }

    #define operator_i(op) \
    op_signature_cond(op, m_type == DxValueType::Double && rhs.m_type == DxValueType::Integer, op_branch(op, int, Integer))

    #define operator_di(op) \
    op_signature_cond(op, m_type == DxValueType::Double && rhs.m_type == DxValueType::Integer,        \
        op_branch(op, double, Double); \
        op_branch(op, int, Integer))

    #define operator_dis(op, cond) \
    op_signature_cond(op, cond,         \
        op_branch(op, double, Double); \
        op_branch(op, int, Integer);   \
        op_branch(op, std::string, String))

    #define operator_disu(op, if_both_null, if_both_not_null) \
    op_nullcheck_signature_cond(op, if_both_null, if_both_not_null, m_type == DxValueType::Double && rhs.m_type == DxValueType::Integer,\
        op_branch(op, double, Double);                        \
        op_branch(op, int, Integer);                          \
        op_branch(op, std::string, String))

    operator_dis(+,
                 m_type == DxValueType::String || (m_type == DxValueType::Double && rhs.m_type == DxValueType::Integer))

    operator_di(-)

    operator_di(*)

    operator_di(/)

    operator_i(%)

    operator_disu(==, true, false)

    operator_disu(!=, false, true)

    operator_di(>)

    operator_di(<)

    operator_di(>=)

    operator_di(<=)

    #pragma endregion
}