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
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.h"

#include <diannex/DxInterpreter.hpp>

using namespace diannex;

struct FlagStore
{
    FlagStore() = default;

    /*
     * To verify that the `registerFunctor` method doesn't copy this functor, which would break its operation
     */
    FlagStore(const FlagStore& other)
    { throw std::runtime_error("I'VE BEEN COPYIED!"); }

    using getter = std::function<DxValue(const std::string&)>;
    using setter = std::function<void(const std::string&, const DxValue&)>;

    DxValue operator()(const std::string& name) const
    {
        return m_flags.contains(name) ? m_flags.at(name) : DxValue{};
    }

    void operator()(const std::string& name, const DxValue& value)
    {
        m_flags.insert_or_assign(name, value);
    }

    bool contains(const std::string& name)
    { return m_flags.contains(name); }

private:
    std::unordered_map<std::string, DxValue> m_flags{};
};

TEST_CASE("Interpreter can run sample scene")
{
    int points = 0;
    bool sceneEnded = false;
    std::string_view scene;
    std::vector<std::string> choices;
    FlagStore flagStore;
    std::string currentText{};

    DxInterpreter interpreter(DxData::fromFile("data/sample.dxb"));
    interpreter.textHandler([&currentText](auto str)
                            { currentText = std::move(str); });
    interpreter.endSceneHandler([&sceneEnded, &scene](auto s)
                                {
                                    sceneEnded = true;
                                    scene = s;
                                });
    interpreter.choiceHandler([&choices](auto c)
                              { choices = std::move(c); });
    interpreter.weightedChanceHandler([](auto c)
                                      { return 0; });

    interpreter.registerFunctor<FlagStore::getter>("getFlag", flagStore);
    interpreter.registerFunctor<FlagStore::setter>("setFlag", flagStore);
    interpreter.registerFunction("awardPoints", [&points](int p)
    { points += p; });
    interpreter.registerFunction("deductPoints", [&points](bool deduct, int p)
    { if (deduct) points += p; });
    interpreter.registerFunction("getPlayerName", []
    { return "Player"s; });

    SUBCASE("under normal conditions")
    {
        REQUIRE_NOTHROW(interpreter.runScene("area0.intro"));

        REQUIRE_EQ(currentText, "Welcome to the test introduction scene!");
        REQUIRE_NOTHROW(interpreter.resumeScene());
        REQUIRE_EQ(currentText, "One quick thing I have to ask before you begin...");
        REQUIRE_NOTHROW(interpreter.resumeScene());
        REQUIRE_EQ(currentText, "Is this a question?");
        REQUIRE_NOTHROW(interpreter.resumeScene());
        REQUIRE_EQ(choices.size(), 2);
        REQUIRE_EQ(choices[0], "Yes");
        REQUIRE_EQ(choices[1], "No");
        REQUIRE_NOTHROW(interpreter.selectChoice(0));
        REQUIRE_EQ(currentText, "That is correct.");
        REQUIRE_NOTHROW(interpreter.resumeScene());
        REQUIRE_EQ(points, 1);
        REQUIRE_EQ(currentText, "Either way, it was nice meeting you, Player.");
        REQUIRE_NOTHROW(interpreter.resumeScene());
        REQUIRE_EQ(currentText, "This is the end of the sample intro scene!");
        REQUIRE_NOTHROW(interpreter.resumeScene());
        REQUIRE(flagStore.contains("sample"));
        REQUIRE_EQ(flagStore("sample").type(), DxValueType::Integer);
        REQUIRE_EQ(flagStore("sample").safe_get<DxValueType::Integer>(), 1);
        REQUIRE_EQ(currentText, "Well, now it's time for a loop!");

        for (int i = 0; i < 5; ++i)
        {
            REQUIRE_NOTHROW(interpreter.resumeScene());
            REQUIRE_EQ(currentText, DxFormat("This is an example function, being passed {}", i));
        }

        REQUIRE_NOTHROW(interpreter.resumeScene());
        REQUIRE_EQ(currentText, "Or, a simpler loop!");

        for (int i = 0; i < 5; ++i)
        {
            REQUIRE_NOTHROW(interpreter.resumeScene());
            REQUIRE_EQ(currentText, "The same thing, over and over...");
        }

        REQUIRE_NOTHROW(interpreter.resumeScene());
        REQUIRE(sceneEnded);
    }
}