
#pragma once

#include <vector>
#include <utility>
#include <string>

namespace Infinity {

    struct HomeProjectButtonStruct {
        std::string name;
        std::string image_link;
        int page_id;

    };

    class Home {
    public:
        Home() = default;

        void RegisterProject(const std::string &name, const std::string &image_link, int page_id);
        void UnregisterProject(const std::string &name);
        void RegisterProject(const std::vector<HomeProjectButtonStruct> &projects);
        void UnregisterProject(const std::vector<std::string> &projects);

        void Render();

    private:
        std::vector<HomeProjectButtonStruct> m_HomeProjectButtons;
    };
}
