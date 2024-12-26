# Updater

The updater module allows for self updating functionality in the **Infinity Launcher**

## Usage

1. Prepare the following from inside the app:
    1. `newExePath` - Path to the new executable
    2. `oldExePath` - Path to the current executable
    3. `pidFilePath` - Get current PID and write to file
        1. ```c++
           // Example
           void WritePidToFile(const std::string &pid_file_path) {
                std::ofstream pid_file(pid_file_path);
                if (pid_file.is_open()) {
           #ifdef _WIN32
                pid_file << GetCurrentProcessId();
           #else
                pid_file << getpid();
           #endif
                pid_file.close();
                }
           }
           ```

2. Call the executable `Updater "tempExePath" "currentExePath" "pidFilePath"`
    1. Its very important that your command is not associated with the current program. The updater will end up ending
       itself if this is the case.
    2. ```c++
       // Example: 
       void LaunchUpdater(const std::string &updaterPath, const std::string &tempExePath, const std::string &currentExePath, const std::string &pidFilePath) {
           std::string command;
       #ifdef _WIN32
           command = "start \"\" \"" + updaterPath + "\" \"" + tempExePath + "\" \"" + currentExePath + "\" \"" + pidFilePath + "\"";
       #else
           command = "\"" + updaterPath + "\" \"" + tempExePath + "\" \"" + currentExePath + "\" \"" + pidFilePath + "\" &";
       #endif
      
           std::system(command.c_str());
       }
       ```

## TODO:

1. Implement a UI (just a simple status window that matches the theme of the launcher will do)
2. Maybe implement a shared lib for properly spawning the updater