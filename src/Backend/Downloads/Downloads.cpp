
#include "Downloads.hpp"

namespace Infinity {
    Downloads *Downloads::m_Instance = nullptr;
    std::mutex Downloads::m_DownloadsMutex;

    Downloads &Downloads::GetInstance() {
        if (!m_Instance) {
            std::lock_guard lock(m_DownloadsMutex);
            if (!m_Instance) {
                m_Instance = new Downloads();
            }
        }
        return *m_Instance;
    }

    void Downloads::Init(const int thread_number) {
        zoe::Zoe::GlobalInit();
        m_ThreadNumber = thread_number;
    }

    void Downloads::Cleanup() {
        std::lock_guard lock(m_DownloadsMutex);
        m_DownloadsMap.clear();
        zoe::Zoe::GlobalUnInit();
    }

    int Downloads::StartDownload(const std::string &url, const std::string &local_path) {
        std::lock_guard lock(m_Mutex);
        int id = m_NextDownloadID++;


        auto &download = m_DownloadsMap[id];
        download.id = id;
        download.url = url;
        download.local_path = local_path;
        download.paused = false;
        download.completed = false;
        download.progress = 0.0;
        download.size = 0;
        download.error = 0;

        download.future = download.zoe->start(
                zoe::utf8string(url), zoe::utf8string(local_path), [this, id](const zoe::Result result) {
                    std::lock_guard lock(m_Mutex);
                    if (const auto it = m_DownloadsMap.find(id); it != m_DownloadsMap.end()) {
                        it->second.completed = true;
                        // switch (result) {
                        //     case zoe::Result::CANCELED:
                        //         it->second.error = zoe::Result::CANCELED;
                        // } TODO: something more advanced like this to actually make use of the errors
                        if (result != zoe::Result::SUCCESSED) {
                            it->second.error = result;
                        }
                    }
                }, [this, id](const int64_t total, const int64_t downloaded) {
                    std::lock_guard lock(m_Mutex);
                    if (const auto it = m_DownloadsMap.find(id); it != m_DownloadsMap.end()) {
                        it->second.progress = total > 0 ? static_cast<float>(downloaded) / static_cast<float>(total) : 0.0f;
                        it->second.size = total > 0 ? total : 0.0;
                    }
                }, [this, id](const int64_t byte_per_sec) {
                    std::lock_guard lock(m_Mutex);
                    if (const auto it = m_DownloadsMap.find(id); it != m_DownloadsMap.end()) {
                        it->second.speed = byte_per_sec;
                    }
                });

        return id;
    }

    void Downloads::PauseDownload(const int id) {
        std::lock_guard lock(m_Mutex);
        if (const auto it = m_DownloadsMap.find(id); it != m_DownloadsMap.end()) {
            it->second.zoe->pause();
        }
    }

    void Downloads::ResumeDownload(const int id) {
        std::lock_guard lock(m_Mutex);
        if (const auto it = m_DownloadsMap.find(id); it != m_DownloadsMap.end()) {
            it->second.zoe->resume();
        }
    }

    void Downloads::StopDownload(const int id) {
        std::lock_guard lock(m_Mutex);
        if (const auto it = m_DownloadsMap.find(id); it != m_DownloadsMap.end()) {
            it->second.zoe->stop();
        }
    }

    void Downloads::PauseAllDownloads() {
        std::lock_guard lock(m_Mutex);
        for (auto it = m_DownloadsMap.begin(); it != m_DownloadsMap.end(); ++it) {
            it->second.zoe->pause();
        }
    }

    void Downloads::ResumeAllDownloads() {
        std::lock_guard lock(m_Mutex);
        for (auto it = m_DownloadsMap.begin(); it != m_DownloadsMap.end(); ++it) {
            it->second.zoe->resume();
        }
    }

    void Downloads::StopAllDownloads() {
        std::lock_guard lock(m_Mutex);
        for (auto it = m_DownloadsMap.begin(); it != m_DownloadsMap.end(); ++it) {
            it->second.zoe->stop();
        }
    }

    Downloads::DownloadData *Downloads::GetDownloadData(const int id) {
        std::lock_guard lock(m_Mutex);
        if (const auto it = m_DownloadsMap.find(id); it != m_DownloadsMap.end()) {
            return &it->second;
        }
        return nullptr;
    }

    std::map<int, Downloads::DownloadData> *Downloads::GetAllDownloads() {
        return &m_DownloadsMap;
    }

}
