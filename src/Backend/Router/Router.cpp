
#include "Router.hpp"

#include <sstream>
#include <stdexcept>

#include "Frontend/Background/Background.hpp"
#include "Frontend/ColorInterpolation/ColorInterpolation.hpp"
#include "Util/Error/Error.hpp"


namespace Infinity::Utils {
  std::unique_ptr<Router> Router::m_instance = nullptr;

  void Router::configure(std::unordered_map<int, std::pair<std::function<void()>, Palette>> pages) {
    if (!m_instance) {
      m_instance = std::unique_ptr<Router>(new Router(std::move(pages)));
    }
    std::cout << "Configuring Router" << std::endl;
  }

  std::optional<Router *> Router::getInstance() {
    if (!m_instance) {
      return std::nullopt;
    }
    return m_instance.get();
  }

  std::expected<bool, Errors::Error> Router::setPage(const int page_id) {
    if (m_page_data.contains(page_id)) {
      m_current_page_id = page_id;
      Background::GetInstance()->SetDotOpacity(page_id < 3 ? 0.3f : 0.1f);
      ColorInterpolation::GetInstance().ChangeGradientColors(
          hexToImVec4(m_page_data[m_current_page_id].second.primary),
          hexToImVec4(m_page_data[m_current_page_id].second.secondary),
          hexToImVec4(m_page_data[m_current_page_id].second.circle1),
          hexToImVec4(m_page_data[m_current_page_id].second.circle2),
          hexToImVec4(m_page_data[m_current_page_id].second.circle3),
          hexToImVec4(m_page_data[m_current_page_id].second.circle4),
          hexToImVec4(m_page_data[m_current_page_id].second.circle5), 1.0f);
      return true;
    }
    std::ostringstream oss;
    oss << "Error: Attempted to access page ID " << page_id;
    return std::unexpected(Errors::Error{Errors::ErrorType::Fatal, oss.str()});
  }

  int Router::getPage() const { return m_current_page_id; }

  void Router::RenderCurrentPage() {
    if (m_page_data.contains(m_current_page_id)) {
      m_page_data[m_current_page_id].first();
    } else {
      throw std::runtime_error("Failed to render page: Page does not exist.");
    }
  }

  Router::Router(std::unordered_map<int, std::pair<std::function<void()>, Palette>> pages)
      : m_current_page_id(0)
      , m_page_data(std::move(pages)) {}
}  // namespace Infinity::Utils
