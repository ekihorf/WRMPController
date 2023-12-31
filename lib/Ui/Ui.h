#pragma once

#include "DeviceState.h"
#include "Symbols.h"
#include <etl/vector.h>

namespace ui {
    struct Buffer {
        char line1[16];
        char line2[16];
    };

    enum class Event {
        ButtonShortPress,
        ButtonLongPress,
        EncoderCCW,
        EncoderCW
    };

    class Ui;

    class Parameter {
    public:
        Parameter(const char* name, const char* unit, int32_t& ref, int32_t min, int32_t max, int32_t step, size_t scale = 0, bool show_frac = true);
        char *getName();
        bool draw(Buffer& buffer);
        void increment();
        void decrement();
        void save();

    private:
        char m_name[16] = {0};
        char m_unit[8] = {0};  
        int32_t& m_ref;
        int32_t m_min;
        int32_t m_max;
        size_t m_scale;
        int32_t m_step;
        bool m_show_frac;
    };

    class View {
    public:
        View(Ui& parent);
        virtual bool draw(Buffer& buffer) = 0;
        virtual void handleEvent(Event event) = 0;

    protected:
        Ui& m_parent;
    };

    class MainView : public View {
    public:
        MainView(Ui& parent);
        bool draw(Buffer& buffer) override;
        void handleEvent(Event event) override;
    };

    class ParameterView : public View {
    public:
        ParameterView(Ui& parent);
        bool draw(Buffer& buffer) override;
        void handleEvent(Event event) override;
        void setParameter(Parameter& parameter);

    private:
        Parameter* m_parameter{nullptr}; 
    };

    

    class SettingsView : public View {
    public:
        SettingsView(Ui& parent);
        bool draw(Buffer& buffer) override;
        void handleEvent(Event event) override;
        void addParameter(Parameter& param);
    
    private:
        etl::vector<Parameter*, 10> m_parameters;
        uint32_t m_selected_parameter = 0;
        uint32_t m_scroll_position = 0;
        ParameterView m_parameter_view{m_parent};

        void goUp();
        void goDown();
    };

    class Ui {
    public:
        Ui(DeviceState& device_state);
        void enterMainView();
        void enterSettingsView();
        void enterView(View& view);
        bool draw(Buffer& buffer);
        void handleEvent(Event event);
        DeviceState& getDeviceState();
        void addParameter(Parameter& param);
        bool isAtMainView();

    private:
        MainView m_main_view;
        SettingsView m_menu_view;
        View* m_current_view{&m_main_view};

        DeviceState& m_device_state;
    };
}