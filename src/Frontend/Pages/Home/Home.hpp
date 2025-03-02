
#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Backend/Image/Image.hpp"

namespace Infinity {

    struct HomeProjectButtonStruct {
        std::string name;
        std::shared_ptr<Image> image;
        std::shared_ptr<Image> logo;
        int page_id;

        HomeProjectButtonStruct(const std::string &name, std::shared_ptr<Image> image, std::shared_ptr<Image> logo, const int page_id) :
            name(name), image(std::move(image)), logo(std::move(logo)), page_id(page_id) {}
    };

    class Home {
    public:
        static Home *GetInstance();

        static void SetExpectedProjects(const unsigned int project_quantity) { m_ExpectedProjects = project_quantity; };
        static bool DoneLoading() { return m_DoneLoading; };
        static void SetLoaded(const bool loaded) { m_DoneLoading = loaded; }

        void RegisterProject(const std::string &name, std::shared_ptr<Image> image, std::shared_ptr<Image> logo, int page_id);
        void UnregisterProject(const std::string &name);
        void RegisterProject(const std::vector<HomeProjectButtonStruct> &projects);
        void UnregisterProject(const std::vector<std::string> &projects);

        void Render() const;

    private:
        Home() = default;
        static void RenderProject(const HomeProjectButtonStruct &project, int page_index);

    private:
        std::vector<HomeProjectButtonStruct> m_HomeProjectButtons;
        static unsigned int m_ExpectedProjects;
        static bool m_DoneLoading;
    };
} // namespace Infinity
