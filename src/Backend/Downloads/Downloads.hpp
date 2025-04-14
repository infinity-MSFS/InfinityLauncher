
#pragma once

#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "zoe/zoe.h"

namespace Infinity {

  class Downloads {
public:
    struct DownloadData {
      int id;
      std::string url;
      std::string local_path;
      bool paused;
      float progress;
      bool completed;
      int error;
      int64_t speed;
      int64_t size;
      std::shared_future<zoe::Result> future;
      std::shared_ptr<zoe::Zoe> zoe;

      DownloadData()
          : id(0)
          , paused(false)
          , progress(0.0f)
          , completed(false)
          , error(-1)
          , speed(0)
          , size(0)
          , zoe(std::make_shared<zoe::Zoe>()) {}

      DownloadData(DownloadData &&) = default;
      DownloadData &operator=(DownloadData &&) = default;

      DownloadData(const DownloadData &) = delete;
      DownloadData &operator=(const DownloadData &) = delete;
    };

    static Downloads &GetInstance();
    int StartDownload(const std::string &url, const std::string &local_path);
    DownloadData *GetDownloadData(int id);
    void PauseDownload(int id);
    void ResumeDownload(int id);
    void StopDownload(int id);
    void PauseAllDownloads();
    void ResumeAllDownloads();
    void StopAllDownloads();
    void RemoveDownload(int id);
    std::map<int, DownloadData> *GetAllDownloads();
    void SetMaxSpeed(int64_t speed);
    void SetDiskCacheSize(int64_t size);

    Downloads(const Downloads &) = delete;
    Downloads &operator=(const Downloads &) = delete;

private:
    Downloads()
        : m_next_download_id(1)
        , m_thread_number(19) {}

    ~Downloads() { Cleanup(); }

    void Cleanup();
    void Init(int thread_number = 10);

private:
    std::map<int, DownloadData> m_downloads_map;
    static Downloads *m_instance;
    static std::mutex m_downloads_mutex;
    std::mutex m_mutex;
    int m_next_download_id;
    int m_thread_number;
  };
}  // namespace Infinity
