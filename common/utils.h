/*
 * util.h
 *
 *  Created on: May 9, 2019
 *      Author: wittich
 */

#ifndef COMMON_UTILS_H_
#define COMMON_UTILS_H_

#include <stdint.h>
#include <stdbool.h>


// write, read or toggle GPIO pin by name
void write_gpio_pin(int pin, uint8_t value);
uint8_t read_gpio_pin(int pin);
uint8_t toggle_gpio_pin(int pin);


void setupActiveLowPins(void);


// EEPROM
#define SN_ADDR 64
#define FF_ADDR (SN_ADDR+4)

void write_eeprom(uint32_t data, uint32_t addr);
uint32_t read_eeprom_single(uint32_t addr);
uint64_t read_eeprom_multi(uint32_t addr);

// EEPROM buffer
#define EBUF_MINBLK 2
#define EBUF_MAXBLK 5

#define ERRDATA_OFFSET 7	// Number of bits reserved for error data
#define ERRCODE_OFFSET 5	// Number of bits reserved for error codes
#define COUNTER_OFFSET (16-ERRDATA_OFFSET-ERRCODE_OFFSET)	// Number of bits reserved for message counter

#define ERRDATA_MASK ((1<<(ERRDATA_OFFSET))-1)
#define ERRCODE_MASK ((1<<(ERRDATA_OFFSET+ERRCODE_OFFSET))-1-ERRDATA_MASK)
#define COUNTER_MASK ((1<<(ERRDATA_OFFSET+ERRCODE_OFFSET+COUNTER_OFFSET))-1-ERRDATA_MASK-ERRCODE_MASK)

#define COUNTER_UPDATE 4	//Number of repeated entries that initiates a hardware counter update (re-write entry)

// error codes
#define EBUF_RESTART 1
#define EBUF_RESET_BUFFER 2
#define EBUF_POWER_OFF 3
#define EBUF_POWER_OFF_TEMP 4
#define EBUF_POWER_ON 5

#define EBUF_TEMP_HIGH(status) ((1<<(ERRCODE_OFFSET-1))|status)
#define EBUF_TEMP_NORMAL EBUF_TEMP_HIGH(0)

typedef struct error_buffer_t error_buffer_t;
typedef error_buffer_t* errbuf_handle_t;

extern errbuf_handle_t ebuf;

void errbuffer_init(errbuf_handle_t ebuf, uint8_t minblk, uint8_t maxblk);
void errbuffer_reset(errbuf_handle_t ebuf);

void errbuffer_put(errbuf_handle_t ebuf, uint16_t errcode, uint16_t errdata);
void errbuffer_get(errbuf_handle_t ebuf, uint32_t num, uint32_t (*arrptr)[num]);

uint32_t errbuffer_capacity(errbuf_handle_t ebuf);
uint32_t errbuffer_minaddr(errbuf_handle_t ebuf);
uint32_t errbuffer_maxaddr(errbuf_handle_t ebuf);
uint32_t errbuffer_head(errbuf_handle_t ebuf);
uint16_t errbuffer_last(errbuf_handle_t ebuf);
uint16_t errbuffer_counter(errbuf_handle_t ebuf);

uint32_t errbuffer_entry(uint16_t errcode, uint16_t errdata);

#endif /* COMMON_UTILS_H_ */
