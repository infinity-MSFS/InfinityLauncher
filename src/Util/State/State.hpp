
#pragma once

#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace Infinity {
  class PageState {
public:
    virtual ~PageState() = default;

    virtual void PrintState() const = 0;
  };

  class State {
public:
    static State &GetInstance() {
      static State instance;
      return instance;
    }

    template<typename T>
    void RegisterPageState(const std::string &page_id,
                           std::shared_ptr<T> state) {
      std::scoped_lock lock(m_Mutex);
      const auto type_id = std::type_index(typeid(T));
      m_PageStates[type_id][page_id] = state;
    }

    template<typename T>
    std::optional<std::shared_ptr<T> > GetPageState(
        const std::string &page_id) {
      std::scoped_lock lock(m_Mutex);
      const auto type_id = std::type_index(typeid(T));
      if (const auto type_it = m_PageStates.find(type_id);
          type_it != m_PageStates.end()) {
        if (const auto page_it = type_it->second.find(page_id);
            page_it != type_it->second.end()) {
          return std::static_pointer_cast<T>(page_it->second);
        }
      }
      return std::nullopt;
    }

    template<typename T>
    void RemovePageState(const std::string &page_id) {
      std::scoped_lock lock(m_Mutex);
      const auto type_id = std::type_index(typeid(T));
      if (const auto type_it = m_PageStates.find(type_id);
          type_it != m_PageStates.end()) {
        type_it->second.erase(page_id);
        if (type_it->second.empty()) {
          m_PageStates.erase(type_id);
        }
      }
    }

    void PrintAllStates() const {
      std::scoped_lock lock(m_Mutex);
      for (const auto &[type_id, pages]: m_PageStates) {
        std::cout << "Type: " << type_id.name() << std::endl;
        for (const auto &[page_id, state]: pages) {
          std::cout << "  Page ID: " << page_id << std::endl;
          state->PrintState();
        }
      }
    }

private:
    State() = default;

    std::unordered_map<
        std::type_index,
        std::unordered_map<std::string, std::shared_ptr<PageState> > >
        m_PageStates;
    mutable std::mutex m_Mutex;
  };
}  // namespace Infinity
