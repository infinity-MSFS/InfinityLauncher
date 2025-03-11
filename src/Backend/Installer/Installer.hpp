
#pragma once
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "Backend/Downloads/Downloads.hpp"
#include "Backend/ZipExtractor/ZipExtractor.hpp"
#include "Util/Error/Error.hpp"
#include "Util/GroupUtil/GroupUtil.hpp"

namespace Infinity {
  class Installer {
public:
    static Installer &GetInstance();
    void SetDownloadDir(const std::string &download_dir);

    /**
     * Start a new download
     * @param url std::string
     * @param download_spec Group enum
     *
     * The download pointer can be obtained by first calling
     * GetActiveDownloadFromEnum() and then using that ID to pass to the
     * downloader singleton
     */
    void PushDownload(const std::string &url,
                      const Groups::GroupVariants &download_spec);


    /**
     * Returns the ID for the active download (if it exists)
     * @param download_variant
     * @return `int id` if exists else nullopt
     */
    std::optional<int> GetActiveDownloadFromEnum(
        const Groups::GroupVariants &download_variant);

private:
    static void StartUnzipWatcher(int id, const std::string &file_path);

private:
    static Installer *m_Instance;
    std::string m_DownloadDir;
    std::map<int, Groups::GroupVariants>
        m_GlobalDownloads;  // <int id, GroupVariants name>
    std::mutex m_GlobalDownloadsMutex;
  };
}  // namespace Infinity
