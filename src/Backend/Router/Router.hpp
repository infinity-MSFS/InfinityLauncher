
#pragma once

#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include "Util/Error/Error.hpp"
#include "Util/State/GroupStateManager.hpp"

namespace Infinity::Utils {
    class Router {
    public:
        static void configure(std::unordered_map<int, std::pair<std::function<void()>, Palette>> pages);

        static std::optional<Router *> getInstance();

        std::expected<bool, Errors::Error> setPage(int pageId);

        [[nodiscard]] std::optional<int> getPage() const;

        void RenderCurrentPage();

    private:
        explicit Router(std::unordered_map<int, std::pair<std::function<void()>, Palette>> pages);

        static std::unique_ptr<Router> m_Instance;
        int m_CurrentPageID;
        std::unordered_map<int, std::pair<std::function<void()>, Palette>> m_PageData;
    };
}

