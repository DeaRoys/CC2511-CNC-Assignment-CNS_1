#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "hardware/pwm.h"
#include "mmhal.h"

// -------- CONSTANTS --------
#define STEPS_PER_MM_X 80.0
#define STEPS_PER_MM_Y 80.0
#define STEPS_PER_MM_Z 400.0

#define MM 1.0
#define INCH 25.4

// -------- GLOBAL VARIABLES --------
int x_pos = 0, y_pos = 0, z_pos = 0;
float unit_scale = MM;
bool manual_mode = true;
uint16_t spindle_speed = 0;

// -------- BUTTON DEFINITIONS --------
#define BTN_COUNT 6
const uint btn_pins[BTN_COUNT] = {27, 26, 4, 5, 6, 7};                      // LEFT, RIGHT, FORWARD, BACKWARD, UP, DOWN
volatile bool btn_states[BTN_COUNT] = {true, true, true, true, true, true}; // released = true
volatile bool btn_state_changed = false;

// Function prototype for moving X axis
void move_x(int steps, bool direction);

// Step size for each button press
#define STEP_SIZE 100

// -------- BASIC FUNCTIONS --------
int mm_to_steps(float mm, float steps_per_mm)
{
    return (int)(mm * steps_per_mm);
}

void step_motor(int pin)
{
    gpio_put(pin, 1);
    sleep_us(1200);
    gpio_put(pin, 0);
    sleep_us(1200);
}

// -------- MOVE FUNCTIONS --------
void move_x(int steps, bool dir)
{
    gpio_put(XDIR_PIN, dir);
    for (int i = 0; i < steps; i++)
        step_motor(XSTEP_PIN);
    x_pos += (dir ? steps : -steps);
}

void move_y(int steps, bool dir)
{
    gpio_put(YDIR_PIN, dir);
    for (int i = 0; i < steps; i++)
        step_motor(YSTEP_PIN);
    y_pos += (dir ? -steps : steps);
}

void move_z(int steps, bool dir)
{
    gpio_put(ZDIR_PIN, dir);
    for (int i = 0; i < steps; i++)
        step_motor(ZSTEP_PIN);
    z_pos += (dir ? steps : -steps);
}

// -------- SPINDLE --------
void spindle_init()
{
    gpio_set_function(SPINDLE_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(SPINDLE_PIN);
    pwm_set_wrap(slice, 65535);
    pwm_set_enabled(slice, true);
}

void spindle_set_speed(uint16_t speed)
{
    if (speed > 65535)
        speed = 65535;
    uint slice = pwm_gpio_to_slice_num(SPINDLE_PIN); // make sure we get slice
    pwm_set_gpio_level(SPINDLE_PIN, speed);
    pwm_set_enabled(slice, true); // ensure slice is enabled
    spindle_speed = speed;
}

// -------- HARDWARE TEST --------
void hardware_test()
{
    printf("\n--- HARDWARE TEST START ---\n");

    printf("Testing X axis\n");
    move_x(500, true);
    sleep_ms(500);
    move_x(500, false);

    printf("Testing Y axis\n");
    move_y(500, true);
    sleep_ms(500);
    move_y(500, false);

    printf("Testing Z axis\n");
    move_z(500, true);
    sleep_ms(500);
    move_z(500, false);

    printf("Testing spindle\n");
    spindle_set_speed(30000);
    sleep_ms(1000);
    spindle_set_speed(0);

    printf("--- TEST COMPLETE ---\n\n");
}

// -------- MANUAL MODE --------
void manual_control(int ch)
{
    int step = 500;

    // Capital letters = bigger move
    if (ch >= 'A' && ch <= 'Z')
    {
        step = 2000;
        ch += 32;
    }

    switch (ch)
    {
    // Movement
    case 'a':
        move_x(step, false);
        break; // left
    case 'd':
        move_x(step, true);
        break; // right
    case 'w':
        move_y(step, false);
        break; // up
    case 's':
        move_y(step, true);
        break; // down
    case 'q':
        move_z(step, false);
        break; // back
    case 'e':
        move_z(step, true);
        break; // forward
    case 't':  // test button or key
        printf("Starting spindle clockwise\n");
        mmhal_spindle_start_cw(30000); // PWM 30k
        break;

    case 'r': // CCW
        printf("Starting spindle anti-clockwise\n");
        mmhal_spindle_start_ccw(30000);
        break;

    case 'y': // stop key
        mmhal_spindle_stop();
        break;

        // Pause
    case 'p':
        printf("Pausing machine for 1 second\n");
        mmhal_dwell_ms(1000); // 1000 ms pause
        break;

    // Mode switch
    case 'm':
        manual_mode = false;
        printf("Command mode\n");
        break;
    }
}

// -------- COMMAND MODE --------
void command_control(int ch)
{
    static char buffer[64];
    static int i = 0;

    // ESC → instant manual mode
    if (ch == 27)
    {
        manual_mode = true;
        printf("Manual mode (ESC)\n");
        i = 0;
        return;
    }
}
void setup_buttons()
{
    // Initialize button pins as input with pull-up resistors
    gpio_init(BTN_LEFT);
    gpio_set_dir(BTN_LEFT, GPIO_IN);
    gpio_pull_up(BTN_LEFT);

    gpio_init(BTN_RIGHT);
    gpio_set_dir(BTN_RIGHT, GPIO_IN);
    gpio_pull_up(BTN_RIGHT);

    gpio_init(BTN_FORWARD);
    gpio_set_dir(BTN_FORWARD, GPIO_IN);
    gpio_pull_up(BTN_FORWARD);

    gpio_init(BTN_BACKWARD);
    gpio_set_dir(BTN_BACKWARD, GPIO_IN);
    gpio_pull_up(BTN_BACKWARD);

    gpio_init(BTN_UP);
    gpio_set_dir(BTN_UP, GPIO_IN);
    gpio_pull_up(BTN_UP);

    gpio_init(BTN_DOWN);
    gpio_set_dir(BTN_DOWN, GPIO_IN);
    gpio_pull_up(BTN_DOWN);
}

// Reads button states and moves X axis accordingly
void read_buttons()
{
    // Buttons are active-low (pressed = 0)
    bool left_pressed = (gpio_get(BTN_LEFT) == 0);
    bool right_pressed = (gpio_get(BTN_RIGHT) == 0);
    bool forward_pressed = (gpio_get(BTN_FORWARD) == 0);
    bool backward_pressed = (gpio_get(BTN_BACKWARD) == 0);
    bool up_pressed = (gpio_get(BTN_UP) == 0);
    bool down_pressed = (gpio_get(BTN_DOWN) == 0);

    if (left_pressed)
    {
        move_x(STEP_SIZE, false); // Move X left
        printf("Moving X left by %d steps\n", STEP_SIZE);
    }

    if (right_pressed)
    {
        move_x(STEP_SIZE, true); // Move X right
        printf("Moving X right by %d steps\n", STEP_SIZE);
    }

    if (forward_pressed)
    {
        move_y(STEP_SIZE, true); // forward = positive Y
        printf("Moving Y forward by %d steps\n", STEP_SIZE);
    }
    if (backward_pressed)
    {
        move_y(STEP_SIZE, false); // backward = negative Y
        printf("Moving Y backward by %d steps\n", STEP_SIZE);
    }
    if (up_pressed)
    {
        move_z(STEP_SIZE, true); // up = positive Z
        printf("Moving Z up by %d steps\n", STEP_SIZE);
    }
    if (down_pressed)
    {
        move_z(STEP_SIZE, false); // down = negative Z
        printf("Moving Z down by %d steps\n", STEP_SIZE);
    }
}

// -------- MAIN --------
int main()
{
    stdio_init_all();
    sleep_ms(2000);

    // Init pins
    gpio_init(XSTEP_PIN);
    gpio_set_dir(XSTEP_PIN, 1);
    gpio_init(XDIR_PIN);
    gpio_set_dir(XDIR_PIN, 1);

    gpio_init(YSTEP_PIN);
    gpio_set_dir(YSTEP_PIN, 1);
    gpio_init(YDIR_PIN);
    gpio_set_dir(YDIR_PIN, 1);

    gpio_init(ZSTEP_PIN);
    gpio_set_dir(ZSTEP_PIN, 1);
    gpio_init(ZDIR_PIN);
    gpio_set_dir(ZDIR_PIN, 1);

    // Enable stepper drivers
    gpio_init(ENABLE_PIN);
    gpio_set_dir(ENABLE_PIN, 1);
    gpio_put(ENABLE_PIN, 0); // LOW = enabled

    // Set GPIO 28 HIGH
    gpio_init(GPIO28_PIN);
    gpio_set_dir(GPIO28_PIN, GPIO_OUT);
    gpio_put(GPIO28_PIN, 1); // Set HIGH (3.3V)

    // Init buttons
    gpio_init(BTN_LEFT);
    gpio_set_dir(BTN_LEFT, GPIO_IN);
    gpio_pull_up(BTN_LEFT);

    gpio_init(BTN_RIGHT);
    gpio_set_dir(BTN_RIGHT, GPIO_IN);
    gpio_pull_up(BTN_RIGHT);

    gpio_init(BTN_FORWARD);
    gpio_set_dir(BTN_FORWARD, GPIO_IN);
    gpio_pull_up(BTN_FORWARD);

    gpio_init(BTN_BACKWARD);
    gpio_set_dir(BTN_BACKWARD, GPIO_IN);
    gpio_pull_up(BTN_BACKWARD);

    gpio_init(BTN_UP);
    gpio_set_dir(BTN_UP, GPIO_IN);
    gpio_pull_up(BTN_UP);

    gpio_init(BTN_DOWN);
    gpio_set_dir(BTN_DOWN, GPIO_IN);
    gpio_pull_up(BTN_DOWN);

    // Initialize spindle PWM
    spindle_init();

    // Setup buttons
    setup_buttons();

    // Hardware test
    printf("Hardware test running\n");
    hardware_test();

    while (true)
    {
        read_buttons();
        sleep_ms(50);

        int ch = getchar_timeout_us(100);

        if (ch != PICO_ERROR_TIMEOUT)
        {
            if (manual_mode)
                manual_control(ch);
            else
                command_control(ch);

            printf("Mode:%s | X:%d Y:%d Z:%d | Spindle:%d\n",
                   manual_mode ? "MANUAL" : "CMD",
                   x_pos, y_pos, z_pos, spindle_speed);
        }
    }
}