#pragma once

class InterruptHandler {
public:
    template <auto Func, typename Class>
    void connect(Class* obj) {
        m_callback.obj  = obj;
        m_callback.func = [](void* data) {
            Class* obj = static_cast<Class*>(data);
            (obj->*Func)();
        };
    }

    template <void (*Func)()>
    void connect() {
        m_callback.obj  = nullptr;
        m_callback.func = [](void* data) {
            Func();
        };
    }

    void operator()() const {
        if (m_callback.func) {
            m_callback.func(m_callback.obj);
        }
    }
    
private:
    struct Callback {
        using Func = void (*)(void*);
        void* obj  = nullptr;
        Func  func = nullptr;
    };

    Callback m_callback;
};