
#include "Installer.hpp"

namespace Infinity {
    Installer &Installer::GetInstance() {
        static Installer instance;
        return instance;
    }

    void Installer::SetDownloadDir(const std::string &download_dir) {
        m_DownloadDir = download_dir;
    }


    void Installer::PushDownload(const std::string &url, const Groups::GroupVariants &download_spec) {
        auto &downloader = Downloads::GetInstance();
        if (m_DownloadDir.empty())
            m_DownloadDir = "C:\\test\\Infinity\\Downloads\\";
        std::lock_guard lock(m_GlobalDownloadsMutex);
        for (auto it = m_GlobalDownloads.begin(); it != m_GlobalDownloads.end(); ++it) {
            if (it->second == download_spec) {
                // we have already started a download for this product previously
                m_GlobalDownloads.erase(it);
            }
        }
        auto id = downloader.StartDownload(url, m_DownloadDir);
        m_GlobalDownloads.insert({id, download_spec});

        StartUnzipWatcher(id, m_DownloadDir);
    }

    std::optional<int> Installer::GetActiveDownloadFromEnum(const Groups::GroupVariants &download_variant) {
        std::lock_guard lock(m_GlobalDownloadsMutex);
        for (const auto it = m_GlobalDownloads.begin(); it != m_GlobalDownloads.end();) {
            if (it->second == download_variant) {
                return it->first;
            }
        }
        return std::nullopt;
    }

    void Installer::StartUnzipWatcher(int id, const std::string &file_path) {
        std::thread([id, file_path] {
            while (true) {
                {
                    auto &downloader = Downloads::GetInstance();
                    auto *data = downloader.GetDownloadData(id);
                    if (data->completed) {
                        auto trim_path = [](std::string &path) {
                            if (path.ends_with(".zip")) {
                                path.erase(path.size() - 4);
                            }
                        };
                        ZipExtractor extractor(file_path);
                        std::string output_path(file_path);
                        trim_path(output_path);
                        extractor.Extract(output_path);
                        if (!extractor.RemoveArchive()) {
                            Errors::Error(Errors::ErrorType::Warning, "Failed to remove archive file: " + extractor.GetArchivePath()).Dispatch();
                        }
                        break;
                    }
                }


                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }).detach();
    }


}