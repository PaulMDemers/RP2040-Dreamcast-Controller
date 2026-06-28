#pragma once

#include <cstdint>

#include "dreamcast/maple_pio_buffer.hpp"

#ifdef PICO_ON_DEVICE
#include "hardware/pio.h"
#endif

namespace dreamcast::pico {

struct MaplePhyConfig {
    std::uint32_t pin_a = 10;
    std::uint32_t sys_clock_khz = 133000;
    std::uint32_t ns_per_bit = 480;
    std::uint32_t idle_check_us = 10;
};

class PicoMaplePhy {
public:
#ifdef PICO_ON_DEVICE
    explicit PicoMaplePhy(const MaplePhyConfig& config,
                          PIO tx_pio = pio0,
                          PIO rx_pio = pio1);
#else
    explicit PicoMaplePhy(const MaplePhyConfig& config);
#endif

    void init();
    bool line_is_idle(std::uint32_t idle_us) const;
    bool write_blocking(const MaplePioTxBuffer& buffer, std::uint32_t timeout_us);
    void release_lines();

    std::uint32_t pin_a() const;
    std::uint32_t pin_b() const;

private:
    MaplePhyConfig config_;
    std::uint32_t pin_b_;
    std::uint32_t mask_ab_;

#ifdef PICO_ON_DEVICE
    PIO tx_pio_;
    PIO rx_pio_;
    std::uint tx_sm_;
    std::uint rx_sm_;
    std::uint tx_program_offset_;
    std::uint rx_program_offset_;
    bool initialized_;
#endif
};

}  // namespace dreamcast::pico

