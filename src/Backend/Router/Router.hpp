
#pragma once

#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>

#include "Util/Error/Error.hpp"
#include "Util/State/GroupStateManager.hpp"

namespace Infinity::Utils {
  class Router {
public:
    static void configure(std::unordered_map<int, std::pair<std::function<void()>, Palette>> pages);

    static std::optional<Router *> getInstance();

    std::expected<bool, Errors::Error> setPage(int page_id);

    [[nodiscard]] int getPage() const;

    void RenderCurrentPage();

private:
    explicit Router(std::unordered_map<int, std::pair<std::function<void()>, Palette>> pages);

    static std::unique_ptr<Router> m_instance;
    int m_current_page_id;
    std::unordered_map<int, std::pair<std::function<void()>, Palette>> m_page_data;
  };
}  // namespace Infinity::Utils
