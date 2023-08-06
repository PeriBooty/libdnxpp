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
#ifndef LIBDIANNEX_COMMON_HPP
#define LIBDIANNEX_COMMON_HPP

#include <vector>
#include <unordered_map>
#include <span>
#include <optional>
#include <variant>
#include <any>
#include <memory>
#include <string>
#include <string_view>
#include <sstream>
#include <concepts>
#include <functional>

namespace diannex
{
    template<typename T>
    using DxVec = std::vector<T>;
    using DxByteBuf = DxVec<std::byte>;
    template<typename TKey, typename TVal>
    using DxMap = std::unordered_map<TKey, TVal>;

    template<typename T>
    using DxSpan = std::span<T>;
    template<typename T>
    using DxROSpan = std::span<const T>;
    using DxByteSpan = DxROSpan<std::byte>;

    template<typename T>
    using DxOpt = std::optional<T>;
    template<typename... Types>
    using DxVariant = std::variant<Types...>;
    using DxAny = std::any;
    template<typename Tr>
    using DxFunc = std::function<Tr>;

    template<typename T>
    using DxPtr = std::shared_ptr<T>;
    template<typename T>
    using DxWeakPtr = std::weak_ptr<T>;

    using DxStr = std::string;
    using DxStrRef = std::string_view;
    using DxStrBuilder = std::stringstream;

    template<class T>
    concept DxStrLike =
    requires(const T& val) {{ std::string(val) } -> std::same_as<std::string>; };
}

#endif //LIBDIANNEX_COMMON_HPP
