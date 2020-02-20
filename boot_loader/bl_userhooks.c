/*
 * bl_userhooks.c
 *
 *  Created on: Sep 26, 2019
 *      Author: pw94
 */
#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/hw_uart.h"
#include "inc/hw_types.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"

#include "boot_loader/bl_config.h"
#include "boot_loader/bl_uart.h"

#include "common/microrl.h"
#include "boot_loader/rl_config.h"


#include "common/uart.h"

//*****************************************************************************
//
// A prototype for the function (in the startup code) for a predictable length
// delay.
//
//*****************************************************************************
extern void Delay(uint32_t ui32Count);

void ConfigureDevice(void);

#ifdef BL_HW_INIT_FN_HOOK
void
bl_user_init_hw_fn(void)
{

  // LEDs
  //
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);


  // Configure the GPIO Pin Mux for PJ0
  // for GPIO_PJ0
  //
  MAP_GPIOPinTypeGPIOOutput(GPIO_PORTJ_BASE, GPIO_PIN_0);

  //
  // Configure the GPIO Pin Mux for PJ1
  // for GPIO_PJ1
  //
  MAP_GPIOPinTypeGPIOOutput(GPIO_PORTJ_BASE, GPIO_PIN_1);
  // Red LED
  // Configure the GPIO Pin Mux for PP0
  // for GPIO_PP0
  //
  MAP_GPIOPinTypeGPIOOutput(GPIO_PORTP_BASE, GPIO_PIN_0);


  return;
}
#endif // BL_INIT_FN_HOOK

#ifdef BL_END_FN_HOOK
#define LONG_DELAY 2500000

// Flash green LED 3 times
void bl_user_end_hook()
{
  MAP_GPIOPinWrite(GPIO_PORTJ_BASE, GPIO_PIN_1, 1);
  Delay(LONG_DELAY);
  MAP_GPIOPinWrite(GPIO_PORTJ_BASE, GPIO_PIN_1, 0);
  Delay(LONG_DELAY);
  MAP_GPIOPinWrite(GPIO_PORTJ_BASE, GPIO_PIN_1, 1);
  Delay(LONG_DELAY);
  MAP_GPIOPinWrite(GPIO_PORTJ_BASE, GPIO_PIN_1, 0);
  return;
}
#endif // BL_END_FN_HOOK

#ifdef BL_PROGRESS_FN_HOOK

void bl_user_progress_hook(unsigned long ulCompleted, unsigned long ulTotal)
{
  int tens = (10*ulCompleted/ulTotal);
  MAP_GPIOPinWrite(GPIO_PORTJ_BASE, GPIO_PIN_0, tens%2);
  return;
}
#endif // BL_PROGRESS_FN_HOOK


#ifdef BL_FLASH_ERROR_FN_HOOK
void bl_user_flash_error()
{
//  MAP_GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_0, 1);
//  Delay(LONG_DELAY);

  return;
}
#endif // BL_FLASH_ERROR_FN_HOOK

#if 0
// UART receive with timeout
static
int
bl_user_UARTReceive_timeout(uint8_t *pui8Data, uint32_t ui32Size, uint32_t timeout)
{
  timeout = (timeout<0?-timeout:timeout);
  //
  // Send out the number of bytes requested.
  //
  while(ui32Size--)
  {
    //
    // Wait for the FIFO to not be empty.
    //
    while ((HWREG(UARTx_BASE + UART_O_FR) & UART_FR_RXFE))
    {
      if ( timeout-- <=0 )
        break;
    }
    if ( timeout <0)
      return 0;

    //
    // Receive a byte from the UART.
    //
    *pui8Data++ = HWREG(UARTx_BASE + UART_O_DR);
  }
  return ui32Size;
}
#endif

#ifdef BL_CHECK_UPDATE_FN_HOOK
// User hook to force an update
void bl_user_uart_print(const char* str)
{
  UARTPrint(UARTx_BASE, str);
  return ;
}

struct bl_user_data_t {
  bool done;
  int retval;
};

static const char * helpstr =
    "h\r\n\tThis help.\r\n"
    "b\r\n\tStart normal boot process\r\n"
    "r\r\n\tRestart MCU\r\n"
    "u\r\n\tForce update\r\n"
    ;

int bl_user_rl_execute(void *d, int argc, char **argv)
{
  struct bl_user_data_t *data = d;

  if ( argc > 1 )  {
    UARTPrint(UARTx_BASE, helpstr);
    data->done = false;
    return 0;
  }

  switch (argv[0][0]) {
  case 'b':
    UARTPrint(UARTx_BASE, "Booting\r\n");
    data->done = true;
    data->retval = 0;
    break;
  case 'u':
    UARTPrint(UARTx_BASE, "Force update\r\n");
    data->done = true;
    data->retval = 1;
    break;
  case 'r':
    UARTPrint(UARTx_BASE, "Hard reboot\r\n");
    ROM_SysCtlReset(); // this function never returns
    break;
  case 'h':
  default:
    UARTPrint(UARTx_BASE, helpstr);
    data->done = false;
    break;
  }


  // this return value is AFAIK ignored
  return 0;
}


// check the UART, if I receive the special command within
// some period of time I force an update
// return non-zero to force an update
#define BL_USER_BUFFSZ  1
unsigned long bl_user_checkupdate_hook(void)
{
  ConfigureDevice();

  UARTPrint(UARTx_BASE, "CM MCU BOOTLOADER\r\n");
  UARTPrint(UARTx_BASE, FIRMWARE_VERSION "\r\n");


  int timeout = 1000000;
  uint32_t ui32Size = BL_USER_BUFFSZ;
  uint8_t ui8Data;
  uint8_t recvd = 0;
  //
  // Send out the number of bytes requested.
  //
  while(ui32Size--)
  {
    //
    // Wait for the FIFO to not be empty.
    //
    while ((HWREG(UARTx_BASE + UART_O_FR) & UART_FR_RXFE))
    {
      if ( timeout-- <=0 )
        break;
    }
    if ( timeout <=0) // why is this here?
      break;

    //
    // Receive a byte from the UART.
    //
    ui8Data = HWREG(UARTx_BASE + UART_O_DR); recvd++;
  }
  UARTPrint(UARTx_BASE, "after timeout\r\n");
  if ( ! recvd )
    return 0; // got nothing on the UART
  // if we received something, start the CLI
  struct bl_user_data_t rl_userdata = {
      .done = false,
      .retval = 0,
  };
  struct microrl_config rl_config = {
      .print = bl_user_uart_print, // default to front panel
      // set callback for execute
      .execute = bl_user_rl_execute,
      .prompt_str = MICRORL_PROMPT_DEFAULT,
      .prompt_length = MICRORL_PROMPT_LEN,
      .userdata = &rl_userdata,
  };
  microrl_t rl;
  microrl_init(&rl, &rl_config);
  microrl_set_execute_callback(&rl, bl_user_rl_execute);
  microrl_insert_char(&rl, ' '); // this seems to be necessary?
  microrl_insert_char(&rl, ui8Data);

  for (;;) {
    UARTReceive(&ui8Data, BL_USER_BUFFSZ);
    microrl_insert_char(&rl, ui8Data);
    if ( rl_userdata.done == true ) {
      return rl_userdata.retval;
    }
  }

  return 0;
}
#endif // BL_CHECK_UPDATE_FN_HOOK
