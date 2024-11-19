
#include "Router.hpp"

#include "Util/Error/Error.hpp"


namespace Infinity::Utils {
    std::unique_ptr<Router> Router::m_Instance = nullptr;

    void Router::configure(std::unordered_map<int, std::function<void()>> pages) {
        if (!m_Instance) {
            m_Instance = std::unique_ptr<Router>(new Router(std::move(pages)));
        }
    }

    std::optional<Router *> Router::getInstance() {
        if (!m_Instance) {
            return std::nullopt;
        }
        return m_Instance.get();
    }

    std::expected<bool, Errors::Error> Router::setPage(const int pageId) {
        if (m_Pages.contains(pageId)) {
            m_CurrentPageID = pageId;
            return true;
        } else {
            std::ostringstream oss;
            oss << "Error: Attempted to access page ID " << pageId;
            return std::unexpected(Errors::Error{Errors::ErrorType::Fatal, oss.str()});
        }
    }

    std::optional<int> Router::getPage() const {
        if (m_CurrentPageID) {
            return m_CurrentPageID;
        }
        return std::nullopt;
    }

    void Router::RenderCurrentPage() {
        if (m_Pages.contains(m_CurrentPageID)) {
            m_Pages[m_CurrentPageID]();
        } else {
            throw std::runtime_error("Failed to render page: Page does not exist.");
        }
    }

    Router::Router(std::unordered_map<int, std::function<void()>> pages) :
        m_CurrentPageID(0), m_Pages(std::move(pages)) {
    }
}
