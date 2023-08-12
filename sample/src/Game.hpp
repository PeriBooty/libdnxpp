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
#ifndef LIBDNXPP_GAME_HPP
#define LIBDNXPP_GAME_HPP

#include <diannex/DxInterpreter.hpp>
#include <raylib.h>

/**
 * Overtly complicated way of doing this, honestly just use a lambda lol
 */
struct FlagStore
{
    FlagStore() = default;

    /*
     * These are used as type hints for the `DxInterpreter::registerFunctor` method,
     * this allows the method to differentiate between the two overloads, since
     * otherwise it will be unable to deduce the types.
     */
    using getter = std::function<diannex::DxValue(const std::string&)>;
    using setter = std::function<void(const std::string&, const diannex::DxValue&)>;

    diannex::DxValue operator()(const std::string& name)
    { return m_flags.contains(name) ? m_flags.at(name) : diannex::DxValue{}; }

    void operator()(const std::string& name, const diannex::DxValue& value)
    { m_flags.insert_or_assign(name, value); }

private:
    std::unordered_map<std::string, diannex::DxValue> m_flags{};
};

class Game
{
    static constexpr int ViewWidth = 800;
    static constexpr int ViewHeight = 450;
    static constexpr int ViewScale = 2;

    int m_frameCounter = 0;
    std::string m_message{};
    FlagStore m_flagStore{};
    diannex::DxInterpreter m_interpreter;

    bool m_shouldRun = false;

    // Choice handling
    bool m_inChoice = false;
    std::vector<std::string> m_choices{};
    int m_selection = 0;
public:
    Game();

    void run();

private:
    void update();

    void render() const;

    void onText(std::string text);

    void onChoice(std::vector<std::string> choices);

    void onSceneEnd(std::string_view);

    static void awardPoints(int points);

    static void deductPoints(bool deduct, int points);
};

#endif //LIBDNXPP_GAME_HPP
