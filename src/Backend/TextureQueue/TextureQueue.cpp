#include "TextureQueue.hpp"

namespace Infinity {

    std::mutex g_TextureQueueMutex;
    std::vector<std::shared_ptr<Image>> g_TextureCreationQueue;
} // namespace Infinity
