
#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "Util/Error/Error.hpp"
#include "zlib.h"

namespace Infinity {
  class ZipExtractor {
public:
    explicit ZipExtractor(const std::string &zip_file_path);
    bool Extract(const std::string &output_file_path);
    bool RemoveArchive() const;
    std::string GetArchivePath() const;

private:
    bool ExtractFile(gzFile file, const std::string &output_path);

private:
    std::string m_Path;
    std::vector<uint8_t> m_Buffer;
    static constexpr size_t CHUNK_SIZE = 8192;
  };
}  // namespace Infinity
