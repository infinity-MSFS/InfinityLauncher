#pragma once

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "openssl/sha.h"

#ifdef WIN32
#include <Windows.h>
#include <comdef.h>
#include <wbemidl.h>
#elif defined(__linux__)
#include <cstdlib>
#include <cstring>
#include <fstream>
#endif


// Effective HWIDs for apps of this nature (were we are trying to eliminate
// duplicates due to similar hardware) include:
// - CPU instruction set
// - Motherboard serial number
// - Graphics card details
// After hashing these values together, we should theoretically have a unique
// HWID for each machine that is challenging enough to spoof for our purposes.
// Note: Using a very inclusive method like this will likely result in loss of
// access to the app if the user upgrades their hardware. This is acceptable
// since it's only for beta access, and we can always reset the HWID if needed.


class HWID {
  public:
  HWID() = default;
  ~HWID() = default;

  std::string GetHWID();

  private:
  static std::string GetCPUInfo();
  static std::string GetMotherboardSerial();
  static std::string GetGPUInfo();

  static std::string Hash(const std::string& input);

  static std::string exec(const char* cmd);  // helper to execute shell commands (Linux only)


  // TODO: maybe store hwid in a singleton to avoid repeated system calls
};
