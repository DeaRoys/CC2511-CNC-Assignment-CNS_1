#include "mmhal.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#ifndef CNC_VERSION
#define CNC_VERSION 2
#endif

// Pin numbers arrays
const int step_pins[] = {XSTEP_PIN, YSTEP_PIN, ZSTEP_PIN};
const int dir_pins[] = {XDIR_PIN, YDIR_PIN, ZDIR_PIN};

// Multipliers for each axis, dealing with assymetric stepper directions
#if CNC_VERSION == 1
const int stepper_multipliers[] = {-1, 1, -1};
#elif CNC_VERSION == 2
const int stepper_multipliers[] = {1, -1, 1};
#else
#error "Invalid CNC_VERSION"
#endif

volatile int mmhal_high_delay_us = 2000; // Microseconds for step pulse high time
volatile int mmhal_low_delay_us = 100;   // Microseconds for step pulse low time
uint spindle_slice = 0; // slice number of stepper mottor

/**
 * @brief Initialize GPIO pins and PWM for spindle control
 */
void mmhal_init()
{
  // TODO - Initialise GPIO pins
  for (int i = 0; i < DIMCOUNT; i++)
  {
    gpio_init(ENABLE_PIN);
    gpio_set_dir(ENABLE_PIN, GPIO_OUT);

    // initalising step pins
    gpio_init(step_pins[i]);
    gpio_set_dir(step_pins[i], GPIO_OUT);

    // initalising dir pins
    gpio_init(dir_pins[i]);
    gpio_set_dir(dir_pins[i], GPIO_OUT);
  
    // TODO - Initialise the pins for X, Y & Z steppers, using the
    // step_pins and dir_pins arrays
  }
  // initalising spindle for PWM
  gpio_set_function(SPINDLE_PIN, GPIO_FUNC_PWM);
  spindle_slice = pwm_gpio_to_slice_num(SPINDLE_PIN);
  pwm_set_enabled(spindle_slice, true);
  pwm_set_gpio_level(SPINDLE_PIN, 0);   // set inistial speed to off/0
  // TODO - Initialize spindle PWM
}

void mmhal_set_spindle_pwm(uint16_t pwm_level)
{
  pwm_set_gpio_level(SPINDLE_PIN, pwm_level);
  // TODO - Implement spindle PWM setting
}
void mmhal_spindle_start_cw(uint16_t S)
{
    if (S > 65535) S = 65535;    // Clamp to max PWM
    mmhal_set_spindle_pwm(S);     // Set PWM on SPINDLE_PIN
    printf("Spindle CW started with PWM: %u\n", S);
}

void mmhal_spindle_start_ccw(uint16_t S)
{
    if (S > 65535) S = 65535;    // Clamp to max PWM
    
    mmhal_set_spindle_direction(0); // 0 = CCW (adjust based on your hardware)
    mmhal_set_spindle_pwm(S);       // Set PWM on SPINDLE_PIN
    
    printf("Spindle CCW started with PWM: %u\n", S);
}

/**
 * @brief Stop spindle (M05)
 */
void mmhal_spindle_stop(void)
{
    mmhal_set_spindle_pwm(0);     // Stop spindle
    printf("Spindle stopped\n");
}
void mmhal_set_microstepping(int x_or_y, mmhal_microstep_mode_t mode)
{
  if (x_or_y){  // if 1/true => y axis
    switch (mode){
      case MMHAL_MS_MODE_1:
        gpio_put(Y_MODE2_PIN, 0);
        gpio_put(Y_MODE1_PIN, 0);
        gpio_put(Y_MODE0_PIN, 0);
        break;
      case MMHAL_MS_MODE_2:
        gpio_put(Y_MODE2_PIN, 0);
        gpio_put(Y_MODE1_PIN, 0);
        gpio_put(Y_MODE0_PIN, 1);
        break;
      case MMHAL_MS_MODE_4:
        gpio_put(Y_MODE2_PIN, 0);
        gpio_put(Y_MODE1_PIN, 1);
        gpio_put(Y_MODE0_PIN, 0);
        break;
      case MMHAL_MS_MODE_8:
        gpio_put(Y_MODE2_PIN, 0);
        gpio_put(Y_MODE1_PIN, 1);
        gpio_put(Y_MODE0_PIN, 1);
        break;
      case MMHAL_MS_MODE_16:
        gpio_put(Y_MODE2_PIN, 1);
        gpio_put(Y_MODE1_PIN, 0);
        gpio_put(Y_MODE0_PIN, 0);
        break;
      case MMHAL_MS_MODE_32:
        gpio_put(Y_MODE2_PIN, 1);
        gpio_put(Y_MODE1_PIN, 0);
        gpio_put(Y_MODE0_PIN, 1);
        break;
    }
  }
  else{ // if false/0 => x axis
    switch (mode){
      case MMHAL_MS_MODE_1:
        gpio_put(X_MODE2_PIN, 0);
        gpio_put(X_MODE1_PIN, 0);
        gpio_put(X_MODE0_PIN, 0);
        break;
      case MMHAL_MS_MODE_2:
        gpio_put(X_MODE2_PIN, 0);
        gpio_put(X_MODE1_PIN, 0);
        gpio_put(X_MODE0_PIN, 1);
        break;
      case MMHAL_MS_MODE_4:
        gpio_put(X_MODE2_PIN, 0);
        gpio_put(X_MODE1_PIN, 1);
        gpio_put(X_MODE0_PIN, 0);
        break;
      case MMHAL_MS_MODE_8:
        gpio_put(X_MODE2_PIN, 0);
        gpio_put(X_MODE1_PIN, 1);
        gpio_put(X_MODE0_PIN, 1);
        break;
      case MMHAL_MS_MODE_16:
        gpio_put(X_MODE2_PIN, 1);
        gpio_put(X_MODE1_PIN, 0);
        gpio_put(X_MODE0_PIN, 0);
        break;
      case MMHAL_MS_MODE_32:
        gpio_put(X_MODE2_PIN, 1);
        gpio_put(X_MODE1_PIN, 0);
        gpio_put(X_MODE0_PIN, 1);
        break;
    }
  }
  // TODO - Implement microstepping mode setting
}

/**
 * @brief Run motors in specified directions
 * @param dirs Array of 3 directions: -1 for negative,
 * 0 for no movement, 1 for positive
 */
void mmhal_step_motors_impl(int dirs[])
{
  for (int i; i < 3; i++){
    if (dirs[i] == 1){   // if the the value at the postion in the array is foward
      if (i == 0){    // x direction
        gpio_put(XDIR_PIN, 1);    // set direction to foward
        sleep_us(10);
        gpio_put(XSTEP_PIN, 1);
        sleep_us(mmhal_high_delay_us);
        gpio_put(XSTEP_PIN, 0);
        sleep_us(mmhal_low_delay_us);
        // code
        break;
      }
      else if (i == 1){
        gpio_put(YDIR_PIN, 1);    // y direction
        sleep_us(10);
        gpio_put(YSTEP_PIN, 1);
        sleep_us(mmhal_high_delay_us);
        gpio_put(YSTEP_PIN, 0);
        sleep_us(mmhal_low_delay_us);
        break;
      }
      else{   // z direction
        gpio_put(ZDIR_PIN, 1);    // set direction forward
        sleep_us(10);
        gpio_put(ZSTEP_PIN, 1);
        sleep_us(mmhal_high_delay_us);
        gpio_put(ZSTEP_PIN, 0);
        sleep_us(mmhal_low_delay_us);
        break;
      }
    }
    else if (dirs[i] == -1){    // if the value at the postion in the array is backwards
      if (i == 0){    // x direction
        gpio_put(XDIR_PIN, 0);
        sleep_us(10);
        gpio_put(XSTEP_PIN, 1);
        sleep_us(mmhal_high_delay_us);
        gpio_put(XSTEP_PIN, 0);
        sleep_us(mmhal_low_delay_us);
        break;
      }
      else if (i == 1){   // y direction
        gpio_put(YDIR_PIN, 0);
        sleep_us(10);
        gpio_put(YSTEP_PIN, 1);
        sleep_us(mmhal_high_delay_us);
        gpio_put(YSTEP_PIN, 0);
        sleep_us(mmhal_low_delay_us);
        break;
      }
      else{   // z direction
        gpio_put(ZDIR_PIN, 0);
        sleep_us(10);
        gpio_put(ZSTEP_PIN, 1);
        sleep_us(mmhal_high_delay_us);
        gpio_put(ZSTEP_PIN, 0);
        sleep_us(mmhal_low_delay_us);
        break;
      }
    } // else assume its 0 and do nothing
  }
  // TODO - Implement motor stepping logic, using the dirs array
  // to determine which motors to step and in which direction
  // Remember to use the stepper_multipliers array to handle
  // asymmetric stepper directions

  // TODO - Implement the timing for the step pulses, using
  // mmhal_high_delay_us and mmhal_low_delay_us for the pulse timing
}

void mmhal_step_motors(int x_dir, int y_dir, int z_dir)
{
  int dirs[3] = {x_dir, y_dir, z_dir};
  mmhal_step_motors_impl(dirs);
}

void mmhal_dwell_ms(uint32_t ms) {
    sleep_ms(ms);
}