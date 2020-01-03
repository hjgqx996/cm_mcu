/*
 * SoftUartTask.c
 *
 *  Created on: Jan 3, 2020
 *      Author: wittich
 */
// includes for types
#include <stdint.h>
#include <stdbool.h>

// to be moved
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/gpio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/rom.h"

#include "common/softuart.h"

#include "Tasks.h"

//
// The buffer used to hold the transmit data.
//
unsigned char g_pucTxBuffer[16];
//
// The buffer used to hold the receive data.
//
//unsigned short g_pusRxBuffer[16];

// The number of processor clocks in the time period of a single bit on the
// software UART interface.
//
unsigned long g_ulBitTime;

tSoftUART g_sUART;

extern uint32_t g_ui32SysClock;

void SoftUartTask(void *parameters)
{
  // Setup
  // initialize to the current tick time
  TickType_t xLastWakeTime = xTaskGetTickCount();

  // Initialize the software UART instance data.
  //
  SoftUARTInit(&g_sUART);
  //
  // Configure the pins used for the software UART.  This example uses
  // pins PD0 and PE1.
  //
  SoftUARTTxGPIOSet(&g_sUART, GPIO_PORTD_BASE, GPIO_PIN_0);
  SoftUARTRxGPIOSet(&g_sUART, GPIO_PORTE_BASE, GPIO_PIN_1);
  //
  // Configure the data buffers used as the transmit and receive buffers.
  //
  SoftUARTTxBufferSet(&g_sUART, g_pucTxBuffer, 16);
  //SoftUARTRxBufferSet(&g_sUART, g_pusRxBuffer, 16);
  //
  // Enable the GPIO modules that contains the GPIO pins to be used by
  // the software UART.
  //
  //ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
  //ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
  //
  // Configure the software UART module: 8 data bits, no parity, and one
  // stop bit.
  //
  SoftUARTConfigSet(&g_sUART,
                    (SOFTUART_CONFIG_WLEN_8 | SOFTUART_CONFIG_PAR_NONE |
                     SOFTUART_CONFIG_STOP_ONE));
  //
  // Compute the bit time for 38,400 baud.
  //
  g_ulBitTime = (g_ui32SysClock / 38400) - 1;
  //
  // Configure the timers used to generate the timing for the software
  // UART.  The interface in this example is run at 38,400 baud,
  // requiring a timer tick at 38,400 Hz.
  //
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
  ROM_TimerConfigure(TIMER0_BASE,
                 (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC |
                  TIMER_CFG_B_PERIODIC));
  ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, g_ulBitTime);
  ROM_TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT | TIMER_TIMB_TIMEOUT);
  ROM_TimerEnable(TIMER0_BASE, TIMER_A);
  //
  // Set the priorities of the interrupts associated with the software
  // UART.  The receiver is higher priority than the transmitter, and the
  // receiver edge interrupt is higher priority than the receiver timer
  // interrupt.
  //
  //ROM_IntPrioritySet(INT_GPIOE, 0x00);
  //ROM_IntPrioritySet(INT_TIMER0B, 0x40);
  ROM_IntPrioritySet(INT_TIMER0A, 0x80);
  //
  // Enable the interrupts associated with the software UART.
  //
  //ROM_IntEnable(INT_GPIOE);
  ROM_IntEnable(INT_TIMER0A);
  //ROM_IntEnable(INT_TIMER0B);
  //
  // Enable the transmit FIFO half full interrupt in the software UART.
  //
  SoftUARTIntEnable(&g_sUART, SOFTUART_INT_TX);

  uint8_t vals[] = {0xA5, 0x5a, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

  // Loop forever
  for (;;) {

    // send data buffer
    for (int i = 0; i < 8; ++i ) {
      SoftUARTCharPut(&g_sUART, vals[i]);
    }


    // wait here for the x msec, where x is 2nd argument below.
    vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS( 5000 ) );
  }

  return;
}
