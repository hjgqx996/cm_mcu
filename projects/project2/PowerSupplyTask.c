/*
 * PowerSupplyTask.c
 *
 *  Created on: May 13, 2019
 *      Author: wittich
 */

// includes for types
#include <stdint.h>
#include <stdbool.h>

// local includes
#include "common/uart.h"
#include "common/utils.h"
#include "common/power_ctl.h"
#include "common/i2c_reg.h"
#include "common/pinout.h"
#include "common/pinsel.h"

// FreeRTOS includes
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
//#include "task.h"
#include "queue.h"
//#include "stream_buffer.h"
//#include "semphr.h"
//#include "portmacro.h"



// Holds the handle of the created queue for the power supply task.
QueueHandle_t xPwrQueue = NULL;

extern QueueHandle_t xLedQueue;

void Print(const char* str);



// monitor and control the power supplies
void PowerSupplyTask(void *parameters)
{
  // initialize to the current tick time
  TickType_t xLastWakeTime = xTaskGetTickCount();
  enum state { PWR_ON, PWR_OFF, UNKNOWN};
  enum state oldState = UNKNOWN;

  // turn on the power supply at the start of the task
  if ( set_ps(true,true,true) ) {
    oldState = PWR_ON;
  }
  else {
    oldState = PWR_OFF;
  }

  // this function never returns
  for ( ;; ) {
    // first check for message on the queue and act on any messages.
    // non-blocking call.
    uint32_t message;
    if ( xQueueReceive(xPwrQueue, &message, 0) ) {
      switch (message ) {
      case PS_OFF:
        disable_ps();
        break;
      case PS_ON:
        set_ps(true,true,true);
        break;
      default:
        toggle_gpio_pin(TM4C_LED_RED); // message I don't understand? Toggle blue LED
        break;
      }
    }
    // now check the actual state
    bool psgood = check_ps();
    enum state newstate = psgood?PWR_ON:PWR_OFF;

    if ( newstate == PWR_OFF  && oldState == PWR_ON) {
      Print("\nPowerSupplyTask: power supplied turned off.\n");
      message = PS_BAD;
    }
    else { // all good
      message = PS_GOOD;
    }
    // only send a message on state change.
    if ( oldState != newstate )
      xQueueSendToBack(xLedQueue, &message, pdMS_TO_TICKS(10));// todo: check on fail to send
    oldState = newstate;
    vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS( 250 ) );
  }
}