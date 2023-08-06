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
#ifndef LIBDIANNEX_MODELS_HPP
#define LIBDIANNEX_MODELS_HPP

#include <string_view>
#include <string>
#include <utility>
#include <vector>

namespace diannex
{
    struct DxDefinition
    {
        unsigned int valueStringIndex;
        int codeOffset;
        bool isInternal;

        DxDefinition(unsigned int valueStringIndex, int codeOffset, bool isInternal)
            : valueStringIndex(valueStringIndex), codeOffset(codeOffset), isInternal(isInternal)
        {}
    };

    struct DxFunction
    {
        // Will be used in a future update
        [[maybe_unused]] std::string_view name;
        int codeOffset;
        std::vector<int> flagOffsets;
        std::vector<std::string> flagNames;

        DxFunction(
            std::string_view name,
            int codeOffset,
            std::vector<int> flagOffsets,
            std::vector<std::string> flagNames
        )
            : name(name), codeOffset(codeOffset), flagOffsets(std::move(flagOffsets)), flagNames(std::move(flagNames))
        {}
    };

    struct DxScene
    {
        std::string_view name;
        int codeOffset;
        std::vector<int> flagOffsets;
        std::vector<std::string> flagNames;

        DxScene(std::string_view name, int codeOffset, std::vector<int> flagOffsets, std::vector<std::string> flagNames)
            : name(name), codeOffset(codeOffset), flagOffsets(std::move(flagOffsets)), flagNames(std::move(flagNames))
        {}
    };
}

#endif //LIBDIANNEX_MODELS_HPP
