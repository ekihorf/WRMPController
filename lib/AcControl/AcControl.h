#include <cstdint>

class AcControl {
public:
    enum class Status {
       Running,
       ZeroDetectorFault
    };

    AcControl(uint32_t zero_port, uint16_t zero_pin, uint32_t out_port, uint16_t out_pin);

    void setControlPeriod(uint32_t halfcycles);
    void setMainsFrequency(uint32_t freq_hz);
    void setOnHalfcycles(uint32_t on_halfcycles);
    void update(uint32_t current_ticks);
    void signalZeroCrossing();
    Status getStatus();

private:
    uint32_t m_zero_port;
    uint16_t m_zero_pin;
    uint32_t m_out_port;
    uint16_t m_out_pin;
    uint32_t m_mains_frequency = 50;
    uint32_t m_control_period = 20;
    uint32_t m_current_halfcycle = 0;
    uint32_t m_duty_cycle = 0;
    uint32_t m_halfcycles_since_update = 0;
    uint32_t m_previous_ticks = 0;
    Status m_status = Status::Running;
};