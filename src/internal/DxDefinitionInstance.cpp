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

namespace diannex::_internal
{
    DxDefinitionInstance::DxDefinitionInstance(
        DxDefinition target,
        DxWeakPtr<DxData> data,
        DxWeakPtr<DxInterpreter> interpreter
    )
        : m_target(target), m_data(std::move(data)), m_interpreter(std::move(interpreter))
    {}

    DxStrRef DxDefinitionInstance::value()
    {
        auto currentCacheId = m_data.lock()->cacheID();
        if (currentCacheId != m_cachedId || !m_cachedValue)
        {
            m_cachedId = currentCacheId;
            return valueNoCache();
        }

        return *m_cachedValue;
    }

    DxStrRef DxDefinitionInstance::valueNoCache()
    {
        auto data = m_data.lock();
        if (m_target.codeOffset != -1)
        {
            auto interpreter = m_interpreter.lock();
            interpreter->executeEvalMultiple(m_target.codeOffset);
            auto& stack = interpreter->m_stack;
            auto elemCount = stack.size();
            std::vector<DxStr> elems(elemCount);
            for (decltype(elemCount) i = 0; i < elemCount; ++i)
            {
                elems[i] = stack.pop().convert(DxValueType::String).get<std::string>();
            }

            if (m_target.isInternal)
            {
                m_cachedValue.emplace(
                    std::move(interpreter->interpolate(data->string(m_target.valueStringIndex), elems)));
                return *m_cachedValue;
            }

            m_cachedValue.emplace(
                std::move(interpreter->interpolate(data->translation(m_target.valueStringIndex), elems)));
            return *m_cachedValue;
        }

        if (m_target.isInternal)
        {
            m_cachedValue = data->string(m_target.valueStringIndex);
            return *m_cachedValue;
        }

        m_cachedValue = data->translation(m_target.valueStringIndex);
        return *m_cachedValue;
    }
}