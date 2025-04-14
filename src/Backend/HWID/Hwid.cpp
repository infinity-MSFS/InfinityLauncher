#include "Hwid.hpp"


#ifdef WIN32
#include <atlbase.h>
#include <atlconv.h>
#include <intrin.h>
#else
#include <x86intrin.h>
#endif


std::string HWID::GetHWID() {
  const std::string hwid = GetCPUInfo() + GetMotherboardSerial() + GetGPUInfo();
  return Hash(hwid);
}

std::string HWID::GetCPUInfo() {
  std::string cpu_info;
#if defined(_WIN32)
  int cpu_info_regs[4] = {0};
  __cpuid(cpu_info_regs, 0);
  std::stringstream ss;
  for (int i = 0; i < 4; ++i) {
    ss << std::hex << cpu_info_regs[i];
  }
  cpu_info = ss.str();
#elif defined(__linux__)
  std::ifstream cpu_file("/proc/cpuinfo");
  if (!cpu_file) {
    cpu_info = "Unknown";
  } else {
    std::string line;
    while (std::getline(cpu_file, line)) {
      if (line.find("model name") != std::string::npos) {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
          cpu_info = line.substr(colonPos + 1);
          size_t start = cpu_info.find_first_not_of(" \t");
          size_t end = cpu_info.find_last_not_of(" \t");
          cpu_info = cpu_info.substr(start, end - start + 1);
        }
        break;
      }
    }
  }
#endif
  return cpu_info;
}


std::string HWID::GetMotherboardSerial() {
  std::string serial;
#if defined(_WIN32)
  IWbemLocator *pLoc = nullptr;
  IWbemServices *pSvc = nullptr;
  HRESULT hres;

  hres = CoInitializeEx(0, COINIT_MULTITHREADED);
  if (FAILED(hres)) {
    return "Unknown";
  }

  hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                              EOAC_NONE, NULL);
  if (FAILED(hres)) {
    return "Unknown";
  }

  hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) &pLoc);
  if (FAILED(hres)) {
    return "Unknown";
  }

  hres = pLoc->ConnectServer(bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
  if (FAILED(hres)) {
    return "Unknown";
  }

  IEnumWbemClassObject *pEnumerator = nullptr;
  hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_BaseBoard"),
                         WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
  if (FAILED(hres)) {
    return "Unknown";
  }

  IWbemClassObject *pclsObj = nullptr;
  ULONG uReturn = 0;
  while (pEnumerator) {
    hres = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
    if (0 == uReturn) break;

    VARIANT vtProp;
    hres = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
    serial = vtProp.bstrVal ? CW2A(vtProp.bstrVal) : "Unknown";
    VariantClear(&vtProp);
    pclsObj->Release();
  }
#elif defined(__linux__)
  std::ifstream serial_file("/var/run/motherboard-serial");
  if (serial_file.is_open()) {
    std::getline(serial_file, serial);
    if (serial.empty()) {
      serial = "Unknown";
    }
  } else {
    serial = "Unknown";
  }
  // TODO: Instead of returning a simple unknown, we should ask the user to run
  // the ./setup_beta_env.sh script, we also need to parse the HWIDs to make
  // sure that when whitelisting them, they arent from users with "unknown"
  // hardware
#endif
  return serial;
}

std::string HWID::GetGPUInfo() {
  std::string gpu_info;
#if defined(_WIN32)
  IWbemLocator *p_loc = nullptr;
  IWbemServices *p_svc = nullptr;
  HRESULT hres;

  hres = CoInitializeEx(0, COINIT_MULTITHREADED);
  if (FAILED(hres)) {
    return "Unknown";
  }

  hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) &p_loc);
  if (FAILED(hres)) {
    return "Unknown";
  }

  hres = p_loc->ConnectServer(bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &p_svc);
  if (FAILED(hres)) {
    return "Unknown";
  }

  IEnumWbemClassObject *p_enumerator = nullptr;
  hres = p_svc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_VideoController"),
                          WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &p_enumerator);
  if (FAILED(hres)) {
    return "Unknown";
  }

  IWbemClassObject *pcls_obj = nullptr;
  ULONG u_return = 0;
  while (p_enumerator) {
    hres = p_enumerator->Next(WBEM_INFINITE, 1, &pcls_obj, &u_return);
    if (0 == u_return) break;

    VARIANT variant_prop;
    hres = pcls_obj->Get(L"Name", 0, &variant_prop, 0, 0);
    gpu_info = variant_prop.bstrVal ? CW2A(variant_prop.bstrVal) : "Unknown";
    VariantClear(&variant_prop);
    pcls_obj->Release();
  }
#elif defined(__linux__)
  gpu_info = exec("lspci | grep VGA");

  if (!gpu_info.empty() && gpu_info[gpu_info.length() - 1] == '\n') {
    gpu_info.erase(gpu_info.length() - 1);
  }
#endif
  return gpu_info;
}

std::string HWID::Hash(const std::string &input) {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256_ctx;
  SHA256_Init(&sha256_ctx);
  SHA256_Update(&sha256_ctx, input.c_str(), input.length());
  SHA256_Final(hash, &sha256_ctx);

  std::stringstream ss;
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    ss << std::hex << std::setw(2) << std::setfill('0') << (int) hash[i];
  }
  return ss.str();
}


std::string HWID::exec(const char *cmd) {
#if defined(__linux__)
  std::array<char, 128> buffer{};
  std::string result;
  FILE *pipe = popen(cmd, "r");
  if (!pipe) return "Unknown";
  while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
    result += buffer.data();
  }
  fclose(pipe);
  return result;
#else
  return "Unknown";
#endif
}
