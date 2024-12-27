
#include "Updater.hpp"
#include <iostream>
#include <filesystem>

namespace Infinity {
    void Updater::WritePidToFile(const std::string &path) {
        std::ofstream pid_file(path);
        if (pid_file.is_open()) {
#ifdef WIN32
            pid_file << GetCurrentProcessId();
#else
            pid_file << getpid();
#endif
            pid_file.close();
        }
    }

    void Updater::LaunchUpdater(const std::string &updater_path, const std::string &temp_exe_path,
                                const std::string &current_exe_path, const std::string &pid_file_path) {
        std::string command;
#ifdef WIN32
        auto convert_path = [](std::string path) {
            std::replace(path.begin(), path.end(), '/', '\\');
            return path;
        };

        std::string win_updater_path = convert_path(updater_path);
        std::string win_temp_exe_path = convert_path(temp_exe_path);
        std::string win_current_exe_path = convert_path(current_exe_path);
        std::string win_pid_file_path = convert_path(pid_file_path);

        command = "start \"\" \"" + win_updater_path + "\" \"" +
                  win_temp_exe_path + "\" \"" +
                  win_current_exe_path + "\" \"" +
                  win_pid_file_path + "\"";
#else
        command = "\"" + updater_path + "\" \"" + temp_exe_path + "\" \"" + current_exe_path +
                  "\" \"" + pid_file_path + "\"; read -p 'Press Enter to continue...'";
#endif

        std::cout << command << std::endl;
        std::system(command.c_str());
    }

    std::string Updater::GetConfigDir() {


        std::filesystem::path output_path;

#ifdef WIN32
        char *appdata_path = nullptr;
        size_t len = 0;
        if (_dupenv_s(&appdata_path, &len, "APPDATA") != 0 || appdata_path == nullptr) {
            std::cerr << "Error: Unable to determine AppData folder (APPDATA environment variable is not set)." << std::endl;
        } else {
            output_path = std::filesystem::path(appdata_path) / "infinity-launcher";
            free(appdata_path);
        }
#elif __linux__
            char *home_dir = nullptr;
            size_t len = 0;
            if (_dupenv_s(&home_dir, &len, "HOME") != 0 || home_dir == nullptr) {
                std::cerr << "Error: Unable to determine home directory (HOME environment variable is not set)." << std::endl;
            } else {
                output_path = std::filesystem::path(home_dir) / ".config/infinity-launcher";
                free(home_dir);
            }
#else
            std::cerr << "Error: Unsupported platform." << std::endl;
#endif
        return output_path.string();

    }

    std::string Updater::GetCurrentExecutablePath() {
        std::string executable_path;

#ifdef WIN32
        char buffer[MAX_PATH];
        GetModuleFileName(nullptr, buffer, MAX_PATH);
        executable_path = std::string(buffer);
#else
        char buffer[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer)-1);
        if (len != -1) {
            buffer[len] = '\0';
            executable_path = std::string(buffer);
        }
#endif
        size_t last_slash = executable_path.find_last_of("/\\");
        if (last_slash != std::string::npos) {
            executable_path = executable_path.substr(0, last_slash);
        }

        return executable_path;
    }
}
