#include "Notifications.hpp"

#if defined(__linux__)
std::expected<void, std::string> NotificationManager::SendNotification(const std::string& title,
                                                                       const std::string& message,
                                                                       const std::optional<std::string> icon) {
  NotifyNotification* notification =
      notify_notification_new(title.c_str(), message.c_str(), icon.value_or(m_icon_path).c_str());
  notify_notification_set_timeout(notification, NOTIFY_EXPIRES_DEFAULT);

  bool success = notify_notification_show(notification, nullptr);
  g_object_unref(notification);
  if (!success) {
    return std::unexpected("Failed to send notification");
  }
  return {};
}

#endif
