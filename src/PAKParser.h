#pragma once

#include "BinReader.h"

#include <filesystem>
#include <vector>

struct PAKFileEntry
{
    uint32_t offset, oldOffset;
    int32_t size;
    int32_t hOffset, hLen, hRef;
    std::string partialName = "";

    PAKFileEntry(BinReader& binReader);
    void fillIn(const std::vector<char>& nameBlock);
    bool isRealFile() const;
};

class PAKParser
{
public:
    static PAKParser& instance(); // TODO: Remove

    PAKParser();
    PAKParser(std::filesystem::path path);
    ~PAKParser();

    void tryExtract(const std::filesystem::path& innerPath);

private:
    void extract(const PAKFileEntry& entry, const std::filesystem::path& outputPath) const;

    const PAKFileEntry* findFile(std::string innerPath) const;
    const PAKFileEntry* findFile(std::string innerPathLeft, std::string innerPathPassed, int offset) const;

    void parse();
    size_t fileSize() const;

    std::vector<PAKFileEntry> m_entries;

    BinReader m_binReader;
};
