#include "mmhal.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#ifndef CNC_VERSION
#define CNC_VERSION 2
#endif

// Pin numbers arrays
const int step_pins[] = {XSTEP_PIN, YSTEP_PIN, ZSTEP_PIN};
const int dir_pins[] = {XDIR_PIN, YDIR_PIN, ZDIR_PIN};

// Multipliers for each axis, dealing with asymmetric stepper directions
#if CNC_VERSION == 1
const int stepper_multipliers[] = {-1, 1, -1};
#elif CNC_VERSION == 2
const int stepper_multipliers[] = {1, -1, 1};
#else
#error "Invalid CNC_VERSION"
#endif

volatile int mmhal_high_delay_us = 2000; // Microseconds for step pulse high time
volatile int mmhal_low_delay_us = 100;   // Microseconds for step pulse low time
uint spindle_slice = 0;                   // slice number of stepper motor

// --- SPINDLE DIRECTION PIN ---
#ifndef SPINDLE_DIR_PIN
#define SPINDLE_DIR_PIN 22  // example, adjust to your hardware
#endif

// --- Initialize GPIO pins and spindle PWM ---
void mmhal_init()
{
    // Initialize spindle PWM
    gpio_set_function(SPINDLE_PIN, GPIO_FUNC_PWM);
    gpio_set_dir(SPINDLE_DIR_PIN, GPIO_OUT); // spindle direction pin
    spindle_slice = pwm_gpio_to_slice_num(SPINDLE_PIN);
    pwm_set_enabled(spindle_slice, true);
    pwm_set_gpio_level(SPINDLE_PIN, 0);   // initial speed 0
}

// --- Set spindle PWM ---
void mmhal_set_spindle_pwm(uint16_t pwm_level)
{
    if (pwm_level > 65535) pwm_level = 65535;
    pwm_set_gpio_level(SPINDLE_PIN, pwm_level);
}

// --- Set spindle direction ---
void mmhal_set_spindle_direction(bool cw)
{
    gpio_put(SPINDLE_DIR_PIN, cw ? 1 : 0);
}

// --- Start spindle clockwise ---
void mmhal_spindle_start_cw(uint16_t S)
{
    if (S > 65535) S = 65535;
    mmhal_set_spindle_direction(true); // CW
    mmhal_set_spindle_pwm(S);
    printf("Spindle CW started with PWM: %u\n", S);
}

// --- Start spindle counter-clockwise ---
void mmhal_spindle_start_ccw(uint16_t S)
{
    if (S > 65535) S = 65535;
    mmhal_set_spindle_direction(false); // CCW
    mmhal_set_spindle_pwm(S);
    printf("Spindle CCW started with PWM: %u\n", S);
}

// --- Stop spindle ---
void mmhal_spindle_stop(void)
{
    mmhal_set_spindle_pwm(0);
    printf("Spindle stopped\n");
}

// --- Dummy microstepping function (keep your code for motors) ---
void mmhal_set_microstepping(int x_or_y, mmhal_microstep_mode_t mode)
{
    // Implementation 
}

// --- step motor functions  ---
void mmhal_step_motors_impl(int dirs[]) { /* Your motor stepping code here */ }
void mmhal_step_motors(int x_dir, int y_dir, int z_dir) { int d[3]={x_dir,y_dir,z_dir}; mmhal_step_motors_impl(d); }
void mmhal_dwell_ms(uint32_t ms) { sleep_ms(ms); }