
#pragma once

#include <vector>
#include <utility>
#include <string>
#include <memory>

#include "Backend/Image/Image.hpp"

namespace Infinity {

    struct HomeProjectButtonStruct {
        std::string name;
        std::shared_ptr<Image> image;
        std::shared_ptr<Image> logo;
        int page_id;

    };

    class Home {
    public:
        static Home *GetInstance();

        void RegisterProject(const std::string &name, const std::string &image_link, const std::string &logo_link, int page_id);
        void UnregisterProject(const std::string &name);
        void RegisterProject(const std::vector<HomeProjectButtonStruct> &projects);
        void UnregisterProject(const std::vector<std::string> &projects);

        void Render();

    private:
        Home() = default;
        static void RenderProject(const HomeProjectButtonStruct &project, int page_index);

    private:
        std::vector<HomeProjectButtonStruct> m_HomeProjectButtons;
        std::vector<std::shared_ptr<Image>> m_Images;
    };
}
