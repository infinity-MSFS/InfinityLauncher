
#pragma once

#include <string>
#include <vector>

#include "Backend/Image/Image.hpp"
#include "Frontend/ColorInterpolation/ColorInterpolation.hpp"
#include "Util/State/GroupStateManager.hpp"

namespace Infinity {

    class AircraftSelectButton {
    public:
        AircraftSelectButton(std::string name, int32_t id, const std::shared_ptr<uint8_t> &selected_aircraft);
        void Render();

    private:
        bool IsActive();

    private:
        std::string m_Name;
        int32_t m_Id;
        bool m_Active;
        std::shared_ptr<uint8_t> m_SelectedAircraft;
    };

    class AircraftSelectButtonBar {
    public:
        explicit AircraftSelectButtonBar(const std::vector<AircraftSelectButton> &aircraft_select_buttons, const std::shared_ptr<uint8_t> &selected_aircraft);
        void Render();

    private:
        std::vector<AircraftSelectButton> m_AircraftSelectButtons;
        std::shared_ptr<uint8_t> m_SelectedAircraft;
    };

    class TopRegion {
    public:
        explicit TopRegion(GroupData *group_data, const std::shared_ptr<uint8_t> &selected_aircraft);
        void Render();

    private:
        AircraftSelectButtonBar m_AircraftSelectButtonBar;
        std::vector<AircraftSelectButton> m_AircraftSelectButtons;
        std::shared_ptr<uint8_t> m_SelectedAircraft;
    };


    class ContentRegionButton {
    public:
        struct Button {
            std::string name;
            uint8_t page;
        };

        explicit ContentRegionButton(const std::string &name, int8_t id, const std::shared_ptr<uint8_t> &selected_page);
        explicit ContentRegionButton(const Button &button, const std::shared_ptr<uint8_t> &selected_page);

        void Render();

    private:
        Button m_ButtonSpec;
        std::shared_ptr<uint8_t> m_SelectedPage;
    };

    class ContentRegionButtonBar {
    public:
        explicit ContentRegionButtonBar(std::vector<ContentRegionButton> buttons, const std::shared_ptr<uint8_t> &selected_page);
        void Render();

    private:
        std::vector<ContentRegionButton> m_Buttons;
        std::shared_ptr<uint8_t> m_SelectedPage;
    };

    class ContentRegion {
    public:
        explicit ContentRegion(GroupData *group_data, const std::shared_ptr<uint8_t> &selected_page);
        void Render();

    private:
        std::vector<std::string> m_Changelogs;
        std::string m_Description;
        std::string m_Overview;

        GroupData *m_GroupData;
        ContentRegionButtonBar m_ButtonBar;
        std::vector<ContentRegionButton> m_Buttons;

        std::shared_ptr<uint8_t> m_SelectedPage;
    };

    class ProjectPage {
    public:
        explicit ProjectPage(const GroupData &group_data, GroupDataImages state_images);
        static void ResetState() {
            m_SelectedPage = std::make_shared<uint8_t>(0);
            m_SelectedAircraft = std::make_shared<uint8_t>(0);
        }
        void Render();

    private:
        GroupData m_GroupData;
        GroupDataImages m_StateImages;

        static std::shared_ptr<uint8_t> m_SelectedPage;
        static std::shared_ptr<uint8_t> m_SelectedAircraft;

        ContentRegion m_ContentRegion;
        TopRegion m_TopRegion;
    };
} // namespace Infinity
