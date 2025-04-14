
#include "Downloads.hpp"

#include <iostream>
#include <ranges>

namespace Infinity {
  Downloads *Downloads::m_instance = nullptr;
  std::mutex Downloads::m_downloads_mutex;

  Downloads &Downloads::GetInstance() {
    if (!m_instance) {
      std::lock_guard lock(m_downloads_mutex);
      if (!m_instance) {
        m_instance = new Downloads();
      }
    }
    return *m_instance;
  }

  void Downloads::Init(const int thread_number) {
    zoe::Zoe::GlobalInit();
    m_thread_number = thread_number;
  }

  void Downloads::Cleanup() {
    std::lock_guard lock(m_downloads_mutex);
    m_downloads_map.clear();
    zoe::Zoe::GlobalUnInit();
  }

  int Downloads::StartDownload(const std::string &url, const std::string &local_path) {
    std::lock_guard lock(m_mutex);
    int id = m_next_download_id++;


    auto &download = m_downloads_map[id];
    download.id = id;
    download.url = url;
    download.local_path = local_path;
    download.paused = false;
    download.completed = false;
    download.progress = 0.0;
    download.size = 0;
    download.error = 0;
    download.zoe = std::make_shared<zoe::Zoe>();

    download.future = download.zoe->start(
        zoe::utf8string(url), zoe::utf8string(local_path),
        [this, id](const zoe::Result result) {
          std::lock_guard lock(m_mutex);
          if (const auto it = m_downloads_map.find(id); it != m_downloads_map.end()) {
            it->second.completed = true;
            // switch (result) {
            //     case zoe::Result::CANCELED:
            //         it->second.error = zoe::Result::CANCELED;
            // } TODO: something more advanced like this to actually make use of
            // the errors
            std::cout << "Download result: " << result << std::endl;
            if (result != zoe::Result::SUCCESSED) {
              it->second.error = result;
            }
          }
        },
        [this, id](const int64_t total, const int64_t downloaded) {
          std::lock_guard lock(m_mutex);
          if (const auto it = m_downloads_map.find(id); it != m_downloads_map.end()) {
            it->second.progress = total > 0 ? static_cast<float>(downloaded) / static_cast<float>(total) : 0.0f;
            it->second.size = total > 0 ? total : 0;
          }
        },
        [this, id](const int64_t byte_per_sec) {
          std::lock_guard lock(m_mutex);
          if (const auto it = m_downloads_map.find(id); it != m_downloads_map.end()) {
            it->second.speed = byte_per_sec;
          }
        });

    return id;
  }

  void Downloads::PauseDownload(const int id) {
    std::lock_guard lock(m_mutex);
    if (const auto it = m_downloads_map.find(id); it != m_downloads_map.end()) {
      it->second.zoe->pause();
    }
  }

  void Downloads::ResumeDownload(const int id) {
    std::lock_guard lock(m_mutex);
    if (const auto it = m_downloads_map.find(id); it != m_downloads_map.end()) {
      it->second.zoe->resume();
    }
  }

  void Downloads::StopDownload(const int id) {
    std::lock_guard lock(m_mutex);
    if (const auto it = m_downloads_map.find(id); it != m_downloads_map.end()) {
      it->second.zoe->stop();
    }
  }

  void Downloads::PauseAllDownloads() {
    std::lock_guard lock(m_mutex);
    for (const auto &data: std::views::values(m_downloads_map)) {
      data.zoe->pause();
    }
  }

  void Downloads::ResumeAllDownloads() {
    std::lock_guard lock(m_mutex);
    for (const auto &data: std::views::values(m_downloads_map)) {
      data.zoe->resume();
    }
  }

  void Downloads::StopAllDownloads() {
    std::lock_guard lock(m_mutex);
    for (const auto &data: std::views::values(m_downloads_map)) {
      data.zoe->stop();
    }
  }

  Downloads::DownloadData *Downloads::GetDownloadData(const int id) {
    std::lock_guard lock(m_mutex);
    if (const auto it = m_downloads_map.find(id); it != m_downloads_map.end()) {
      return &it->second;
    }
    return nullptr;
  }

  std::map<int, Downloads::DownloadData> *Downloads::GetAllDownloads() { return &m_downloads_map; }

  void Downloads::RemoveDownload(const int id) {
    std::shared_ptr<zoe::Zoe> zoe;
    {
      std::lock_guard lock(m_mutex);
      const auto it = m_downloads_map.find(id);
      if (it == m_downloads_map.end()) return;
      zoe = it->second.zoe;
      m_downloads_map.erase(it);
    }
    if (zoe) {
      zoe->stop();
    }
  }


}  // namespace Infinity
