#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace Infinity {
    class Image;

    extern std::mutex g_TextureQueueMutex;
    extern std::vector<std::shared_ptr<Image>> g_TextureCreationQueue;
} // namespace Infinity
