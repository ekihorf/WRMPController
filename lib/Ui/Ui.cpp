#include "Ui.h"
#include <cstring>
#include "Utils.h"
#include "Constants.h"

ui::View::View(Ui &parent) : m_parent{parent} {}

ui::Ui::Ui(DeviceState& device_state)
: m_main_view{*this},
  m_menu_view{*this},
  m_device_state{device_state} {}

void ui::Ui::enterMainView() {
    m_current_view = &m_main_view;
}

void ui::Ui::enterSettingsView() {
    m_current_view = &m_menu_view;
}

bool ui::Ui::draw(Buffer &buffer) {
    return m_current_view->draw(buffer);
}

void ui::Ui::handleEvent(Event event) {
    m_current_view->handleEvent(event);
}

DeviceState &ui::Ui::getDeviceState() {
    return m_device_state;
}

void ui::Ui::enterView(View &view) {
    m_current_view = &view;
}

void ui::Ui::addParameter(Parameter &param) {
    m_menu_view.addParameter(param);
}

bool ui::Ui::isAtMainView() {
    return m_current_view == &m_main_view;
}

ui::MainView::MainView(Ui &parent) : ui::View{parent} {}

bool ui::MainView::draw(Buffer &buffer) {
    auto& ds = m_parent.getDeviceState();

    if (ds.heating_status == HeatingStatus::On) {
        memcpy(buffer.line1, "  On          \337C", 16);
    } else if (ds.heating_status == HeatingStatus::Off) {
        memcpy(buffer.line1, "  Off         \337C", 16);
    } else if (ds.heating_status == HeatingStatus::Standby) {
        memcpy(buffer.line1, "  Standby     \337C", 16);
    }

    buffer.line1[10] = static_cast<uint8_t>(Symbols::SetTemperature);
    utils::uintToStr(buffer.line1 + 11, ds.set_temp.asDegreesC(), 3); 

    if (ds.handle_in_stand) {
        buffer.line1[0] = static_cast<uint8_t>(Symbols::HandleInStand);
    } else {
        buffer.line1[0] = static_cast<uint8_t>(Symbols::HandleNotInStand);
    }

    memcpy(buffer.line2, "              \337C", 16);
    buffer.line2[10] = static_cast<uint8_t>(Symbols::CurrentTemperature);
    if (ds.heating_status != HeatingStatus::Off) {
        int bars = ds.heater_power / 10;
        for (int i = 0; i < bars; ++i) {
            buffer.line2[i] = 0xFF;
        } 
    }
    
    utils::uintToStr(buffer.line2 + 11, ds.tip_temp.asDegreesC(), 3); 
    return true;
}

void ui::MainView::handleEvent(Event event) {
    auto& ds = m_parent.getDeviceState();
    bool temp_changed = false;
    switch(event) {
    case Event::ButtonLongPress:
        m_parent.enterSettingsView();
        break;

    case Event::ButtonShortPress:
        break;
    
    case Event::EncoderCW:
        ds.set_temp += ds.settings.temp_increment;
        temp_changed = true;
        break;

    case Event::EncoderCCW:
        ds.set_temp -= ds.settings.temp_increment;
        temp_changed = true;
        break;
    } 

    if (temp_changed) {
        if (ds.set_temp > MAX_TIP_TEMP) {
            ds.set_temp = MAX_TIP_TEMP;
        }

        if (ds.set_temp < MIN_TIP_TEMP) {
            ds.set_temp = MIN_TIP_TEMP;
        }

        ds.temp_updated = true;
    }
}

ui::SettingsView::SettingsView(Ui &parent) : ui::View{parent} {}

bool ui::SettingsView::draw(Buffer &buffer) {
    if (m_selected_parameter == m_scroll_position) {
        buffer.line1[0] = '>';
        buffer.line2[0] = ' ';
    } else if (m_selected_parameter == m_scroll_position + 1) {
        buffer.line2[0] = '>';
        buffer.line1[0] = ' ';
    }

    if (m_scroll_position >= m_parameters.size()) {
        return false;
    }

    strncpy(buffer.line1 + 1, m_parameters[m_scroll_position]->getName(), 15);
    
    if (m_scroll_position + 1 >= m_parameters.size()) {
        buffer.line2[0] = '\0';
        return true;
    }

    strncpy(buffer.line2 + 1, m_parameters[m_scroll_position+1]->getName(), 15);

    return true;
}

void ui::SettingsView::handleEvent(Event event) {
    switch (event) {
    case Event::ButtonLongPress:
        m_parent.getDeviceState().settings_updated = true;
        m_parent.enterMainView();
        break;
    
    case Event::ButtonShortPress:
        m_parameter_view.setParameter(*m_parameters[m_selected_parameter]);
        m_parent.enterView(m_parameter_view);
        break;
    
    case Event::EncoderCCW:
        goUp();
        break;
    
    case Event::EncoderCW:
        goDown();
        break;
    }
}

void ui::SettingsView::addParameter(Parameter &param) {
    m_parameters.push_back(&param);
}

void ui::SettingsView::goUp()
{
    if (m_selected_parameter > 0) {
        --m_selected_parameter;
    }

    if (m_selected_parameter < m_scroll_position) {
        m_scroll_position = m_selected_parameter;
    }
}

void ui::SettingsView::goDown() {
    if (m_selected_parameter < m_parameters.size() - 1) {
        ++m_selected_parameter;
    }

    if (m_selected_parameter > m_scroll_position + 1) {
        m_scroll_position = m_selected_parameter - 1;
    }
}

ui::ParameterView::ParameterView(Ui &parent) : View{parent} {}

bool ui::ParameterView::draw(Buffer &buffer) {
    if (!m_parameter) {
        return false;
    }

    return m_parameter->draw(buffer);
}

void ui::ParameterView::handleEvent(Event event) {
    if (!m_parameter) {
        return;
    }

    switch (event) {
    case Event::ButtonShortPress:
        m_parameter->save();
        m_parent.enterSettingsView();
        break;
    case Event::EncoderCCW:
        m_parameter->decrement();
        break;
    case Event::EncoderCW:
        m_parameter->increment();
        break;
    default:
        break;
    }
}

void ui::ParameterView::setParameter(Parameter &parameter) {
    m_parameter = &parameter;
}

ui::Parameter::Parameter(const char *name, const char *unit, int32_t &ref, int32_t min, int32_t max, int32_t step, size_t scale, bool show_frac)
: m_ref{ref}, m_min{min}, m_max{max}, m_step{step}, m_scale{scale}, m_show_frac{show_frac} {
    strncat(m_name, name, 15);
    strncat(m_unit, unit, 7);
}

char *ui::Parameter::getName()
{
    return m_name;
}

bool ui::Parameter::draw(Buffer &buffer)
{
    buffer.line1[0] = '\0';
    strncat(buffer.line1, m_name, 15);

    if (m_scale && m_show_frac) {
        utils::fixedIntToStr(buffer.line2, m_ref, 8, m_scale);
    } else {
        int32_t scale = 1;
        for (int i = 0; i < m_scale; ++i) {
            scale *= 10;
        }
        utils::intToStr(buffer.line2, m_ref / scale, 8);
    }
    buffer.line2[8] = ' ';
    buffer.line2[9] = '\0';
    strncat(buffer.line2 + 9, m_unit, 7);

    return true;
}

void ui::Parameter::increment() {
    m_ref += m_step;
    if (m_ref > m_max) {
        m_ref = m_max;
    }
}

void ui::Parameter::decrement() {
    m_ref -= m_step;
    if (m_ref < m_min) {
        m_ref = m_min;
    }
}

void ui::Parameter::save() {
    if (m_ref > m_max) { m_ref = m_max; };
    if (m_ref < m_min) { m_ref = m_min; };
}
