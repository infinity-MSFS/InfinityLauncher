#pragma once

#include <expected>
#include <optional>
#include <string>

#if defined(__linux__)
#include <libnotify/notify.h>
class NotificationManager {
  public:
  explicit NotificationManager(const std::string& app_name = "InfinityLauncher")
      : m_app_name(app_name)
      , m_icon_path("usr/share/icons/hicolor/256x256/apps/kitty.png") {
    notify_init(m_app_name.c_str());
  }

  ~NotificationManager() { notify_uninit(); }

  std::expected<void, std::string> SendNotification(const std::string& title, const std::string& message,
                                                    std::optional<std::string> icon);

  private:
  std::string m_app_name;
  std::string m_icon_path;
};
#endif

class Notifications {};
