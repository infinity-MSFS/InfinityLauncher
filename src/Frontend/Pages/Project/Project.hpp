
#pragma once

#include <string>
#include <vector>

#include "Backend/Image/Image.hpp"
#include "Util/State/GroupStateManager.hpp"
#include "Frontend/ColorInterpolation/ColorInterpolation.hpp"

namespace Infinity {
    class ProjectPage {
    public:
        explicit ProjectPage(GroupData group_data, GroupDataImages state_images);

        void Render();

    private:
        GroupData m_GroupData;
        GroupDataImages m_StateImages;
    };
}
