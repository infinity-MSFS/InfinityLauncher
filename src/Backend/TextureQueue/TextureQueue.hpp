#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace Infinity {
  class Image;

  extern std::mutex g_texture_queue_mutex;
  extern std::vector<std::shared_ptr<Image>> g_texture_creation_queue;
}  // namespace Infinity
