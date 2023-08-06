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
#include "DxInterpreter.hpp"

#include "DxInstructions.hpp"
#include "utils/BinaryReader.hpp"
#include "utils/generator.hpp"

#include <cmath>

namespace diannex
{
    void DxInterpreter::interpret(diannex::DxByteSpan buff)
    {
        auto reader = BinarySpanReader::create(buff);
        auto read = [this, &reader]<typename T>
        {
            m_programCounter += sizeof(T);
            return reader->read<T>();
        };
        auto argI = [&read]
        { return std::tuple(read.operator()<int32_t>()); };
        auto argII = [&read]
        {
            auto v1 = read.operator()<int32_t>();
            auto v2 = read.operator()<int32_t>();
            return std::tuple(v1, v2);
        };
        auto argD = [&read]
        { return std::tuple(read.operator()<double>()); };

        auto opcode = read.operator()<DxOpcode>();
        switch (opcode)
        {
            case DxOpcode::nop:
                break;
            case DxOpcode::freeloc:
            {
                auto [argIndex] = argI();
                if (argIndex == m_locals.size() - 1)
                {
                    if (argIndex < m_flagCount)
                    {
                        auto value = m_locals[argIndex];
                        dx_assert(m_flagsInitialized, "Flags not initialized before being used by an interpreter");
                        m_setFlagHandler(m_currentScene->flagNames[argIndex], value);
                    }

                    m_locals.pop_back();
                }
                break;
            }

            case DxOpcode::save:
            {
                m_saveRegister = m_stack.peek();
                break;
            }

            case DxOpcode::load:
            {
                m_stack.push(m_saveRegister.value_or(DxValue{}));
                m_saveRegister.reset();
                break;
            }

            case DxOpcode::pushu:
            {
                m_stack.push(DxValue{});
                break;
            }

            case DxOpcode::pushi:
            {
                auto [val] = argI();
                m_stack.push(DxValue{ val, DxValueType::Integer });
                break;
            }

            case DxOpcode::pushd:
            {
                auto [val] = argD();
                m_stack.push(DxValue{ val, DxValueType::Double });
                break;
            }

            case DxOpcode::pushs:
            case DxOpcode::pushbs:
            {
                auto [textIdx] = argI();
                m_stack.push(DxValue{
                    std::string{ opcode == DxOpcode::pushs ? m_data->translation(textIdx) : m_data->string(textIdx) },
                    DxValueType::String });
                break;
            }

            case DxOpcode::pushints:
            case DxOpcode::pushbints:
            {
                auto [textIdx, elemCount] = argII();
                DxStrRef str;
                if (opcode == DxOpcode::pushints)
                    str = m_data->translation(textIdx);
                else
                    str = m_data->string(textIdx);

                auto gen = [this]() -> generator<DxStr>
                {
                    while (true)
                        co_yield DxStr(this->m_stack.pop().convert(DxValueType::String).get<std::string>());
                }();
                auto elems = generate_vector<DxStr>(elemCount, gen);
                m_stack.push(DxValue{ interpolate(str, elems), DxValueType::String });
                break;
            }

            case DxOpcode::makearr:
            {
                auto [arrSize] = argI();
                DxVec<DxValue> arr(arrSize);
                for (int i = arrSize - 1; i >= 0; i--)
                    arr[i] = std::move(m_stack.pop());
                m_stack.push(DxValue{ arr, DxValueType::Array });
                break;
            }

            case DxOpcode::pusharrind:
            {
                auto ind = m_stack.pop().safe_get<DxValueType::Integer>();
                auto arr = std::move(m_stack.pop());
                if (arr.type() != DxValueType::Array)
                    panic("Array get on variable which is not an array");
                auto vArr = arr.get<DxVec<DxPtr<DxValue>>>();
                m_stack.push(*(vArr[ind]));
                break;
            }

            case DxOpcode::setarrind:
            {
                auto value = std::move(m_stack.pop());
                auto ind = m_stack.pop().safe_get<DxValueType::Integer>();
                auto& arr = m_stack.peek();
                if (arr.type() != DxValueType::Array)
                    panic("Array set on variable which is not an array");
                auto& vArr = arr.get_mut<DxVec<DxPtr<DxValue>>>();
                vArr[ind] = std::make_shared<DxValue>(value);
                break;
            }

            case DxOpcode::setvarglb:
            {
                auto name = m_data->string(m_stack.pop().safe_get<DxValueType::Integer>());
                m_setVariableHandler(name, std::move(m_stack.pop()));
                break;
            }

            case DxOpcode::setvarloc:
            {
                auto&& value = m_stack.pop();
                auto count = m_locals.size();

                auto [idx] = argI();
                if (idx >= count)
                {
                    for (int i = 0; i < idx - count; ++i)
                        m_locals.emplace_back();

                    m_locals.push_back(std::move(value));
                }
                else
                {
                    m_locals[idx] = std::move(value);
                }

                break;
            }

            case DxOpcode::pushvarglb:
            {
                auto name = m_data->string(m_stack.pop().safe_get<DxValueType::Integer>());
                m_stack.push(m_getVariableHandler(name));
                break;
            }

            case DxOpcode::pushvarloc:
            {
                auto [idx] = argI();
                if (idx >= m_locals.size())
                    m_stack.push(DxValue{});
                else
                    m_stack.push(m_locals[idx]);
                break;
            }

            case DxOpcode::pop:
                (void)m_stack.pop();
                break;

            case DxOpcode::dup:
                m_stack.push(m_stack.peek());
                break;

            case DxOpcode::dup2:
            {
                auto v1 = m_stack.pop();
                auto v2 = m_stack.pop();
                m_stack.push(v2);
                m_stack.push(v1);
                m_stack.push(v2);
                m_stack.push(v1);
                break;
            }

            case DxOpcode::add:
            case DxOpcode::sub:
            case DxOpcode::mul:
            case DxOpcode::div:
            case DxOpcode::mod:
            {
                auto v2 = m_stack.pop();
                auto v1 = m_stack.pop();
                switch (opcode)
                {
                    case DxOpcode::add:
                        m_stack.push(v1 + v2);
                        break;
                    case DxOpcode::sub:
                        m_stack.push(v1 - v2);
                        break;
                    case DxOpcode::mul:
                        m_stack.push(v1 * v2);
                        break;
                    case DxOpcode::div:
                        m_stack.push(v1 / v2);
                        break;
                    case DxOpcode::mod:
                        m_stack.push(v1 % v2);
                        break;
                    default:; // To make IDE happy despite the fact this branch is will never be touched
                }
                break;
            }

            case DxOpcode::neg:
            {
                auto v = m_stack.pop();
                auto t = v.type();
                switch (t)
                {
                    case DxValueType::Integer:
                        m_stack.push(DxValue{ -v.get<int>(), DxValueType::Integer });
                        break;
                    case DxValueType::Double:
                        m_stack.push(DxValue{ -v.get<double>(), DxValueType::Double });
                        break;
                    default:
                        panic(std::format("Cannot negate type {}", type_name(t)));
                }
                break;
            }

            case DxOpcode::inv:
            {
                auto v = m_stack.pop();
                auto t = v.type();
                switch (t)
                {
                    case DxValueType::Integer:
                        m_stack.push(DxValue{ !v.get<int>() ? 1 : 0, DxValueType::Integer });
                        break;
                    case DxValueType::Double:
                        m_stack.push(DxValue{ !(bool)(v.get<double>()) ? 1.0 : 0.0, DxValueType::Double });
                        break;
                    default:
                        panic(std::format("Cannot invert type {}", type_name(t)));
                }
                break;
            }

            case DxOpcode::bitls:
            {
                auto v2 = m_stack.pop();
                auto v1 = m_stack.pop();
                m_stack.push(DxValue{ v1.safe_get<DxValueType::Integer>() << v2.safe_get<DxValueType::Integer>(),
                                      DxValueType::Integer });
                break;
            }

            case DxOpcode::bitrs:
            {
                auto v2 = m_stack.pop();
                auto v1 = m_stack.pop();
                m_stack.push(DxValue{ v1.safe_get<DxValueType::Integer>() >> v2.safe_get<DxValueType::Integer>(),
                                      DxValueType::Integer });
                break;
            }

            case DxOpcode::_bitand:
            {
                auto v2 = m_stack.pop();
                auto v1 = m_stack.pop();
                m_stack.push(DxValue{ v1.safe_get<DxValueType::Integer>() & v2.safe_get<DxValueType::Integer>(),
                                      DxValueType::Integer });
                break;
            }

            case DxOpcode::_bitor:
            {
                auto v2 = m_stack.pop();
                auto v1 = m_stack.pop();
                m_stack.push(DxValue{ v1.safe_get<DxValueType::Integer>() | v2.safe_get<DxValueType::Integer>(),
                                      DxValueType::Integer });
                break;
            }

            case DxOpcode::bitxor:
            {
                auto v2 = m_stack.pop();
                auto v1 = m_stack.pop();
                m_stack.push(DxValue{ v1.safe_get<DxValueType::Integer>() ^ v2.safe_get<DxValueType::Integer>(),
                                      DxValueType::Integer });
                break;
            }

            case DxOpcode::bitneg:
                m_stack.push(DxValue{ ~m_stack.pop().safe_get<DxValueType::Integer>(), DxValueType::Integer });
                break;

            case DxOpcode::pow:
            {
                auto v2 = m_stack.pop();
                auto v1 = m_stack.pop();
                m_stack.push(DxValue{
                    std::pow(
                        v1.safe_get<DxValueType::Double>(),
                        v2.safe_get<DxValueType::Double>()),
                    DxValueType::Integer });
                break;
            }

            case DxOpcode::cmpeq:
            {
                auto v2 = m_stack.pop();
                auto v1 = m_stack.pop();
                m_stack.push(v1 == v2);
                break;
            }

            case DxOpcode::cmpgt:
            {
                auto v2 = m_stack.pop();
                auto v1 = m_stack.pop();
                m_stack.push(v1 > v2);
                break;
            }

            case DxOpcode::cmplt:
            {
                auto v2 = m_stack.pop();
                auto v1 = m_stack.pop();
                m_stack.push(v1 < v2);
                break;
            }

            case DxOpcode::cmpgte:
            {
                auto v2 = m_stack.pop();
                auto v1 = m_stack.pop();
                m_stack.push(v1 >= v2);
                break;
            }

            case DxOpcode::cmplte:
            {
                auto v2 = m_stack.pop();
                auto v1 = m_stack.pop();
                m_stack.push(v1 <= v2);
                break;
            }

            case DxOpcode::cmpneq:
            {
                auto v2 = m_stack.pop();
                auto v1 = m_stack.pop();
                m_stack.push(v1 != v2);
                break;
            }

            case DxOpcode::j:
            {
                auto [relAddr] = argI();
                m_programCounter += relAddr;
                break;
            }

            case DxOpcode::jt:
            {
                auto [relAddr] = argI();
                if (m_stack.pop().safe_get<DxValueType::Integer>() != 0)
                    m_programCounter += relAddr;
                break;
            }

            case DxOpcode::jf:
            {
                auto [relAddr] = argI();
                if (m_stack.pop().safe_get<DxValueType::Integer>() == 0)
                    m_programCounter += relAddr;
                break;
            }

            case DxOpcode::exit:
            {
                if (m_state == State::Eval)
                {
                    m_state = State::Inactive;
                    break;
                }

                if (m_callStack.empty())
                {
                    endScene();
                    break;
                }

                auto lastFrame = m_callStack.pop();
                m_programCounter = lastFrame.returnOffset;
                m_stack = std::move(lastFrame.stack);
                m_locals = std::move(lastFrame.locals);
                m_flagCount = lastFrame.flagCount;

                m_stack.push(DxValue{});

                break;
            }

            case DxOpcode::ret:
            {
                if (m_callStack.empty())
                {
                    endScene();
                    break;
                }

                auto returnValue = m_stack.pop();
                auto lastFrame = m_callStack.pop();
                m_stack = std::move(lastFrame.stack);
                m_locals = std::move(lastFrame.locals);

                m_stack.push(returnValue);
                break;
            }

            case DxOpcode::call:
            {
                auto [funcIdx, count] = argII();

                DxVec<DxValue> args(count);
                for (int i = 0; i < count; ++i)
                    args[i] = std::move(m_stack.pop());

                m_callStack.push({
                                     .returnOffset = m_programCounter,
                                     .stack = std::exchange(m_stack, {}),
                                     .locals = std::exchange(m_locals, {}),
                                     .flagCount = m_flagCount
                                 });
                auto func = m_data->functions()[funcIdx];
                m_programCounter = func.codeOffset;

                auto& flagNames = func.flagNames;
                m_flagCount = (int)flagNames.size();
                for (int i = 0; i < m_flagCount; ++i)
                    m_locals.push_back(std::move(m_getFlagHandler(flagNames[i])));

                for (int i = 0; i < count; ++i)
                    m_locals.push_back(std::move(args[i]));

                break;
            }

            case DxOpcode::callext:
            {
                auto [funcNameIdx, argCount] = argII();
                auto funcName = m_data->string(funcNameIdx);

                DxVec<DxValue> args(argCount);
                for (int i = 0; i < argCount; ++i)
                    args[i] = m_stack.pop();

                auto handler = m_functionHandlers.contains(funcName)
                               ? m_functionHandlers.at(funcName)
                               : ([this, funcName](auto& args) -> DxValue
                    {
                        m_unregisteredFunctionHandler(funcName);
                        return DxValue{};
                    });
                m_stack.push(handler(args));
                break;
            }

            case DxOpcode::choicebeg:
            {
                dx_assert(m_state == State::Running && !m_startingChoice, "Invalid choice begin state");

                m_startingChoice = true;
                break;
            }

            case DxOpcode::choiceadd:
            {
                dx_assert(m_startingChoice, "Invalid choice add state");
                auto [rel] = argI();

                auto chance = m_stack.pop().safe_get<DxValueType::Double>();
                auto text = m_stack.pop().safe_get<DxValueType::String>();
                if (m_chanceHandler(chance))
                    m_choiceOptions.emplace_back(m_programCounter + rel, text);
                break;
            }

            case DxOpcode::choiceaddt:
            {
                dx_assert(m_startingChoice, "Invalid choice add state");
                auto [rel] = argI();

                auto condition = m_stack.pop().safe_get<DxValueType::Integer>() != 0;
                auto chance = m_stack.pop().safe_get<DxValueType::Double>();
                auto text = m_stack.pop().safe_get<DxValueType::String>();
                if (condition && m_chanceHandler(chance))
                    m_choiceOptions.emplace_back(m_programCounter + rel, text);
                break;
            }

            case DxOpcode::choicesel:
            {
                dx_assert(m_startingChoice, "Invalid choice selection state");
                dx_assert(!m_choiceOptions.empty(), "Choice statement has no choices to present");

                m_startingChoice = false;
                m_state = State::InChoice;

                auto count = m_choiceOptions.size();
                DxVec<DxStr> textChoices(count);
                for (int i = 0; i < count; ++i)
                    textChoices[i] = m_choiceOptions[i].text;
                m_choiceHandler(std::move(textChoices));
                break;
            }

            case DxOpcode::chooseadd:
            {
                auto [rel] = argI();
                m_chooseOptions.emplace_back(m_programCounter + rel, m_stack.pop().safe_get<DxValueType::Double>());
                break;
            }

            case DxOpcode::chooseaddt:
            {
                auto [rel] = argI();
                auto condition = m_stack.pop().safe_get<DxValueType::Integer>() != 0;
                auto chance = m_stack.pop().safe_get<DxValueType::Double>();
                if (condition != 0)
                    m_chooseOptions.emplace_back(m_programCounter + rel, chance);
                break;
            }

            case DxOpcode::choosesel:
            {
                dx_assert(!m_chooseOptions.empty(), "No entries for choose statement");

                auto count = m_chooseOptions.size();
                DxVec<double> weights(count);
                for (int i = 0; i < count; ++i)
                    weights[i] = m_chooseOptions[i].chance;

                m_programCounter = m_chooseOptions[m_weighedChanceHandler(weights)].targetOffset;
                m_chooseOptions.clear();
                break;
            }

            case DxOpcode::textrun:
            {
                assert_state(State::Running, "Invalid text run state");

                m_state = State::InText;
                auto text = m_stack.pop().safe_get<DxValueType::String>();
                m_textHandler(std::move(text));
                break;
            }
        }
    }
}