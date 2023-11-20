#include "Ui.h"
#include <cstring>

ui::View::View(Ui &parent) : m_parent{parent} {}

ui::Ui::Ui() : m_main_view{*this}, m_menu_view{*this} {}

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

ui::MainView::MainView(Ui &parent) : ui::View{parent} {}

bool ui::MainView::draw(Buffer &buffer) {
    memcpy(buffer.line1, "Main view       ", 16);
    memcpy(buffer.line2, "4657            ", 16);
    return true;
}

bool ui::MainView::handleEvent(Event event) {
    if (event == Event::ButtonLongPress) {
        m_parent.enterMenuView();
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
