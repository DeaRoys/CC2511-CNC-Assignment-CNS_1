/**************************************************************
 * main.c
 * rev 1.0 29-Mar-2026 
 * main.c
 * ***********************************************************/

#include "pico/stdlib.h"
#include <stdbool.h>
#include <stdio.h> 
#include "hardware/pwm.h"
#include "mmhal.h"

bool manual_mode = true;

#define RED_LED 11
#define GREEN_LED 12
#define BLUE_LED 13

#define SPINDLE_PIN 15   // PWM pin for spindle

// Machine state definitions
typedef enum {
    IDLE,
    MOVING,
    ERROR
} machine_mode_t;

// Position variables
int x_pos = 0;
int y_pos = 0;
int z_pos = 0;

// Spindle variables
uint16_t spindle_speed = 0;   // 0–65535 PWM level

// Current mode
machine_mode_t current_mode = IDLE;

// ---------------- PWM SETUP ----------------
void spindle_init() {
    gpio_set_function(SPINDLE_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(SPINDLE_PIN);

    pwm_set_wrap(slice, 65535);
    pwm_set_enabled(slice, true);
}

void spindle_set_speed(uint16_t speed) {
    uint slice = pwm_gpio_to_slice_num(SPINDLE_PIN);
    pwm_set_gpio_level(SPINDLE_PIN, speed);
    spindle_speed = speed;
}

// ---------------- MOVEMENT ----------------
void move_x(int step, bool dir){
    current_mode = MOVING;

    gpio_put(xDIR,dir);

    for(int i= 0; i<step; i++){
        gpio_put(xSTEP,1);
        sleep_us(1200);
        gpio_put(xSTEP,0);
        sleep_us(1200);
    }

    x_pos += (dir ? step : -step);
    current_mode = IDLE;
}

void move_y(int step, bool dir){
    current_mode = MOVING;

    gpio_put(yDIR,dir);

    for(int i= 0; i<step; i++){
        gpio_put(ySTEP,1);
        sleep_us(1200);
        gpio_put(ySTEP,0);
        sleep_us(1200);
    }

    y_pos += (dir ? -step : step);
    current_mode = IDLE;
}

void move_z(int step, bool dir){
    current_mode = MOVING;

    gpio_put(zDIR,dir);

    for(int i= 0; i<step; i++){
        gpio_put(zSTEP,1);
        sleep_us(1200);
        gpio_put(zSTEP,0);
        sleep_us(1200);
    }

    z_pos += (dir ? step : -step);
    current_mode = IDLE;
}

// ---------------- MANUAL CONTROL ----------------
void handle_manual_mode(int ch) {
    switch(ch){
        // X axis
        case 'a': move_x(1000,false); break; // left
        case 'd': move_x(1000,true);  break; // right

        // Y axis
        case 'w': move_y(1000,false); break; // up
        case 's': move_y(1000,true);  break; // down

        // Z axis
        case 'q': move_z(1000,false); break; // back
        case 'e': move_z(1000,true);  break; // forward

        // Spindle speed
        case '+':
            if (spindle_speed < 65000) spindle_speed += 2000;
            spindle_set_speed(spindle_speed);
            break;

        case '-':
            if (spindle_speed > 2000) spindle_speed -= 2000;
            spindle_set_speed(spindle_speed);
            break;

        case '0':
            spindle_set_speed(0);
            break;
    }
}

// ---------------- COMMAND MODE (placeholder) ----------------
void handle_command_mode(int ch) {
    // Future G-code or structured command parser
}

// ---------------- MAIN ----------------
int main(void)
{
    stdio_init_all();
    mmhal_init();

    // LEDs
    gpio_init(RED_LED);
    gpio_init(GREEN_LED);
    gpio_init(BLUE_LED);

    gpio_set_dir(RED_LED, true);
    gpio_set_dir(GREEN_LED, true);
    gpio_set_dir(BLUE_LED, true);

    // Stepper pins
    gpio_init(xSTEP); gpio_init(xDIR);
    gpio_set_dir(xSTEP, true); gpio_set_dir(xDIR, true);

    gpio_init(ySTEP); gpio_init(yDIR);
    gpio_set_dir(ySTEP, true); gpio_set_dir(yDIR, true);

    gpio_init(zSTEP); gpio_init(zDIR);
    gpio_set_dir(zSTEP, true); gpio_set_dir(zDIR, true);

    // Spindle
    spindle_init();

    while (true)
    {
        int ch = getchar_timeout_us(1000);

        if (ch != PICO_ERROR_TIMEOUT){

            // Toggle mode with single key
            if (ch == 'm') {
                manual_mode = !manual_mode;

                if (manual_mode) {
                    gpio_put(GREEN_LED, 1);
                    gpio_put(RED_LED, 0);
                } else {
                    gpio_put(RED_LED, 1);
                    gpio_put(GREEN_LED, 0);
                }
                continue;
            }

            if (manual_mode) {
                handle_manual_mode(ch);
            } else {
                handle_command_mode(ch);
            }
        }
    }
}