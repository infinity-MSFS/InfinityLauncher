#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <cstdlib>
#ifdef WIN32
#include <Windows.h>
#else
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#endif


// TODO: Implement in launcher
// #include <fstream>
// #include <string>
// #ifdef _WIN32
// #include <windows.h>
// #else
// #include <unistd.h>
// #endif
//
// void WritePidToFile(const std::string &pid_file_path) {
//     std::ofstream pid_file(pid_file_path);
//     if (pid_file.is_open()) {
// #ifdef _WIN32
//         pid_file << GetCurrentProcessId();
// #else
//         pid_file << getpid();
// #endif
//         pid_file.close();
//     }
// }
// void LaunchUpdater(const std::string &updaterPath, const std::string &tempExePath, const std::string &currentExePath, const std::string &pidFilePath) {
//     std::string command;
// #ifdef _WIN32
//     command = "start \"\" \"" + updaterPath + "\" \"" + tempExePath + "\" \"" + currentExePath + "\" \"" + pidFilePath + "\"";
// #else
//     command = "\"" + updaterPath + "\" \"" + tempExePath + "\" \"" + currentExePath + "\" \"" + pidFilePath + "\" &";
// #endif
//
//     std::system(command.c_str());
// }


bool IsProcessRunning(const int pid) {
#ifdef WIN32
    if (const HANDLE h_process = OpenProcess(SYNCHRONIZE, FALSE, pid)) {
        DWORD exitCode = 0;
        GetExitCodeProcess(h_process, &exitCode);
        CloseHandle(h_process);
        return exitCode == STILL_ACTIVE;
    }
    return false;
#else
    return (kill(pid, 0) == 0);
#endif
}

void WaitForProcess(const int pid) {
    while (IsProcessRunning(pid)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

int main(const int argc, char **argv) {
    if (argc < 4) {
        std::cerr << "Usage: Updater <tempExePath> <currentExePath> <pidFilePath>\n";
        return 1;
    }

    const std::string temp_exe_path = argv[1];
    const std::string current_exe_path = argv[2];
    const std::string pid_file_path = argv[3];

    int pid;
    try {
        std::ifstream pid_file(pid_file_path);
        if (!pid_file.is_open()) {
            throw std::runtime_error("Failed to open pid file");
        }
        pid_file >> pid;
        pid_file.close();
    } catch (std::exception &e) {
        std::cerr << "Error reading PID file: " << e.what() << std::endl;
        return 1;
    }

    WaitForProcess(pid);

    try {
        std::filesystem::remove(current_exe_path);
        std::filesystem::rename(temp_exe_path, current_exe_path);

#ifdef _WIN32
        std::system((R"(start "" ")" + current_exe_path + "\"").c_str());
#else
        std::system(("\"" + current_exe_path + "\" &").c_str());
#endif
    } catch (std::filesystem::filesystem_error &e) {
        std::cerr << "Error updating: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
