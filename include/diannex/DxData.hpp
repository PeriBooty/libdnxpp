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
#ifndef LIBDIANNEX_DXDATA_HPP
#define LIBDIANNEX_DXDATA_HPP

#include "common.hpp"
#include "models.hpp"

namespace diannex
{
    class DxData
    {
        int m_currentCacheID{ -1 };

        DxVec<DxStr> m_strings;
        DxVec<DxStr> m_translations;
        DxVec<std::byte> m_instructions;
        DxVec<DxFunction> m_functions;
        DxMap<DxStrRef, DxScene> m_scenes;
        DxMap<DxStrRef, DxDefinition> m_definitions;
        DxOpt<DxVec<DxStr>> m_originalText;
    public:
        static constexpr int FormatVersion = 4;
        static constexpr int TranslationFormatVersion = 0;

        [[nodiscard]] DxStrRef string(size_t idx) const;

        [[nodiscard]] DxStrRef translation(size_t idx) const;

        [[nodiscard]] DxScene scene(const DxStrRef& name) const;

        [[nodiscard]] const DxMap<DxStrRef, DxScene>& scenes() const;

        [[nodiscard]] DxMap<DxStrRef, DxScene>& scenes_mut();

        [[nodiscard]] DxROSpan<DxFunction> functions() const;

        [[nodiscard]] DxSpan<DxFunction> functions_mut();

        [[nodiscard]] DxDefinition definition(const DxStrRef& name) const;

        [[nodiscard]] DxByteSpan instructions() const;

        [[nodiscard]] inline int cacheID() const
        { return m_currentCacheID; }

        [[maybe_unused]] void loadTranslationFile(const DxStrRef& filename);

        static DxData fromFile(const DxStrRef& filename);
    };
}

#endif //LIBDIANNEX_DXDATA_HPP
