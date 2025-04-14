#pragma once

#include <cctype>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

namespace Infinity::Groups {
  enum class AERO_DYNAMICS {
    DC10 = 1000,
    KC10 = 1001,
  };

  enum class DELTA_SIM {
    C17 = 2000,
    H60 = 2001,
    KC46 = 2002,
  };

  enum class LUNAR_SIM {
    B767 = 3000,
  };

  enum class OUROBOROS_JETS { E170 = 4000, E190 = 4001 };

  enum class QBIT_SIM { B737 = 5000 };

  class EnumRegistry {
public:
    static EnumRegistry &GetInstance() {
      static EnumRegistry instance;
      return instance;
    }

    [[nodiscard]] std::pair<std::string, std::string> GetString(const int enum_value) {
      if (const auto it = m_EnumMap.find(enum_value); it != m_EnumMap.end()) {
        return it->second;
      }
      return {"unknown", "unknown"};
    }

    void Register(const int enum_value, const std::string &group_name, const std::string &aircraft_name) {
      m_EnumMap[enum_value] = std::make_pair(group_name, aircraft_name);
    }

private:
    EnumRegistry() = default;


    void RegisterEnums() {
      Register(static_cast<int>(AERO_DYNAMICS::DC10), "Aero Dynamics", "DC10");
      Register(static_cast<int>(AERO_DYNAMICS::KC10), "Aero Dynamics", "KC10");

      Register(static_cast<int>(DELTA_SIM::C17), "Delta Sim", "C17");
      Register(static_cast<int>(DELTA_SIM::H60), "Delta Sim", "H60");
      Register(static_cast<int>(DELTA_SIM::KC46), "Delta Sim", "KC46");

      Register(static_cast<int>(LUNAR_SIM::B767), "Lunar Sim", "B767");

      Register(static_cast<int>(OUROBOROS_JETS::E170), "Ouroboros Jets", "E170");
      Register(static_cast<int>(OUROBOROS_JETS::E190), "Ouroboros Jets", "E190");

      Register(static_cast<int>(QBIT_SIM::B737), "Qbit Sim", "B737");
    }

private:
    std::unordered_map<int, std::pair<std::string, std::string>> m_EnumMap;
  };

  template<typename T>
  std::pair<std::string, std::string> GetEnumString(T value) {
    return EnumRegistry::GetInstance().GetString(static_cast<int>(value));
  }

  using GroupVariants = std::variant<AERO_DYNAMICS, DELTA_SIM, LUNAR_SIM, OUROBOROS_JETS, QBIT_SIM>;


}  // namespace Infinity::Groups
