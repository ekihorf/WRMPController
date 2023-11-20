#pragma once

#include <functional>

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

    class View {
    public:
        View(Ui& parent);
        virtual bool draw(Buffer& buffer) = 0;
        virtual bool handleEvent(Event event) = 0;

    protected:
        Ui& m_parent;
    };

    class MainView : public View {
    public:
        MainView(Ui& parent);
        bool draw(Buffer& buffer) override; 
        bool handleEvent(Event event) override;

    private:

    };

    class MenuView : public View {
    public:
        MenuView(Ui& parent);
        bool draw(Buffer& buffer) override; 
        bool handleEvent(Event event) override;

    private:

    };


    class Ui {
    public:
        Ui();
        void enterMainView();
        void enterMenuView();
        bool draw(Buffer& buffer);
        void handleEvent(Event event); 

    private:
        MainView m_main_view;
        MenuView m_menu_view;
        std::reference_wrapper<View> m_current_view = m_main_view;
    };
}