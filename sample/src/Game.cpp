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
#include "Game.hpp"

#include <raylib.h>

#include <iostream>
#include <algorithm>

/**
 * std::bind is apprently the devil, so this macro does nearly the same thing but with lambdas, which aren't the devil
 */
#define Bind(func) [this]<typename... Args>(Args&&... args) { func(std::forward<Args>(args)...); }

#define TRY(expr) \
try {             \
    expr;         \
}                 \
catch (const diannex::diannex_exception& ex) { \
    std::cerr << "[Diannex::Interpreter]: "    \
              << ex.what() << std::endl;       \
    std::exit(1); \
}

Game::Game()
    : m_interpreter(diannex::DxData::fromFile("data/sample.dxb"))
{

    m_interpreter.textHandler(Bind(onText));
    m_interpreter.endSceneHandler(Bind(onSceneEnd));
    m_interpreter.choiceHandler(Bind(onChoice));

    /*
     * For functors like `FlagStore` you must use `DxInterpreter::registerFunctor` to avoid making
     * a copy of the functor class, which would leave you with an inaccessible state.
     *
     * Additionally, if you have multiple overloads of `operator()` then you must manually specify
     * their type because the method won't be able to deduce it automatically it.
     */
    m_interpreter.registerFunctor<FlagStore::getter>("getFlag", m_flagStore);
    m_interpreter.registerFunctor<FlagStore::setter>("setFlag", m_flagStore);

    /*
     * Since there are no overloads for `awardPoints` and `deductPoints` the function will correctly
     * deduce its types, so no extra details are needed.
     */
    m_interpreter.registerFunction("awardPoints", awardPoints);
    m_interpreter.registerFunction("deductPoints", deductPoints);
    m_interpreter.registerFunction("getPlayerName", []
    { return "Player"s; });
}

void Game::run()
{
    m_shouldRun = true;

    InitWindow(ViewWidth * ViewScale, ViewHeight * ViewScale, "libdnxpp sample program");
    SetTargetFPS(60);

    // I have bad vision, and I'm on a 4K screen, so I have to scale this up, so I can see it lol
    RenderTexture2D target = LoadRenderTexture(ViewWidth, ViewHeight);

    TRY(m_interpreter.runScene("area0.intro"))

    while (!WindowShouldClose() && m_shouldRun)
    {
        update();

        BeginDrawing();
        {
            BeginTextureMode(target);
            render();
            EndTextureMode();

            DrawTexturePro(target.texture,
                           Rectangle{ 0.0f, 0.0f, ViewWidth, -ViewHeight },
                           Rectangle{ 0.0f, 0.0, ViewWidth * ViewScale, ViewHeight * ViewScale },
                           Vector2{ 0, 0 },
                           0.0f,
                           WHITE);
        }
        EndDrawing();
    }

    CloseWindow();
}

void Game::update()
{
    // I probably should have these constants as constexpr variables. so they can be changed easier
    if (IsKeyDown(KEY_SPACE)) m_frameCounter += 16;
    else m_frameCounter += 8;

    if (m_inChoice)
    {
        if (IsKeyPressed(KEY_DOWN)) m_selection = std::clamp(m_selection + 1, 0, (int)m_choices.size() - 1);
        else if (IsKeyPressed(KEY_UP)) m_selection = std::clamp(m_selection - 1, 0, (int)m_choices.size() - 1);
        else if (IsKeyPressed(KEY_Z))
        {
            m_inChoice = false;
            TRY(m_interpreter.selectChoice(m_selection))
        }
    }
    else if ((m_frameCounter / 10) >= m_message.size() && IsKeyPressed(KEY_Z))
    {
        TRY(m_interpreter.resumeScene())
    }
}

void Game::render() const
{
    ClearBackground(RAYWHITE);

    if (!m_inChoice)
    {
        DrawText(TextSubtext(m_message.c_str(), 0, m_frameCounter / 10), 210, 160, 20, MAROON);
    }
    else
    {
        DrawText(TextSubtext(m_message.c_str(), 0, m_frameCounter / 10), 210, 20, 20, MAROON);
        for (int i = 0; i < m_choices.size(); ++i)
            DrawText(m_choices[i].c_str(), 210, 100 + (i * 40), 20, i == m_selection ? LIME : MAROON);
    }

    DrawText("PRESS [Z] TO PROCEED!", 240, 260, 20, LIGHTGRAY);
    DrawText("PRESS [SPACE] TO SPEED UP!", 239, 300, 20, LIGHTGRAY);
}

void Game::onText(std::string text)
{
    m_frameCounter = 0;
    m_message = std::move(text);
}

void Game::onChoice(std::vector<std::string> choices)
{
    m_choices = std::move(choices);
    m_inChoice = true;
}

void Game::onSceneEnd(std::string_view)
{
    m_shouldRun = false;
    (void)m_shouldRun; // To shut the IDE up about being unused, it is used >:V
}

void Game::awardPoints(int points)
{
    std::cout << "Awarded " << points << " points\n";
}

void Game::deductPoints(bool deduct, int points)
{
    std::cout << "Deduct " << points << " points?: " << (deduct ? "Yes" : "No") << '\n';
}