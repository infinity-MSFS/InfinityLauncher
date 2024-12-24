
#include "Router.hpp"

#include "Frontend/ColorInterpolation/ColorInterpolation.hpp"
#include "Util/Error/Error.hpp"


namespace Infinity::Utils {
    std::unique_ptr<Router> Router::m_Instance = nullptr;

    void Router::configure(std::unordered_map<int, std::pair<std::function<void()>, Palette>> page_content) {
        if (!m_Instance) {
            m_Instance = std::unique_ptr<Router>(new Router(std::move(page_content)));
        }
        std::cout << "Configuring Router" << std::endl;
    }

    std::optional<Router *> Router::getInstance() {
        if (!m_Instance) {
            return std::nullopt;
        }
        return m_Instance.get();
    }

    std::expected<bool, Errors::Error> Router::setPage(const int pageId) {
        if (m_PageData.contains(pageId)) {
            m_CurrentPageID = pageId;
            ColorInterpolation::GetInstance().ChangeGradientColors(hexToImVec4(m_PageData[m_CurrentPageID].second.primary), hexToImVec4(m_PageData[m_CurrentPageID].second.secondary),
                                                                   hexToImVec4(m_PageData[m_CurrentPageID].second.circle1), hexToImVec4(m_PageData[m_CurrentPageID].second.circle2),
                                                                   hexToImVec4(m_PageData[m_CurrentPageID].second.circle3), hexToImVec4(m_PageData[m_CurrentPageID].second.circle4),
                                                                   hexToImVec4(m_PageData[m_CurrentPageID].second.circle5),
                                                                   1.0f);
            return true;
        } else {
            std::ostringstream oss;
            oss << "Error: Attempted to access page ID " << pageId;
            return std::unexpected(Errors::Error{Errors::ErrorType::Fatal, oss.str()});
        }
    }

    int Router::getPage() const {
        return m_CurrentPageID;
    }

    void Router::RenderCurrentPage() {
        if (m_PageData.contains(m_CurrentPageID)) {
            m_PageData[m_CurrentPageID].first();
        } else {
            throw std::runtime_error("Failed to render page: Page does not exist.");
        }
    }

    Router::Router(std::unordered_map<int, std::pair<std::function<void()>, Palette>> pages) :
        m_CurrentPageID(0), m_PageData(std::move(pages)) {
    }
}
