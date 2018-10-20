/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Scott Shawcroft for Adafruit Industries
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "shared-bindings/microcontroller/Pin.h"

#include "nrf_gpio.h"
#include "py/mphal.h"

#include "nrf/pins.h"
#include "supervisor/shared/rgb_led_status.h"

#ifdef MICROPY_HW_NEOPIXEL
bool neopixel_in_use;
#endif
#ifdef MICROPY_HW_APA102_MOSI
bool apa102_sck_in_use;
bool apa102_mosi_in_use;
#endif
#ifdef SPEAKER_ENABLE_PIN
bool speaker_enable_in_use;
#endif

// Bit mask of claimed pins on each of up to two ports. nrf52832 has one port; nrf52840 has two.
STATIC uint32_t claimed_pins[GPIO_COUNT];
STATIC uint32_t never_reset_pins[GPIO_COUNT];

void reset_all_pins(void) {
    return;
    for (size_t i = 0; i < GPIO_COUNT; i++) {
        claimed_pins[i] = never_reset_pins[i];
    }

    for (uint32_t pin = 0; pin < NUMBER_OF_PINS; ++pin) {
        if (!(never_reset_pins[nrf_pin_port(pin)] & (1 << nrf_relative_pin_number(pin)))) {
            continue;
        }
        nrf_gpio_cfg_default(pin);
    }

    #ifdef MICROPY_HW_NEOPIXEL
    neopixel_in_use = false;
    #endif
    #ifdef MICROPY_HW_APA102_MOSI
    apa102_sck_in_use = false;
    apa102_mosi_in_use = false;
    #endif

    // After configuring SWD because it may be shared.
    #ifdef SPEAKER_ENABLE_PIN
    speaker_enable_in_use = false;
    // TODO set pin to out and turn off.
    #endif
}

// Mark pin as free and return it to a quiescent state.
void reset_pin_number(uint8_t pin_number) {
    return;
    if (pin_number == NO_PIN) {
        return;
    }

    // Clear claimed bit.
    claimed_pins[nrf_pin_port(pin_number)] &= ~(1 << nrf_relative_pin_number(pin_number));

    #ifdef MICROPY_HW_NEOPIXEL
    if (pin_number == MICROPY_HW_NEOPIXEL->number) {
        neopixel_in_use = false;
        rgb_led_status_init();
        return;
    }
    #endif
    #ifdef MICROPY_HW_APA102_MOSI
    if (pin == MICROPY_HW_APA102_MOSI->number ||
        pin == MICROPY_HW_APA102_SCK->number) {
        apa102_mosi_in_use = apa102_mosi_in_use && pin_number != MICROPY_HW_APA102_MOSI->number;
        apa102_sck_in_use = apa102_sck_in_use && pin_number != MICROPY_HW_APA102_SCK->number;
        if (!apa102_sck_in_use && !apa102_mosi_in_use) {
            rgb_led_status_init();
        }
        return;
    }
    #endif

    #ifdef SPEAKER_ENABLE_PIN
    if (pin_number == SPEAKER_ENABLE_PIN->number) {
        speaker_enable_in_use = false;
        common_hal_digitalio_digitalinout_switch_to_output(
        nrf_gpio_pin_dir_set(pin_number, NRF_GPIO_PIN_DIR_OUTPUT);
        nrf_gpio_pin_write(pin_number, false);
    }
    #endif
}


void never_reset_pin_number(uint8_t pin_number) {
    never_reset_pins[nrf_pin_port(pin_number)] |= 1 << nrf_relative_pin_number(pin_number);
}

void claim_pin(const mcu_pin_obj_t* pin) {
    // Set bit in claimed_pins bitmask.
    claimed_pins[nrf_pin_port(pin->number)] |= 1 << nrf_relative_pin_number(pin->number);

    #ifdef MICROPY_HW_NEOPIXEL
    if (pin == MICROPY_HW_NEOPIXEL) {
        neopixel_in_use = true;
    }
    #endif
    #ifdef MICROPY_HW_APA102_MOSI
    if (pin == MICROPY_HW_APA102_MOSI) {
        apa102_mosi_in_use = true;
    }
    if (pin == MICROPY_HW_APA102_SCK) {
        apa102_sck_in_use = true;
    }
    #endif

    #ifdef SPEAKER_ENABLE_PIN
    if (pin == SPEAKER_ENABLE_PIN) {
        speaker_enable_in_use = true;
    }
    #endif
}

bool common_hal_mcu_pin_is_free(const mcu_pin_obj_t *pin) {
    #ifdef MICROPY_HW_NEOPIXEL
    if (pin == MICROPY_HW_NEOPIXEL) {
        return !neopixel_in_use;
    }
    #endif
    #ifdef MICROPY_HW_APA102_MOSI
    if (pin == MICROPY_HW_APA102_MOSI) {
        return !apa102_mosi_in_use;
    }
    if (pin == MICROPY_HW_APA102_SCK) {
        return !apa102_sck_in_use;
    }
    #endif

    #ifdef SPEAKER_ENABLE_PIN
    if (pin == SPEAKER_ENABLE_PIN) {
        return !speaker_enable_in_use;
    }
    #endif

    return !(claimed_pins[nrf_pin_port(pin->number)] & (1 << nrf_relative_pin_number(pin->number)));
}
