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
#include "DxData.hpp"

#include <zlib.h>

#include "exceptions.hpp"
#include "utils/BinaryReader.hpp"

namespace diannex
{
    DxStrRef DxData::string(size_t idx) const
    { return m_strings.at(idx); }

    DxStrRef DxData::translation(size_t idx) const
    { return m_translations.at(idx); }

    DxScene DxData::scene(const diannex::DxStrRef& name) const
    { return m_scenes.at(name); }

    const DxMap<DxStrRef, DxScene>& DxData::scenes() const
    { return m_scenes; }

    DxMap<DxStrRef, DxScene>& DxData::scenes_mut()
    { return m_scenes; }

    DxROSpan<DxFunction> DxData::functions() const
    { return { m_functions }; }

    DxSpan<DxFunction> DxData::functions_mut()
    { return { m_functions }; }

    DxDefinition DxData::definition(const diannex::DxStrRef& name) const
    { return m_definitions.at(name); }

    DxByteSpan DxData::instructions() const
    { return { m_instructions }; }

    void DxData::loadTranslationFile(const diannex::DxStrRef& filename)
    {
        auto reader = BinaryFileReader::create(std::ifstream(filename.data(), std::ios::in | std::ios::binary));

        if (reader->read<char>() != 'D' || reader->read<char>() != 'X' || reader->read<char>() != 'T')
            throw data_processing_exception(filename, "Not a Diannex binary translation file: invalid header");

        if (reader->read<uint8_t>() != TranslationFormatVersion)
            throw data_processing_exception(filename,
                                            "Diannex translation binary format version is not compatible with this interpreter");

        auto stringCount = reader->read<uint32_t>();
        if (!m_translations.empty() && stringCount != m_translations.size())
            throw data_processing_exception(filename, "Translation file string count does not match");

        // Load text into a cache, so it can be potentially reloaded later
        if (!m_originalText)
            m_originalText = std::move(std::exchange(m_translations, {}));

        for (int _ = 0; _ < stringCount; ++_)
            m_translations.emplace_back(std::move(reader->read<std::string>()));

        m_currentCacheID++;
    }

    DxData DxData::fromFile(const diannex::DxStrRef& filename)
    {
        auto reader = BinaryFileReader::create(std::ifstream(filename.data(), std::ios::in | std::ios::binary));

        if (reader->read<char>() != 'D' || reader->read<char>() != 'N' || reader->read<char>() != 'X')
            throw data_processing_exception(filename, "Not a Diannex binary file (invalid header)");

        if (reader->read<unsigned char>() != FormatVersion)
            throw data_processing_exception(filename,
                                            "Diannex binary format version is not compatible with this interpreter");

        auto flags = reader->read<uint8_t>();
        bool flagCompressed = (flags & 1) != 0;
        bool flagInternalTranslation = (flags & (1 << 1)) != 0;

        // Up here so it stays in scope
        std::vector<std::byte> uncompressedData;
        if (flagCompressed)
        {
            uLongf uncompressedSize = reader->read<uint32_t>();
            auto size = reader->read<uint32_t>();
            DxVec<std::byte> tempBuff(size);
            reader->read_n(size, tempBuff.data());

            uncompressedData.resize(uncompressedSize);
            if (uncompress((Bytef*)uncompressedData.data(), &uncompressedSize, (const Bytef*)tempBuff.data(), size) !=
                Z_OK)
                throw data_processing_exception(filename, "Diannex binary decompression failed");

            reader = BinarySpanReader::create(uncompressedData);
        }
        else
        {
            reader->skip(4);
        }

        auto sceneBlock = reader->read_block();
        auto funcBlock = reader->read_block();
        auto defBlock = reader->read_block();

        DxData data;
        data.m_instructions = reader->read_block();

        reader->skip(4); // Ignore size; we're going to process this now
        auto stringCount = reader->read<uint32_t>();
        data.m_strings.reserve(stringCount);

        for (int i = 0; i < stringCount; ++i)
            data.m_strings.emplace_back(std::move(reader->read<std::string>()));

        if (flagInternalTranslation)
        {
            reader->skip(4); // Ignore size; we're going to process this now

            auto translationCount = reader->read<uint32_t>();
            data.m_translations.reserve(translationCount);

            for (int i = 0; i < translationCount; ++i)
                data.m_translations.emplace_back(std::move(reader->read<std::string>()));
        }

        [[maybe_unused]] auto externalFunctionBlock = reader->read_block();

        // Parse scene data
        reader = BinarySpanReader::create(sceneBlock);
        auto sceneCount = reader->read<uint32_t>();
        data.m_scenes.reserve(sceneCount);
        for (int _1 = 0; _1 < sceneCount; ++_1)
        {
            auto sceneName = data.string(reader->read<uint32_t>());
            auto flagCount = reader->read<uint16_t>() - 1;
            auto codeOffset = reader->read<int32_t>();
            std::vector<int> flagOffsets;
            std::vector<DxStr> flagNames;
            if (flagCount != 0)
            {
                flagOffsets.reserve(flagCount);
                for (int _2 = 0; _2 < flagCount; ++_2)
                    flagOffsets.push_back(reader->read<int32_t>());
                flagNames.resize(flagCount / 2);
            }
            data.m_scenes
                .emplace(std::make_pair(sceneName,
                                        DxScene(sceneName, codeOffset, std::move(flagOffsets), std::move(flagNames))));
        }

        // Parse function data
        reader = BinarySpanReader::create(funcBlock);
        auto funcCount = reader->read<uint32_t>();
        data.m_functions.reserve(funcCount);
        for (int _1 = 0; _1 < funcCount; ++_1)
        {
            auto funcName = data.string(reader->read<uint32_t>());
            auto flagCount = reader->read<uint16_t>() - 1;
            auto codeOffset = reader->read<int32_t>();
            std::vector<int> flagOffsets;
            std::vector<DxStr> flagNames;
            if (flagCount != 0)
            {
                flagOffsets.reserve(flagCount);
                for (int _2 = 0; _2 < flagCount; ++_2)
                    flagOffsets.push_back(reader->read<int32_t>());
                flagNames.resize(flagCount / 2);
            }
            data.m_functions.emplace_back(funcName, codeOffset, std::move(flagOffsets), std::move(flagNames));
        }

        // Parse definition data
        reader = BinarySpanReader::create(defBlock);
        auto defCount = reader->read<uint32_t>();
        data.m_definitions.reserve(defCount);
        for (int _ = 0; _ < defCount; ++_)
        {
            auto defName = data.string(reader->read<uint32_t>());
            auto valueStringIndex = reader->read<uint32_t>();
            auto codeOffset = reader->read<int32_t>();
            auto isInternal = false;

            if (valueStringIndex & (1 << 31))
            {
                isInternal = true;
                valueStringIndex &= ~(1 << 31);
            }

            data.m_definitions.emplace(std::make_pair(defName, DxDefinition(valueStringIndex, codeOffset, isInternal)));
        }

        data.m_currentCacheID++;

        return data;
    }
}