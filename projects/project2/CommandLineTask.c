/*
 * CommandLineTask.c
 *
 *  Created on: Apr 7, 2019
 *      Author: wittich
 */



#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"

// local includes
#include "common/i2c_reg.h"
#include "common/uart.h"
#include "common/power_ctl.h"

// FreeRTOS includes
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "queue.h"

#include "FreeRTOS_CLI.h"

// Generic C headers -- caveat emptor
// todo: do I need these
#include "string.h"
#include <stdlib.h>

// TivaWare includes
#include "driverlib/uart.h"

// prototype of mutex'd print
void Print(const char* str);

// local sprintf prototype
int snprintf( char *buf, unsigned int count, const char *format, ... );

// external definition
extern QueueHandle_t xPwrQueue;


#define MAX_INPUT_LENGTH    50
#define MAX_OUTPUT_LENGTH   100

// sample commands from the demo project
void vRegisterSampleCLICommands( void );

// Ugly hack for now -- I don't understand how to reconcile these
// two parts of the FreeRTOS-Plus code w/o casts o plenty
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-sign"
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
#pragma GCC diagnostic ignored "-Wformat="

static BaseType_t readI2Creg1(char *m, size_t s, const char *mm)
{

	int8_t *p1, *p2, *p3;
	BaseType_t p1l, p2l, p3l;
  p1 = FreeRTOS_CLIGetParameter(mm, 1, &p1l);
  p2 = FreeRTOS_CLIGetParameter(mm, 2, &p2l);
  p3 = FreeRTOS_CLIGetParameter(mm, 3, &p3l);
  p1[p1l] = 0x00; // terminate strings
  p2[p2l] = 0x00; // terminate strings
  p3[p3l] = 0x00; // terminate strings

  BaseType_t i1, i2, i3;
  i1 = strtol(p1, NULL, 16);
  i2 = strtol(p2, NULL, 16);
  i3 = strtol(p3, NULL, 16);
  uint8_t data[10];
  memset(data,0,10);
  if ( i3 > 10 ) i3 = 10;
  char tmp[64];
  snprintf(tmp, 64, "readI2CReg1: command arguments are: %x %x %x\n", i1, i2, i3);
  Print(tmp);
  readI2Creg(I2C1_BASE, i1, i2, data, i3);
  // should actually do something here
  snprintf(m, s, "Read: %d %d %d\n", data[0], data[1], data[2]);
	return pdFALSE;
}

// send power control commands
static BaseType_t power_ctl(char *m, size_t s, const char *mm)
{
  int8_t *p1;
  BaseType_t p1l;
  p1 = FreeRTOS_CLIGetParameter(mm, 1, &p1l);
  p1[p1l] = 0x00; // terminate strings
  BaseType_t i1 = strtol(p1, NULL, 10);

  uint32_t message;
  if ( !(i1 == 1 || i1 == 0 )) {
    snprintf(m, s, "power_ctl: invalid argument %d received\n", i1);
    return pdFALSE;
  }
  else if ( i1 == 1 ) {
    message = PS_ON; // turn on power supply
  }
  else if ( i1 == 0 ) {
    message = PS_OFF; // turn off power supply
  }
  xQueueSendToBack(xPwrQueue, &message, pdMS_TO_TICKS(10));

  return pdFALSE;
}


#pragma GCC diagnostic pop


static const char * const pcWelcomeMessage =
		"FreeRTOS command server.\r\nType Help to view a list of registered commands.\r\n";

CLI_Command_Definition_t i2c_read_command = {
		.pcCommand="i2cr",
		.pcHelpString="i2cr <address> <reg> <number of bytes>\n Read no1 I2C controller.\r\n",
		.pxCommandInterpreter = readI2Creg1,
		3
};

CLI_Command_Definition_t pwr_ctl_command = {
    .pcCommand="pwr",
    .pcHelpString="pwr (0|1)\n Turn on or off all power.\r\n",
    .pxCommandInterpreter = power_ctl,
    1
};


extern StreamBufferHandle_t xStreamBuffer;


void vCommandLineTask( void *pvParameters )
{
  // todo: echo what you typed
  uint8_t cRxedChar, cInputIndex = 0;
  BaseType_t xMoreDataToFollow;
  /* The input and output buffers are declared static to keep them off the stack. */
  static char pcOutputString[ MAX_OUTPUT_LENGTH ], pcInputString[ MAX_INPUT_LENGTH ];


  // register the commands
  FreeRTOS_CLIRegisterCommand(&i2c_read_command);
  FreeRTOS_CLIRegisterCommand( &pwr_ctl_command);

  // register sample commands
  vRegisterSampleCLICommands();

  /* Send a welcome message to the user knows they are connected. */
  Print(pcWelcomeMessage);

  for( ;; ) {
    /* This implementation reads a single character at a time.  Wait in the
        Blocked state until a character is received. */
    xStreamBufferReceive(xStreamBuffer, &cRxedChar, 1, portMAX_DELAY);
    UARTCharPut(CLI_UART, cRxedChar); // TODO this should use the Mutex

    if( cRxedChar == '\n' ) {
      /* A newline character was received, so the input command string is
            complete and can be processed.  Transmit a line separator, just to
            make the output easier to read. */
      Print("\r\n");

      snprintf(pcOutputString, MAX_OUTPUT_LENGTH, "\nCalling command %s\n",
          (const char*)pcInputString);

      Print((const char*)pcOutputString);
      /* The command interpreter is called repeatedly until it returns
            pdFALSE.  See the "Implementing a command" documentation for an
            explanation of why this is. */
      do {
        /* Send the command string to the command interpreter.  Any
                output generated by the command interpreter will be placed in the
                pcOutputString buffer. */
        xMoreDataToFollow = FreeRTOS_CLIProcessCommand
            (
                (const char*)pcInputString,   /* The command string.*/
                (char*)pcOutputString,  /* The output buffer. */
                MAX_OUTPUT_LENGTH/* The size of the output buffer. */
            );

        /* Write the output generated by the command interpreter to the
                console. */
        Print((char*)pcOutputString);

      } while( xMoreDataToFollow != pdFALSE );

      /* All the strings generated by the input command have been sent.
            Processing of the command is complete.  Clear the input string ready
            to receive the next command. */
      cInputIndex = 0;
      memset( pcInputString, 0x00, MAX_INPUT_LENGTH );
      Print("% ");
    }
    else {
      /* The if() clause performs the processing after a newline character
            is received.  This else clause performs the processing if any other
            character is received. */

      if( cRxedChar == '\r' ) {
        /* Ignore carriage returns. */
      }
      else if( cRxedChar == '\b' ) {
        /* Backspace was pressed.  Erase the last character in the input
                buffer - if there are any. */
        if( cInputIndex > 0 ) {
          cInputIndex--;
          pcInputString[ cInputIndex ] = '\0';
        }
      }
      else {
        /* A character was entered.  It was not a new line, backspace
                or carriage return, so it is accepted as part of the input and
                placed into the input buffer.  When a \n is entered the complete
                string will be passed to the command interpreter. */
        if( cInputIndex < MAX_INPUT_LENGTH ) {
          pcInputString[ cInputIndex ] = cRxedChar;
          cInputIndex++;
        }
      }
    }
  }
}

