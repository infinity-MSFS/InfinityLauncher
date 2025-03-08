
#pragma once

#include <string>
#include <vector>

#include "Backend/Image/Image.hpp"
#include "Frontend/ColorInterpolation/ColorInterpolation.hpp"
#include "Util/State/GroupStateManager.hpp"

namespace Infinity {

    class AircraftSelectButton {
    public:
        AircraftSelectButton(std::string name, int32_t id);
        void Render();

        void SetActive(bool active);

    private:
        std::string m_Name;
        int32_t m_Id;
        bool m_Active;
    };

    class AircraftSelectButtonBar {
    public:
        explicit AircraftSelectButtonBar(std::vector<AircraftSelectButton> aircraft_select_buttons);
        void Render();
        [[nodiscard]] int32_t GetSelectedAircraftId() const;

    private:
        std::vector<AircraftSelectButton> m_AircraftSelectButtons;
        int32_t m_SelectedAircraftId;
    };

    class VariantButton {
    public:
        explicit VariantButton(const std::string &name, int32_t id);

        void Render();

        void SetActive(bool active);

    private:
        std::string m_Name;
        int32_t m_Id;
        bool m_Active;
    };

    class VariantButtonBar {
    public:
        VariantButtonBar(std::vector<VariantButton> variant_buttons, const std::string &title);

    private:
        std::string m_Title;
        std::vector<VariantButton> m_VariantButtons;
        int32_t m_SelectedVariantId;
    };

    class ContentRegionButton {};

    class ContentRegionButtonBar {};

    class ContentRegion {};

    class ProjectPage {
    public:
        explicit ProjectPage(GroupData group_data, GroupDataImages state_images);

        void Render();

    private:
        GroupData m_GroupData;
        GroupDataImages m_StateImages;
    };
} // namespace Infinity
