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

    // ---------------- PCB HARDWARE TEST ----------------
    void hardware_test()
    {
        printf("\n=== PCB HARDWARE TEST START ===\n");

        // ---- TEST 1: GPIO Direction Pins ----
        printf("Testing DIR pins...\n");

        gpio_put(xDIR, 1);
        sleep_ms(200);
        gpio_put(xDIR, 0);

        gpio_put(yDIR, 1);
        sleep_ms(200);
        gpio_put(yDIR, 0);

        gpio_put(zDIR, 1);
        sleep_ms(200);
        gpio_put(zDIR, 0);

        printf("DIR pin toggle test DONE\n");

        // ---- TEST 2: STEP Pulses ----
        printf("Testing STEP pulses (X/Y/Z)...\n");

        for (int i = 0; i < 200; i++)
        {
            gpio_put(xSTEP, 1);
            sleep_us(500);
            gpio_put(xSTEP, 0);
            sleep_us(500);
        }

        for (int i = 0; i < 200; i++)
        {
            gpio_put(ySTEP, 1);
            sleep_us(500);
            gpio_put(ySTEP, 0);
            sleep_us(500);
        }

        for (int i = 0; i < 200; i++)
        {
            gpio_put(zSTEP, 1);
            sleep_us(500);
            gpio_put(zSTEP, 0);
            sleep_us(500);
        }

        printf("STEP pulse test DONE\n");

        // ---- TEST 3: Motion Functions ----
        printf("Testing movement functions...\n");

        move_x(500, true);
        move_x(500, false);

        move_y(500, true);
        move_y(500, false);

        move_z(500, true);
        move_z(500, false);

        printf("Position: X=%d Y=%d Z=%d\n", x_pos, y_pos, z_pos);

        // ---- TEST 4: Spindle PWM ----
        printf("Testing spindle PWM ramp...\n");

        for (uint16_t s = 0; s <= 60000; s += 5000)
        {
            spindle_set_speed(s);
            printf("PWM = %d\n", s);
            sleep_ms(300);
        }

        spindle_set_speed(0);
        printf("Spindle OFF\n");

        // ---- TEST COMPLETE ----
        printf("=== PCB HARDWARE TEST COMPLETE ===\n");
    }
    // ---------------- MAIN ----------------
    int main(void)
    {
        stdio_init_all();
        mmhal_init();

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