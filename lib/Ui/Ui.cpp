#include "Ui.h"
#include <cstring>
#include "Utils.h"
#include "Constants.h"

ui::View::View(Ui &parent) : m_parent{parent} {}

ui::Ui::Ui(DeviceState& device_state)
: m_device_state{device_state},
  m_main_view{*this},
  m_menu_view{*this} {}

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

ui::MainView::MainView(Ui &parent) : ui::View{parent} {}

bool ui::MainView::draw(Buffer &buffer) {
    auto& ds = m_parent.getDeviceState();
    if (ds.heating_status == HeatingStatus::On) {
        memcpy(buffer.line1, "Power on       C", 16);
        buffer.line1[14] = 0xDF; // degree symbol
        buffer.line1[10] = 'S'; // TODO: replace with custom symbol
        utils::uintToStr(buffer.line1 + 11, ds.set_temp.value, 3); 
    } else if (ds.heating_status == HeatingStatus::Off) {
        memcpy(buffer.line1, "Power off      C", 16);
        buffer.line1[14] = 0xDF; // degree symbol
        buffer.line1[10] = 'S'; // TODO: replace with custom symbol
        utils::uintToStr(buffer.line1 + 11, ds.set_temp.value, 3); 
    } else if (ds.heating_status == HeatingStatus::Standby) {
        memcpy(buffer.line1, "Standby        C", 16);
        buffer.line1[14] = 0xDF; // degree symbol
        buffer.line1[10] = 'S'; // TODO: replace with custom symbol
        utils::uintToStr(buffer.line1 + 11, ds.standby_temp.value, 3); 
    }

    memcpy(buffer.line2, "P   %          C", 16);
    buffer.line2[14] = 0xDF; // degree symbol
    buffer.line2[10] = 'T'; // TODO: replace with custom symbol
    utils::uintToStr(buffer.line2 + 1, ds.heater_power, 3);
    utils::uintToStr(buffer.line2 + 11, ds.tip_temp.value, 3); 
    return true;
}

bool ui::MainView::handleEvent(Event event) {
    auto& ds = m_parent.getDeviceState();
    bool temp_changed = false;
    switch(event) {
    case Event::ButtonLongPress:
        m_parent.enterSettingsView();
        break;

    case Event::ButtonShortPress:
        if (ds.heating_status != HeatingStatus::On) {
            ds.heating_status = HeatingStatus::On;
        } else {
            ds.heating_status = HeatingStatus::Off;
        }
        break;
    
    case Event::EncoderCW:
        ds.set_temp.value += ds.temp_increment;
        temp_changed = true;
        break;

    case Event::EncoderCCW:
        ds.set_temp.value -= ds.temp_increment;
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

        if (ds.heating_status == HeatingStatus::Standby) {
            ds.heating_status = HeatingStatus::On;
        }
    }

    return true;
}

static int32_t param1 = 5;
static int32_t param2 = 10;
static int32_t param3 = 16;
ui::I32Parameter p1("width", "cm", param1, -200, 200, 1, 1);
ui::I32Parameter p2("height", "m", param2, 0, 20, 1, 1);
ui::I32Parameter p3("length", "km", param3, 0, 20, 1, 1);


ui::SettingsView::SettingsView(Ui &parent) : ui::View{parent} {
    m_parameters.push_back(&p1);
    m_parameters.push_back(&p2);
    m_parameters.push_back(&p3);
}

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

bool ui::SettingsView::handleEvent(Event event) {
    switch (event) {
    case Event::ButtonLongPress:
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

    default:
        return false;
    }

    return true;
}

void ui::SettingsView::goUp() {
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

ui::Parameter::Parameter(char *name, char *unit) {
    strncat(m_name, name, 15);
    strncat(m_unit, unit, 7);
}

char *ui::Parameter::getName() {
    return m_name;
}

ui::ParameterView::ParameterView(Ui &parent) : View{parent} {}

bool ui::ParameterView::draw(Buffer &buffer) {
    if (!m_parameter) {
        return false;
    }

    return m_parameter->draw(buffer);
}

bool ui::ParameterView::handleEvent(Event event) {
    if (!m_parameter) {
        return false;
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
    }
}

void ui::ParameterView::setParameter(Parameter &parameter) {
    m_parameter = &parameter;
}

ui::I32Parameter::I32Parameter(char *name, char *unit, int32_t &ref, int32_t min, int32_t max, int32_t scale, int32_t step)
: Parameter{name, unit}, m_ref{ref}, m_min{min}, m_max{max}, m_scale{scale}, m_step{step}, m_val{ref} {

}

bool ui::I32Parameter::draw(Buffer& buffer) {
    buffer.line1[0] = '\0';
    strncat(buffer.line1, m_name, 15);

    utils::fixedIntToStr(buffer.line2, m_val, 5, 1);
    // utils::intToStr(buffer.line2, m_val, 5);
    buffer.line2[5] = ' ';
    buffer.line2[6] = '\0';
    strncat(buffer.line2 + 6, m_unit, 5);

    return true;
}

void ui::I32Parameter::increment() {
    m_val += m_step;
    if (m_val > m_max) {
        m_val = m_max;
    }
}

void ui::I32Parameter::decrement() {
    m_val -= m_step;
    if (m_val < m_min) {
        m_val = m_min;
    }
}

void ui::I32Parameter::save() {
    if (m_val > m_max) { m_val = m_max; };
    if (m_val < m_min) { m_val = m_min; };
    m_ref = m_val;
}
