
#include "ZipExtractor.hpp"

namespace Infinity {
  bool ZipExtractor::ExtractFile(gzFile file, const std::string &output_path) {
    std::ofstream output_file(output_path, std::ios::binary);
    if (!output_file) {
      return false;
    }

    m_buffer.resize(CHUNK_SIZE);
    int bytes_read;

    while ((bytes_read = gzread(file, m_buffer.data(), CHUNK_SIZE)) > 0) {
      output_file.write(reinterpret_cast<char *>(m_buffer.data()), bytes_read);
      if (output_file.bad()) {
        output_file.close();
        return false;
      }
    }
    output_file.close();
    return bytes_read >= 0;
  }

  ZipExtractor::ZipExtractor(const std::string &zip_file_path)
      : m_path(zip_file_path) {
    if (!std::filesystem::exists(zip_file_path)) {
      Errors::Error(Errors::ErrorType::Warning,
                    "ZipExtractor::ZipExtractor(const std::string "
                    "&zip_file_path): File does not exist")
          .Dispatch();
      return;
    }
  }

  bool ZipExtractor::Extract(const std::string &output_file_path) {
    std::filesystem::create_directory(output_file_path);

    gzFile file = gzopen(m_path.c_str(), "rb");
    if (!file) {
      return false;
    }

    std::string extract_path = std::filesystem::path(m_path).string();
    const bool success = ExtractFile(file, output_file_path);

    gzclose(file);
    return success;
  }

  bool ZipExtractor::RemoveArchive() const {
    try {
      return std::filesystem::remove(m_path);
    } catch (const std::filesystem::filesystem_error &e) {
      Errors::Error(Errors::ErrorType::Warning, "Failed to delete archive" + m_path).Dispatch();
      return false;
    }
  }

  std::string ZipExtractor::GetArchivePath() const { return m_path; }


}  // namespace Infinity
