
#pragma once

#include "GL/glew.h"
//

#include <GL/gl.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "curl/curl.h"
#include "imgui.h"

struct GLFWwindow;

namespace Infinity {

  class Image {
public:
    enum class Format { None, RGBA8, RGBA32F };

    Image();
    ~Image();

    Image(const Image &) = delete;
    Image &operator=(const Image &) = delete;
    Image(Image &&other) noexcept;
    Image &operator=(Image &&other) noexcept;

    static std::shared_ptr<Image> Create(uint32_t width, uint32_t height, Format format = Format::RGBA8,
                                         const void *data = nullptr);

    static std::shared_ptr<Image> LoadFromMemory(const void *data, size_t dataSize);

    static std::shared_ptr<Image> LoadFromURL(const std::string &url);

    static std::shared_ptr<Image> LoadFromBinary(const std::vector<uint8_t> &binaryData);

    static std::vector<uint8_t> FetchFromURL(const std::string &url);

    void SetData(const void *data);
    void Resize(uint32_t width, uint32_t height);
    void Release();


    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }
    Format GetFormat() const { return m_Format; }
    uint32_t GetTextureID() const;

    void *GetImGuiTextureID() const;

    static void *DecodeImage(const uint8_t *data, size_t dataSize, uint32_t &outWidth, uint32_t &outHeight);


    /// <summary>
    /// Renders a pre-constructed image with no controls for width and height,
    /// only scale
    /// </summary>
    /// <param name="image">Shared pointer to an Infinity::Image</param>
    /// <param name="pos">X and Y position for the image</param>
    /// <param name="scale">Scale of the image 1.0f - 0.0f</param>
    static void RenderImage(const std::unique_ptr<Image> &image, ImVec2 pos, float scale);
    static void RenderImage(const std::shared_ptr<Image> &image, ImVec2 pos, float scale);

    /// <summary>
    /// Renders a pre-constructed image with a specified width and height. The
    /// image will always fill the height requirement (if the image width >
    /// specified width, the image will be clipped evenly left and right)
    /// </summary>
    /// <param name="image">Shared pointer to an Infinity::Image</param>
    /// <param name="pos">X any Y position to render the image</param>
    /// <param name="size">Size of the image (Width, Height)</param>
    static void RenderImage(const std::unique_ptr<Image> &image, ImVec2 pos, ImVec2 size);
    static void RenderImage(const std::shared_ptr<Image> &image, ImVec2 pos, ImVec2 size);
    static void RenderImage(const std::shared_ptr<Image> &image, ImVec2 pos, ImVec2 size, float opacity);
    static void RenderHomeImage(const std::unique_ptr<Image> &image, ImVec2 pos, ImVec2 size, bool is_hovered);
    static void RenderHomeImage(const std::shared_ptr<Image> &image, ImVec2 pos, ImVec2 size, bool is_hovered);

    static float &GetAnimationProgress(ImGuiID id);
    void CreateGLTexture();


    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    Format m_Format = Format::None;

    void AllocateMemory(const void *data);

private:
    // Helper to select the correct OpenGL format based on the Image format
    static uint32_t GetGLFormat(Format format);
    static uint32_t GetGLInternalFormat(Format format);
    static uint32_t GetGLDataType(Format format);
    static uint32_t GetBytesPerPixel(Format format);


    class Impl;
    std::unique_ptr<Impl> m_Impl;


    static std::unordered_map<ImGuiID, float> s_AnimationProgress;
  };


}  // namespace Infinity
