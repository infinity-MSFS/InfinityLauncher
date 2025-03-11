#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
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
// void LaunchUpdater(const std::string &updaterPath, const std::string
// &tempExePath, const std::string &currentExePath, const std::string
// &pidFilePath) {
//     std::string command;
// #ifdef _WIN32
//     command = "start \"\" \"" + updaterPath + "\" \"" + tempExePath + "\" \""
//     + currentExePath + "\" \"" + pidFilePath + "\"";
// #else
//     command = "\"" + updaterPath + "\" \"" + tempExePath + "\" \"" +
//     currentExePath + "\" \"" + pidFilePath + "\" &";
// #endif
//
//     std::system(command.c_str());
// }

bool IsProcessRunning(const int pid) {
#ifdef WIN32
  // Open process with more rights
  HANDLE h_process = OpenProcess(
      PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE, FALSE, pid);
  if (h_process == NULL) {
    DWORD error = GetLastError();
    if (error == ERROR_INVALID_PARAMETER || error == ERROR_ACCESS_DENIED) {
      std::cout << "Process " << pid << " not found or access denied"
                << std::endl;
      return false;
    }
    std::cout << "Error opening process: " << error << std::endl;
    return false;
  }

  DWORD exitCode;
  bool success = GetExitCodeProcess(h_process, &exitCode);
  CloseHandle(h_process);

  if (!success) {
    std::cout << "Failed to get process exit code" << std::endl;
    return false;
  }

  return exitCode == STILL_ACTIVE;
#else
  return (kill(pid, 0) == 0);
#endif
}

bool ForceTerminateProcess(const int pid) {
#ifdef WIN32
  HANDLE h_process = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
  if (h_process == NULL) {
    std::cout << "Could not open process for termination" << std::endl;
    return false;
  }

  bool result = TerminateProcess(h_process, 1) != 0;
  CloseHandle(h_process);
  return result;
#else
  return kill(pid, SIGTERM) == 0;
#endif
}

void WaitForProcess(const int pid) {
  int attempts = 0;
  constexpr int max_attempts = 20;

  while (IsProcessRunning(pid) && attempts < max_attempts) {
    std::cout << "Waiting for process " << pid << " to exit... (attempt "
              << attempts + 1 << "/" << max_attempts << ")" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    attempts++;
  }

  if (attempts >= max_attempts) {
    std::cout << "Process didn't exit naturally, attempting to force close..."
              << std::endl;
    if (ForceTerminateProcess(pid)) {
      std::cout << "Process terminated successfully" << std::endl;
      // Give it a moment to fully terminate
      std::this_thread::sleep_for(std::chrono::seconds(1));
    } else {
      std::cout << "Failed to terminate process" << std::endl;
    }
  }
}

int main(const int argc, char **argv) {
  try {
    if (argc < 4) {
      throw std::runtime_error(
          "Usage: Updater <tempExePath> <currentExePath> <pidFilePath>");
    }

    const std::string temp_exe_path = argv[1];
    const std::string current_exe_path = argv[2];
    const std::string pid_file_path = argv[3];

    std::cout << "Starting update process..." << std::endl;
    std::cout << "Temp exe path: " << temp_exe_path << std::endl;
    std::cout << "Current exe path: " << current_exe_path << std::endl;
    std::cout << "PID file path: " << pid_file_path << std::endl;

    int pid;
    try {
      std::ifstream pid_file(pid_file_path);
      if (!pid_file.is_open()) {
        throw std::runtime_error("Failed to open pid file: " + pid_file_path);
      }
      pid_file >> pid;
      pid_file.close();
      std::cout << "Read PID: " << pid << std::endl;
    } catch (const std::exception &e) {
      throw std::runtime_error(std::string("Error reading PID file: ") +
                               e.what());
    }

    std::cout << "Waiting for process to end..." << std::endl;
    WaitForProcess(pid);
    std::cout << "Process ended, starting update..." << std::endl;

    try {
      if (!std::filesystem::exists(temp_exe_path)) {
        throw std::runtime_error("Temp exe does not exist: " + temp_exe_path);
      }

      if (!std::filesystem::exists(current_exe_path)) {
        throw std::runtime_error("Current exe does not exist: " +
                                 current_exe_path);
      }

      std::filesystem::remove(current_exe_path);
      std::cout << "Removed old exe" << std::endl;

      std::filesystem::rename(temp_exe_path, current_exe_path);
      std::cout << "Renamed new exe" << std::endl;

#ifdef _WIN32
      std::cout << "Launching updated executable..." << std::endl;
      std::system((R"(start "" ")" + current_exe_path + "\"").c_str());
#else
      std::cout << "Launching updated executable..." << std::endl;
      std::system(("\"" + current_exe_path + "\" &").c_str());
#endif
      std::cout << "Update completed successfully!" << std::endl;
    } catch (const std::filesystem::filesystem_error &e) {
      throw std::runtime_error(std::string("Filesystem error during update: ") +
                               e.what());
    }

    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();
    return 0;

  } catch (const std::exception &e) {
    std::cerr << "\nError: " << e.what() << std::endl;
    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();
    return 1;
  }
}
