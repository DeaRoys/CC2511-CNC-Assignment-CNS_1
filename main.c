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

// Current mode
machine_mode_t current_mode = IDLE;

void set_pins(bool step, bool dir){

  gpio_put(xDIR, dir);
  gpio_put(xSTEP, step);
  gpio_put(yDIR,dir);
  gpio_put(ySTEP,step);
  gpio_put(zDIR,dir);
  gpio_put(zSTEP,step);
  
}

void move_x(int step, bool dir){
  current_mode = MOVING;

  gpio_put(xDIR,dir); //put direction

  for(int i= 0; i<step; i++){
    gpio_put(xSTEP,1);
    sleep_us(1200);
    gpio_put(xSTEP,0);
    sleep_us(1200);
  }

  // Update position
  if(dir == true){
    x_pos += step;   // right
  } else {
    x_pos -= step;   // left
  }

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

  if(dir == true){
    y_pos -= step;   // down
  } else {
    y_pos += step;   // up
  }

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

  if(dir == true){
    z_pos += step;   // forward
  } else {
    z_pos -= step;   // back
  }

  current_mode = IDLE;
}

int main(void)
{
  // Initialise components and variables
  stdio_init_all();
  mmhal_init();
      // Initialise stdio
  gpio_init(RED_LED);   //GPIO initialisation
  gpio_init(GREEN_LED); 
  gpio_init(BLUE_LED);

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

  bool step, dir = false;

  gpio_set_dir(RED_LED, true);
  gpio_set_dir(BLUE_LED, true);
  gpio_set_dir(GREEN_LED, true);

  while (true)
  {
   
  int ch1 = getchar_timeout_us(1000);
    if (ch1 != PICO_ERROR_TIMEOUT){
     switch(ch1){
      case '1': 
          gpio_put(RED_LED,1);
          gpio_put(GREEN_LED,0);
          gpio_put(BLUE_LED,0);
          move_x(2000,false);   //move left
          break;
        
      case '2':  
          gpio_put(RED_LED,1);
          gpio_put(GREEN_LED,0);
          gpio_put(BLUE_LED,0);
          move_x(2000,true);    //move right
          break;

      case '3':
          gpio_put(GREEN_LED,1);
           gpio_put(RED_LED,0);
          gpio_put(BLUE_LED,0);
          move_y(1000,false);  //move up
          break;
        
      case '4':  
          gpio_put(GREEN_LED,1);
          gpio_put(RED_LED,0);
          gpio_put(BLUE_LED,0);
          move_y(1000,true);   //move down
          break;

      case '5':
          gpio_put(BLUE_LED,1);
          gpio_put(GREEN_LED,0);
           gpio_put(RED_LED,0);
          move_z(1000,false);  //move back
          break;
        
      case '6':  
          gpio_put(BLUE_LED,1);
          gpio_put(GREEN_LED,0);
           gpio_put(RED_LED,0);
          move_z(1000,true);   //move forward
          break;  
      }
      
    }
    if (manual_mode)
    {
      // handle_manual_mode();
    }
    else
    {
      // handle_command_mode();
    }
  }
  sleep_ms(20);
}