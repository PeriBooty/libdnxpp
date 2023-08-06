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

#include <random>

namespace diannex
{
    void DxInterpreter::assert_state(State state, const DxStrRef& message)
    {
        if (state != m_state)
            throw diannex_exception(message);
    }

    struct DefaultVariableStore
    {
        DxMap<DxStr, DxValue> container;

        DxValue operator()(DxStrRef name)
        {
            return container.at(DxStr{ name });
        }

        void operator()(DxStrRef name, const DxValue& value)
        {
            container.emplace(std::make_pair(DxStr{ name }, value));
        }
    };

    DefaultVariableStore defaultFlagStore;
    DefaultVariableStore defaultVarStore;

    DxInterpreter::DxInterpreter(DxData&& data)
        : m_data(std::make_shared<DxData>(std::move(data)))
    {
        m_unregisteredFunctionHandler = [](auto name)
        { throw diannex_exception("Unregistered function \"{}\"", name); };

        m_textHandler = stub<void, DxStrRef>(
            "Missing text handler. Set a text handler with 'DxInterpreter::textHandler' before using the interpreter");
        m_choiceHandler = stub<void, DxVec<DxStr>>(
            "Missing choice handler. Set one with 'DxInterpreter::choiceHandler' before using the interpreter");

        m_setVariableHandler = defaultVarStore;
        m_getVariableHandler = defaultVarStore;
        m_setFlagHandler = defaultFlagStore;
        m_getFlagHandler = defaultFlagStore;

        m_functionHandlers.try_emplace("char", [](auto args) -> DxValue
        { return DxValue{}; });
        m_endSceneHandler = [](auto name)
        {};
        m_chanceHandler = [](auto chance)
        { return chance == 1 || random_real() < chance; };
        m_weighedChanceHandler = [](auto chances)
        {
            auto sum = 0.0;
            auto count = chances.size();
            auto fixedWeights = std::vector<double>(count);
            for (int i = 0; i < count; ++i)
            {
                fixedWeights.push_back(sum);
                sum += chances[i];
            }

            auto r = random_real(0.0, sum);
            int sel = -1;
            double prev = -1;

            for (int i = 0; i < count; ++i)
            {
                auto curr = fixedWeights[i];
                if (r >= curr && curr > prev)
                {
                    sel = i;
                    prev = curr;
                }
            }

            return sel;
        };
    }

    void DxInterpreter::runScene(const DxStrRef& name)
    {
        m_currentScene.emplace(std::move(m_data->scene(name)));
        m_programCounter = m_currentScene->codeOffset;
        if (m_programCounter == -1)
            return;
        m_state = State::Running;
        clearVMState();

        // Load flags into local variables
        auto& flagNames = m_currentScene->flagNames;
        for (const auto& flagName: flagNames)
            m_locals.emplace_back(std::move(m_getFlagHandler(flagName)));

        auto buff = m_data->instructions();
        while (m_state == State::Running)
        {
            interpret(buff.subspan(m_programCounter));
        }
    }

    [[maybe_unused]]
    void DxInterpreter::pauseScene()
    {
        if (m_state == State::Running)
            m_state = State::Paused;
    }

    void DxInterpreter::resumeScene()
    {
        if (m_state == State::Paused || m_state == State::InText)
            m_state = State::Running;

        auto buff = m_data->instructions();
        while (m_state == State::Running)
            interpret(buff.subspan(m_programCounter));
    }

    void DxInterpreter::endScene()
    {
        m_state = State::Inactive;
        auto name = m_currentScene->name;
        m_currentScene.reset();
        clearVMState();
        m_endSceneHandler(name);
    }

    void DxInterpreter::selectChoice(int idx)
    {
        assert_state(State::InChoice, "Attempting to select choice in invalid state");

        m_programCounter = m_choiceOptions[idx].targetOffset;
        m_choiceOptions.clear();

        m_state = State::Running;
        auto buff = m_data->instructions();
        while (m_state == State::Running)
            interpret(buff.subspan(m_programCounter));
    }

    [[maybe_unused]]
    DxStrRef DxInterpreter::definition(const DxStrRef& name)
    {
        if (!m_definitions.contains(name))
        {
            auto baseDef = m_data->definition(name);
            _internal::DxDefinitionInstance instance{
                baseDef,
                m_data,
                shared_from_this()
            };
            m_definitions.emplace(std::make_pair(name, instance));
        }
        return m_definitions.at(name).value();
    }

    DxValue DxInterpreter::executeEval(int address)
    {
        assert_state(State::Inactive,
                     "Invalid evaluation state in interpreter - make a separate interpreter?");

        m_state = State::Eval;
        m_programCounter = address;

        auto buff = m_data->instructions();
        while (m_state == State::Eval)
            interpret(buff.subspan(m_programCounter));

        return std::move(m_stack.pop());
    }

    void DxInterpreter::executeEvalMultiple(int address)
    {
        assert_state(State::Inactive,
                     "Invalid execution state in interpreter - make a separate interpreter?");

        m_state = State::Eval;
        m_programCounter = address;

        auto buff = m_data->instructions();
        while (m_state == State::Eval)
            interpret(buff.subspan(m_programCounter));
    }

    DxStr DxInterpreter::interpolate(const DxStrRef& str, const DxROSpan<DxStr>& elems)
    {
        auto elemCount = elems.size();

        DxStrBuilder result;
        for (auto pos = 0; pos < str.length(); ++pos)
        {
            auto c = str.at(pos);
            if (c == '\\' && (pos + 1) <= str.length())
            {
                pos++; // Skip escaped character
                continue;
            }
            if (c == '$' && (pos + 2) <= str.length())
            {
                auto startPos = pos;
                if (str.at(pos + 1) == '{')
                {
                    pos += 2; // Skip '${' from interpolation string
                    DxStrBuilder build;
                    c = str.at(pos);
                    do
                    {
                        build << c;
                        pos++;
                        c = str.at(pos);
                    }
                    while (c != '}' && pos <= str.length());

                    if (c != '}')
                    {
                        // Backtrack; ignore this
                        pos = startPos + 1;
                        result << '$';
                        continue;
                    }

                    int index;
                    try
                    {
                        index = std::stoi(build.str());
                    }
                    catch (...)
                    {
                        // Backtrack; ignore this
                        pos = startPos + 1;
                        result << '$';
                        continue;
                    }

                    if (index < 0 || index >= elemCount)
                    {
                        // Backtrack; ignore this;
                        pos = startPos + 1;
                        result << '$';
                        continue;
                    }

                    result << elems[index];
                    continue;
                }
            }

            result << c;
        }

        return result.str();
    }

    void DxInterpreter::registerFunctionSafe(const DxStrRef& name, const DxFuncSig& func)
    {
        m_functionHandlers.insert_or_assign(name, func);
    }

    #define setter(name, callback, field) \
    DxInterpreter& DxInterpreter::name(callback func) \
    {                          \
        (field) = std::move(func); \
        return *this;          \
    }

    setter(textHandler, TextCallback, m_textHandler)

    setter(variableSetHandler, VariableSetCallback, m_setVariableHandler)

    setter(variableGetHandler, VariableGetCallback, m_getVariableHandler)

    setter(endSceneHandler, EndSceneCallback, m_endSceneHandler)

    setter(chanceHandler, ChanceCallback, m_chanceHandler)

    setter(weightedChanceHandler, WeightedChanceCallback, m_weighedChanceHandler)

    setter(flagSetHandler, SetFlagCallback, m_setFlagHandler)

    setter(flagGetHandler, GetFlagCallback, m_getFlagHandler)

    setter(choiceHandler, ChoiceCallback, m_choiceHandler)

    bool DxInterpreter::initializeFlags()
    {
        if (m_flagsInitialized)
        {
            resetFlags();
            return false;
        }

        for (auto& [_, scene]: m_data->scenes_mut())
        {
            for (int i = 0; i < scene.flagOffsets.size(); i += 2)
            {
                auto value = executeEval(scene.flagOffsets[i]);
                auto name = executeEval(scene.flagOffsets[i + 1])
                    .convert(DxValueType::String)
                    .get<DxStr>();
                auto& n = scene.flagNames[i / 2] = std::move(name);
                m_setFlagHandler(n, value);
            }
        }

        for (auto& function: m_data->functions_mut())
        {
            for (int i = 0; i < function.flagOffsets.size(); i += 2)
            {
                auto value = executeEval(function.flagOffsets[i]);
                auto name = executeEval(function.flagOffsets[i + 1])
                    .convert(DxValueType::String)
                    .get<DxStr>();
                auto& n = function.flagNames[i / 2] = std::move(name);
                m_setFlagHandler(n, value);
            }
        }

        m_flagsInitialized = true;

        return true;
    }

    void DxInterpreter::resetFlags()
    {
        if (!m_flagsInitialized)
        {
            initializeFlags();
            return;
        }

        for (const auto& [_, scene]: m_data->scenes())
        {
            for (int i = 0; i < scene.flagOffsets.size(); i += 2)
            {
                auto value = executeEval(scene.flagOffsets[i]);
                auto name = scene.flagNames[i / 2];
                m_setFlagHandler(name, value);
            }
        }

        for (const auto& function: m_data->functions())
        {
            for (int i = 0; i < function.flagOffsets.size(); i += 2)
            {
                auto value = executeEval(function.flagOffsets[i]);
                auto name = function.flagNames[i / 2];
                m_setFlagHandler(name, value);
            }
        }
    }

    double DxInterpreter::random_real(double min, double max)
    {
        static std::random_device rd;
        static std::mt19937_64 gen{ rd() };
        std::uniform_real_distribution<> dist{ min, max };
        return dist(gen);
    }

    [[maybe_unused]]
    int DxInterpreter::random_int(int min, int max)
    {
        static std::random_device rd;
        static std::mt19937_64 gen{ rd() };
        std::uniform_int_distribution<> dist{ min, max };
        return dist(gen);
    }

    void DxInterpreter::clearVMState()
    {
        m_stack.clear();
        m_callStack.clear();
        m_locals.clear();
        m_choiceOptions.clear();
        m_chooseOptions.clear();
        m_saveRegister.reset();
    }

    interpreter_runtime_exception::interpreter_runtime_exception(
        const diannex::DxInterpreter& interpreter,
        const std::string_view& message
    )
        : diannex_exception("Diannex Runtime Error (scene: {}): {}", interpreter.m_currentScene->name, message)
    {}

    void DxInterpreter::panic(const std::string_view& message)
    {
        throw interpreter_runtime_exception(*this, message);
    }
}
