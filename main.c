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

    if (ch == '\n' || ch == '\r')
    {
        buffer[i] = '\0';
        printf("CMD: %s\n", buffer);

        // Units
        if (strcmp(buffer, "G20") == 0)
        {
            unit_scale = INCH;
            printf("Units: INCH\n");
        }
        else if (strcmp(buffer, "G21") == 0)
        {
            unit_scale = MM;
            printf("Units: MM\n");
        }

        // Spindle
        else if (strncmp(buffer, "M3", 2) == 0)
        {
            int speed = 30000;
            char *p = strchr(buffer, 'S');
            if (p)
                speed = atoi(p + 1);
            spindle_set_speed(speed);
        }
        else if (strcmp(buffer, "M5") == 0)
        {
            spindle_set_speed(0);
        }

        // Exit
        else if (strcmp(buffer, "M02") == 0)
        {
            manual_mode = true;
            printf("Manual mode\n");
        }

        // Move
        else if (strncmp(buffer, "G00", 3) == 0 || strncmp(buffer, "G0", 2) == 0)
        {
            float x = 0, y = 0, z = 0;
            char *p;
            if ((p = strchr(buffer, 'X')))
                x = atof(p + 1) * unit_scale;
            if ((p = strchr(buffer, 'Y')))
                y = atof(p + 1) * unit_scale;
            if ((p = strchr(buffer, 'Z')))
                z = atof(p + 1) * unit_scale;

            int xs = mm_to_steps(x, STEPS_PER_MM_X);
            int ys = mm_to_steps(y, STEPS_PER_MM_Y);
            int zs = mm_to_steps(z, STEPS_PER_MM_Z);

            if (xs)
                move_x(abs(xs), xs > 0);
            if (ys)
                move_y(abs(ys), ys > 0);
            if (zs)
                move_z(abs(zs), zs > 0);
        }

        i = 0;
    }
    else
    {
        if (i < 63)
            buffer[i++] = ch;
    }

    // Inside command_control after reading a line
    if (strncmp(buffer, "G04", 3) == 0)
    {
        char *p = strchr(buffer, 'P'); // find P parameter
        if (p)
        {
            uint32_t dwell_time = atoi(p + 1); // convert to milliseconds
            mmhal_dwell_ms(dwell_time);        // pause machine
        }
    }
}
void button_control()
{
    int step = 200; // smaller step for smoother control

    // LOW = pressed
    if (!gpio_get(BTN_LEFT))
        move_x(step, false);

    if (!gpio_get(BTN_RIGHT))
        move_x(step, true);

    if (!gpio_get(BTN_FORWARD))
        move_y(step, false);

    if (!gpio_get(BTN_BACKWARD))
        move_y(step, true);

    if (!gpio_get(BTN_UP))
        move_z(step, true);

    if (!gpio_get(BTN_DOWN))
        move_z(step, false);
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

    // Set GPIO 28 HIGH (3.3V output)
    gpio_init(GPIO28_PIN);
    gpio_set_dir(GPIO28_PIN, GPIO_OUT);
    gpio_put(GPIO28_PIN, 1); // Set HIGH (3.3V)
    // Init buttons (INPUT + pull-up)
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

    spindle_init();

    printf("Hardware test running\n");
    hardware_test();

    while (true)
    {
        // Read hardware buttons continuously
        button_control();

        int ch = getchar_timeout_us(1000);

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