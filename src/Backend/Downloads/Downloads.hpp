
#pragma once

#include "zoe/zoe.h"

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <mutex>
#include <future>

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
            std::unique_ptr<zoe::Zoe> zoe;

            DownloadData():
                id(0), paused(false), progress(0.0f), completed(false), error(-1), speed(0), size(0), zoe(std::make_unique<zoe::Zoe>()) {
            }

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
        std::map<int, DownloadData> *GetAllDownloads();
        void SetMaxSpeed(int64_t speed);
        void SetDiskCacheSize(int64_t size);

    private:
        Downloads() :
            m_NextDownloadID(1),
            m_ThreadNumber(19) {
        }

        ~Downloads() {
            Cleanup();
        }

        Downloads(const Downloads &) = delete;
        Downloads &operator=(const Downloads &) = delete;


        void Cleanup();
        void Init(int thread_number = 10);

    private:
        std::map<int, DownloadData> m_DownloadsMap;
        static Downloads *m_Instance;
        static std::mutex m_DownloadsMutex;
        std::mutex m_Mutex;
        int m_NextDownloadID;
        int m_ThreadNumber;


    };
}
