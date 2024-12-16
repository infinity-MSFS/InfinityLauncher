
#pragma once

#include <string>
#include <vector>

#include "Util/State/GroupStateManager.hpp"
#include "Frontend/ColorInterpolation/ColorInterpolation.hpp"

namespace Infinity {
    class ProjectPage {
    public:
        explicit ProjectPage(GroupData group_data);

        void Render();

    private:
        GroupData m_GroupData;
    };
}
