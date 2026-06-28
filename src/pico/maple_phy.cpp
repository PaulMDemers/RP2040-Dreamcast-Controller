#include "dreamcast/pico/maple_phy.hpp"

#ifdef PICO_ON_DEVICE
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "pico/time.h"

#include "maple_in.pio.h"
#include "maple_out.pio.h"
#endif

namespace dreamcast::pico {

#ifdef PICO_ON_DEVICE
namespace {

float maple_tx_clkdiv(std::uint32_t sys_clock_khz, std::uint32_t ns_per_bit) {
    const float ticks_per_double_phase = static_cast<float>(maple_out_DOUBLE_PHASE_TICKS);
    const float ns_per_double_phase = static_cast<float>((ns_per_bit / 3u) * 2u);
    return (static_cast<float>(sys_clock_khz) * ns_per_double_phase) /
           ticks_per_double_phase / 1000000.0f;
}

}  // namespace

PicoMaplePhy::PicoMaplePhy(const MaplePhyConfig& config, PIO tx_pio, PIO rx_pio)
    : config_(config),
      pin_b_(config.pin_a + 1u),
      mask_ab_(3u << config.pin_a),
      tx_pio_(tx_pio),
      rx_pio_(rx_pio),
      tx_sm_(0),
      rx_sm_(0),
      tx_program_offset_(0),
      rx_program_offset_(0),
      initialized_(false) {}
#else
PicoMaplePhy::PicoMaplePhy(const MaplePhyConfig& config)
    : config_(config), pin_b_(config.pin_a + 1u), mask_ab_(3u << config.pin_a) {}
#endif

void PicoMaplePhy::init() {
#ifdef PICO_ON_DEVICE
    if (initialized_) {
        return;
    }

    gpio_set_pulls(config_.pin_a, true, false);
    gpio_set_pulls(pin_b_, true, false);
    gpio_set_dir_in_masked(mask_ab_);
    gpio_set_function(config_.pin_a, GPIO_FUNC_SIO);
    gpio_set_function(pin_b_, GPIO_FUNC_SIO);

    tx_program_offset_ = pio_add_program(tx_pio_, &maple_out_program);
    rx_program_offset_ = pio_add_program(rx_pio_, &maple_in_program);
    tx_sm_ = pio_claim_unused_sm(tx_pio_, true);
    rx_sm_ = pio_claim_unused_sm(rx_pio_, true);

    pio_sm_config tx_config = maple_out_program_get_default_config(tx_program_offset_);
    sm_config_set_sideset_pins(&tx_config, config_.pin_a);
    sm_config_set_set_pins(&tx_config, config_.pin_a, 2);
    sm_config_set_out_shift(&tx_config, false, true, 32);
    sm_config_set_clkdiv(&tx_config, maple_tx_clkdiv(config_.sys_clock_khz, config_.ns_per_bit));
    pio_sm_init(tx_pio_, tx_sm_, tx_program_offset_, &tx_config);

    pio_sm_config rx_config = maple_in_program_get_default_config(rx_program_offset_);
    sm_config_set_in_pins(&rx_config, config_.pin_a);
    sm_config_set_jmp_pin(&rx_config, config_.pin_a);
    sm_config_set_out_shift(&rx_config, true, false, 32);
    sm_config_set_in_shift(&rx_config, false, true, 32);
    sm_config_set_clkdiv(&rx_config, 1.0f);
    pio_sm_init(rx_pio_, rx_sm_, rx_program_offset_, &rx_config);

    initialized_ = true;
#endif
}

bool PicoMaplePhy::line_is_idle(std::uint32_t idle_us) const {
#ifdef PICO_ON_DEVICE
    const absolute_time_t deadline = make_timeout_time_us(idle_us);
    while (!time_reached(deadline)) {
        if (!gpio_get(config_.pin_a) || !gpio_get(pin_b_)) {
            return false;
        }
        sleep_us(1);
    }
    return true;
#else
    (void)idle_us;
    return true;
#endif
}

bool PicoMaplePhy::write_blocking(const MaplePioTxBuffer& buffer, std::uint32_t timeout_us) {
#ifdef PICO_ON_DEVICE
    if (!initialized_ || buffer.words.empty() || !line_is_idle(config_.idle_check_us)) {
        return false;
    }

    pio_sm_set_enabled(tx_pio_, tx_sm_, false);
    pio_sm_clear_fifos(tx_pio_, tx_sm_);
    pio_sm_restart(tx_pio_, tx_sm_);
    pio_sm_clkdiv_restart(tx_pio_, tx_sm_);
    pio_sm_exec(tx_pio_, tx_sm_, pio_encode_jmp(tx_program_offset_));
    pio_sm_set_consecutive_pindirs(tx_pio_, tx_sm_, config_.pin_a, 2, false);

    pio_gpio_init(tx_pio_, config_.pin_a);
    pio_gpio_init(tx_pio_, pin_b_);
    pio_interrupt_clear(tx_pio_, tx_sm_);
    pio_sm_set_enabled(tx_pio_, tx_sm_, true);

    pio_sm_put_blocking(tx_pio_, tx_sm_, buffer.bit_count);
    for (std::uint32_t word : buffer.words) {
        pio_sm_put_blocking(tx_pio_, tx_sm_, word);
    }

    const absolute_time_t deadline = make_timeout_time_us(timeout_us);
    while (!pio_interrupt_get(tx_pio_, tx_sm_)) {
        if (time_reached(deadline)) {
            pio_sm_set_enabled(tx_pio_, tx_sm_, false);
            release_lines();
            return false;
        }
        tight_loop_contents();
    }

    pio_interrupt_clear(tx_pio_, tx_sm_);
    pio_sm_set_enabled(tx_pio_, tx_sm_, false);
    release_lines();
    return true;
#else
    (void)buffer;
    (void)timeout_us;
    return true;
#endif
}

void PicoMaplePhy::release_lines() {
#ifdef PICO_ON_DEVICE
    gpio_set_dir_in_masked(mask_ab_);
    gpio_set_pulls(config_.pin_a, true, false);
    gpio_set_pulls(pin_b_, true, false);
    gpio_set_function(config_.pin_a, GPIO_FUNC_SIO);
    gpio_set_function(pin_b_, GPIO_FUNC_SIO);
#endif
}

std::uint32_t PicoMaplePhy::pin_a() const {
    return config_.pin_a;
}

std::uint32_t PicoMaplePhy::pin_b() const {
    return pin_b_;
}

}  // namespace dreamcast::pico

