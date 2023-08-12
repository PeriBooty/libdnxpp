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
#ifndef LIBDIANNEX_DXINTERPRETER_HPP
#define LIBDIANNEX_DXINTERPRETER_HPP

#include "DxData.hpp"
#include "utils/DxStack.hpp"
#include "internal/DxValueConcepts.hpp"

namespace diannex
{
    // Forward Declaration
    class DxInterpreter;

    namespace _internal
    {
        class DxDefinitionInstance
        {
            DxDefinition m_target;
            DxWeakPtr<DxData> m_data;
            DxWeakPtr<DxInterpreter> m_interpreter;

            DxOpt<DxStr> m_cachedValue{ std::nullopt };
            int m_cachedId{ -1 };
        public:
            DxDefinitionInstance(DxDefinition target, DxWeakPtr<DxData> data, DxWeakPtr<DxInterpreter> interpreter);

            DxStrRef value();

            DxStrRef valueNoCache();
        };

        template<class Expected, class Actual>
        auto make_copyable_functor(Actual&& func) -> Expected
        {
            auto raw = &func;
            return [raw](auto&& ... args) -> decltype(auto)
            {
                return (*raw)(decltype(args)(args)...);
            };
        }

        template<typename InT, typename OutT>
        concept constructible_to = std::constructible_from<OutT, InT>;
    }

    class DxInterpreter : std::enable_shared_from_this<DxInterpreter>
    {
        using DxFuncSig = DxFunc<DxValue(const DxVec<DxValue>&)>;
        using DxFuncMap = DxMap<DxStrRef, DxFuncSig>;

        friend class _internal::DxDefinitionInstance;

        enum class State
        {
            Inactive,
            Running,
            Paused,
            InText,
            InChoice,
            Eval
        };

        struct StackFrame
        {
            int returnOffset{};
            DxStack<DxValue> stack{};
            DxVec<DxValue> locals{};
            int flagCount{};
        };

        struct ChoiceEntry
        {
            int targetOffset{};
            DxStr text{};
        };

        struct ChooseEntry
        {
            int targetOffset{};
            double chance{};
        };

        DxPtr<DxData> m_data;
        DxFuncMap m_functionHandlers{};

        State m_state{ State::Inactive };
        int m_programCounter{ -1 };
        DxStack<DxValue> m_stack{};
        DxStack<StackFrame> m_callStack{};
        DxVec<DxValue> m_locals{};
        DxVec<ChoiceEntry> m_choiceOptions{};
        DxVec<ChooseEntry> m_chooseOptions{};
        DxOpt<DxValue> m_saveRegister{ std::nullopt };
        int m_flagCount{ 0 };
        DxOpt<DxScene> m_currentScene{ std::nullopt };
        bool m_startingChoice{ false };
        DxMap<DxStrRef, _internal::DxDefinitionInstance> m_definitions{};
        bool m_flagsInitialized{ false };
    public:
        template<DxCoercableTo R, DxCoercableFrom... Args, std::size_t... Is>
        static DxValue registerFunctionImpl(
            const DxVec<DxValue>& args,
            DxFunc<R(Args...)> func,
            std::index_sequence<Is...>
        )
        {
            if constexpr (std::is_same_v<R, void>)
            {
                func(coerce_from_value<Args>(args[Is])...);
                return DxValue{};
            }
            else if constexpr (DxValueLike<R>)
            {
                return func(coerce_from_value<Args>(args[Is])...);
            }
            else
            {
                return coerce_to_value(func(coerce_from_value<Args>(args[Is])...));
            }
        }

        using UnregisteredFunctionCallback = DxFunc<void(DxStrRef)>;
        using TextCallback = DxFunc<void(DxStr)>;
        using VariableSetCallback = DxFunc<void(DxStrRef, DxValue)>;
        using VariableGetCallback = DxFunc<DxValue(DxStrRef)>;
        using EndSceneCallback = DxFunc<void(DxStrRef)>;
        using ChanceCallback = DxFunc<bool(double)>;
        using WeightedChanceCallback = DxFunc<int(const DxVec<double>&)>;
        using SetFlagCallback = DxFunc<void(DxStrRef, DxValue)>;
        using GetFlagCallback = DxFunc<DxValue(DxStrRef)>;
        using ChoiceCallback = DxFunc<void(DxVec<DxStr>)>;

        explicit DxInterpreter(DxData&& data);

        void interpret(DxByteSpan buff);

        void runScene(const DxStrRef& name);

        [[maybe_unused]] void pauseScene();

        void resumeScene();

        void endScene();

        void selectChoice(int idx);

        [[maybe_unused]] DxStrRef definition(const DxStrRef& name);

        DxValue executeEval(int address);

        void executeEvalMultiple(int address);

        static DxStr interpolate(const DxStrRef& str, const DxROSpan<DxStr>& elems);

        void registerFunctionSafe(const DxStrRef& name, const DxFuncSig& func);

        template<typename R, DxCoercableFrom... Args>
        void registerFunctionInternal(const DxStrRef& name, DxFunc<R(Args...)>&& func)
        {
            registerFunctionSafe(name,
                                 [func = std::forward<DxFunc<R(Args...)>>(func)](const DxVec<DxValue>& args) -> DxValue
                                 {
                                     return registerFunctionImpl<R, Args...>(
                                         args,
                                         func,
                                         std::index_sequence_for<Args...>{});
                                 });
        }

        template<typename Func>
        void registerFunction(const DxStrRef& name, Func&& func)
        {
            registerFunctionInternal(name, std::function(std::forward<Func>(func)));
        }

        template<typename Func>
        void registerFunctor(const DxStrRef& name, _internal::constructible_to<Func> auto&& func)
        {
            registerFunctionInternal(name, _internal::make_copyable_functor<Func>(func));
        }

        DxInterpreter& textHandler(TextCallback func);

        DxInterpreter& choiceHandler(ChoiceCallback func);

        [[maybe_unused]] DxInterpreter& variableSetHandler(VariableSetCallback func);

        [[maybe_unused]] DxInterpreter& variableGetHandler(VariableGetCallback func);

        [[maybe_unused]] DxInterpreter& endSceneHandler(EndSceneCallback func);

        [[maybe_unused]] DxInterpreter& chanceHandler(ChanceCallback func);

        [[maybe_unused]] DxInterpreter& weightedChanceHandler(WeightedChanceCallback func);

        [[maybe_unused]] DxInterpreter& flagSetHandler(SetFlagCallback func);

        [[maybe_unused]] DxInterpreter& flagGetHandler(GetFlagCallback func);

        bool initializeFlags();

        void resetFlags();

        [[maybe_unused]] static double random_real(double min = 0.0, double max = 1.0);

        [[maybe_unused]] static int random_int(int min = 0, int max = std::numeric_limits<int>::max());

    private:
        UnregisteredFunctionCallback m_unregisteredFunctionHandler;
        TextCallback m_textHandler;
        VariableSetCallback m_setVariableHandler;
        VariableGetCallback m_getVariableHandler;
        EndSceneCallback m_endSceneHandler;
        ChanceCallback m_chanceHandler;
        WeightedChanceCallback m_weighedChanceHandler;
        SetFlagCallback m_setFlagHandler;
        GetFlagCallback m_getFlagHandler;
        ChoiceCallback m_choiceHandler;

        void assert_state(State state, const DxStrRef& message);

        void clearVMState();

        template<typename R, typename... Args>
        auto stub(const std::string_view& message) -> DxFunc<R(Args...)>
        {
            return [message](Args...) -> R
            { throw diannex_exception(message); };
        }

        [[noreturn]] void panic(const std::string_view& message);

        friend class interpreter_runtime_exception;
    };

    class interpreter_runtime_exception : public diannex_exception
    {
    public:
        interpreter_runtime_exception(const DxInterpreter& interpreter, const std::string_view& message);
    };
}

#endif //LIBDIANNEX_DXINTERPRETER_HPP
