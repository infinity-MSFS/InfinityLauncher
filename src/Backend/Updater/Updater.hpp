
#pragma once
#include <string>
#include <fstream>
#ifdef WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <linux/limits.h>
#endif


namespace Infinity
{
    class Updater
    {
    public:
        static void WritePidToFile(const std::string& path);
        static void LaunchUpdater(const std::string& updater_path, const std::string& temp_exe_path,
                                  const std::string& current_exe_path, const std::string& pid_file_path);
        static std::string GetConfigDir();
        static std::string GetCurrentExecutablePath();

        // static void DownloadExe(const std::string& path, const std::string& url);
    };
}
