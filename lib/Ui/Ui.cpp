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
    m_current_view = m_main_view;
}

void ui::Ui::enterMenuView() {
    m_current_view = m_menu_view;
}

bool ui::Ui::draw(Buffer &buffer) {
    return m_current_view.get().draw(buffer);
}

void ui::Ui::handleEvent(Event event) {
    m_current_view.get().handleEvent(event);
}

DeviceState &ui::Ui::getDeviceState() {
    return m_device_state;
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
        m_parent.enterMenuView();
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
            ds.heating_status == HeatingStatus::On;
        }
    }

    return true;
}

ui::MenuView::MenuView(Ui &parent) : ui::View{parent} {}

bool ui::MenuView::draw(Buffer &buffer)
{
    memcpy(buffer.line1, ">Menu item 1    ", 16);
    memcpy(buffer.line2, " Menu item 2    ", 16);

    return true;
}

bool ui::MenuView::handleEvent(Event event)
{
    if (event == Event::ButtonShortPress) {
        m_parent.enterMainView();
    }
}
