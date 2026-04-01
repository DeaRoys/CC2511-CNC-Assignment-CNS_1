/**************************************************************
 * rev 1.0 29-Mar-2026
 * main.c- Dea
 * ***********************************************************/

#include "pico/stdlib.h"
#include <stdbool.h>
#include <stdio.h>
#include "hardware/pwm.h"
#include "mmhal.h"

bool manual_mode = true;

#define SPINDLE_PIN 15 // PWM pin for spindle

#define STEPS_PER_MM_X 80.0
#define STEPS_PER_MM_Y 80.0
#define STEPS_PER_MM_Z 400.0

// Machine state definitions
typedef enum
{
    IDLE,
    MOVING,
    ERROR
} machine_mode_t;

// Position variables
int x_pos = 0;
int y_pos = 0;
int z_pos = 0;

// Spindle variables
uint16_t spindle_speed = 0; // 0–65535 PWM level

// Current mode
machine_mode_t current_mode = IDLE;

// ---------------- PWM SETUP ----------------
void spindle_init()
{
    gpio_set_function(SPINDLE_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(SPINDLE_PIN);

    pwm_set_wrap(slice, 65535);
    pwm_set_enabled(slice, true);
}

void spindle_set_speed(uint16_t speed)
{
    pwm_set_gpio_level(SPINDLE_PIN, speed);
    spindle_speed = speed;
}

// ---------------- MOVEMENT ----------------
void move_x(int step, bool dir)
{
    current_mode = MOVING;

    gpio_put(xDIR, dir);

    for (int i = 0; i < step; i++)
    {
        gpio_put(xSTEP, 1);
        sleep_us(1200);
        gpio_put(xSTEP, 0);
        sleep_us(1200);
    }

    x_pos += (dir ? step : -step);
    current_mode = IDLE;
}

void move_y(int step, bool dir)
{
    current_mode = MOVING;

    gpio_put(yDIR, dir);

    for (int i = 0; i < step; i++)
    {
        gpio_put(ySTEP, 1);
        sleep_us(1200);
        gpio_put(ySTEP, 0);
        sleep_us(1200);
    }

    y_pos += (dir ? -step : step);
    current_mode = IDLE;
}

void move_z(int step, bool dir)
{
    current_mode = MOVING;

    gpio_put(zDIR, dir);

    for (int i = 0; i < step; i++)
    {
        gpio_put(zSTEP, 1);
        sleep_us(1200);
        gpio_put(zSTEP, 0);
        sleep_us(1200);
    }

    z_pos += (dir ? step : -step);
    current_mode = IDLE;
}

// ---------------- MANUAL CONTROL ----------------
void handle_manual_mode(int ch)
{
    switch (ch)
    {
    // X axis
    case 'a':
        move_x(1000, false);
        break;
    case 'd':
        move_x(1000, true);
        break;

    // Y axis
    case 'w':
        move_y(1000, false);
        break;
    case 's':
        move_y(1000, true);
        break;

    // Z axis
    case 'q':
        move_z(1000, false);
        break;
    case 'e':
        move_z(1000, true);
        break;

    // Spindle speed
    case '+':
        if (spindle_speed < 65000)
            spindle_speed += 2000;
        spindle_set_speed(spindle_speed);
        break;

    case '-':
        if (spindle_speed > 2000)
            spindle_speed -= 2000;
        spindle_set_speed(spindle_speed);
        break;

    case '0':
        spindle_set_speed(0);
        break;
    }
}

// ---------------- COMMAND MODE ----------------
// ---------------- COMMAND MODE ----------------
void handle_command_mode(int ch)
{
    static char cmd_buffer[64];
    static int index = 0;

    // When Enter is pressed → process command
    if (ch == '\n' || ch == '\r')
    {
        cmd_buffer[index] = '\0';

        printf("\nReceived: %s\n", cmd_buffer);

        // -------- M02: Switch back to manual mode --------
        if (strcmp(cmd_buffer, "M02") == 0)
        {
            manual_mode = true;
            printf("Switched to MANUAL mode\n");
        }

        // -------- G00: Rapid positioning --------
        else if (strncmp(cmd_buffer, "G00", 3) == 0)
        {
            float x = 0, y = 0, z = 0;

            char *ptr;

            // Parse X value
            ptr = strchr(cmd_buffer, 'X');
            if (ptr)
                x = atof(ptr + 1);

            // Parse Y value
            ptr = strchr(cmd_buffer, 'Y');
            if (ptr)
                y = atof(ptr + 1);

            // Parse Z value
            ptr = strchr(cmd_buffer, 'Z');
            if (ptr)
                z = atof(ptr + 1);

            printf("G00 Move → X: %.2f Y: %.2f Z: %.2f\n", x, y, z);

            // Convert mm → steps
            int x_steps = mm_to_steps(x, STEPS_PER_MM_X);
            int y_steps = mm_to_steps(y, STEPS_PER_MM_Y);
            int z_steps = mm_to_steps(z, STEPS_PER_MM_Z);

            // Move motors
            if (x_steps != 0)
                move_x(abs(x_steps), x_steps > 0);

            if (y_steps != 0)
                move_y(abs(y_steps), y_steps > 0);

            if (z_steps != 0)
                move_z(abs(z_steps), z_steps > 0);
        }

        else
        {
            printf("Unknown command\n");
        }

        index = 0; // reset buffer
    }
    else
    {
        // Store incoming characters
        if (index < sizeof(cmd_buffer) - 1)
        {
            cmd_buffer[index++] = ch;
        }
    }

    int mm_to_steps(float mm, float steps_per_mm)
    {
        return (int)(mm * steps_per_mm);
    }

    // ---------------- MAIN ----------------
    int main(void)
    {
        stdio_init_all();
        mmhal_init();

        // Stepper pins
        gpio_init(xSTEP);
        gpio_init(xDIR);
        gpio_set_dir(xSTEP, true);
        gpio_set_dir(xDIR, true);

        gpio_init(ySTEP);
        gpio_init(yDIR);
        gpio_set_dir(ySTEP, true);
        gpio_set_dir(yDIR, true);

        gpio_init(zSTEP);
        gpio_init(zDIR);
        gpio_set_dir(zSTEP, true);
        gpio_set_dir(zDIR, true);

        // Spindle
        spindle_init();

        hardware_test();
        while (true)
        {
            int ch = getchar_timeout_us(1000);

            if (ch != PICO_ERROR_TIMEOUT)
            {

                // Toggle mode (no LEDs now)
                if (ch == 'm')
                {
                    manual_mode = !manual_mode;
                    continue;
                }

                if (manual_mode)
                {
                    handle_manual_mode(ch);
                }
                else
                {
                    handle_command_mode(ch);
                }
            }
        }
    }