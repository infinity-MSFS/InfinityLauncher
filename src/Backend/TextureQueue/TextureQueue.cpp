#include "TextureQueue.hpp"

namespace Infinity {

  std::mutex g_texture_queue_mutex;
  std::vector<std::shared_ptr<Image>> g_texture_creation_queue;
}  // namespace Infinity
