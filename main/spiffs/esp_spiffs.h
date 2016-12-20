/*
 * Lua RTOS, write syscall implementation
 *
 * Copyright (C) 2015 - 2016
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
 * 
 * Author: Jaume Olivé (jolive@iberoxarxa.com / jolive@whitecatboard.org)
 * 
 * All rights reserved.  
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#ifndef __ESP_SPIFFS_H__
#define __ESP_SPIFFS_H__

#include "spiffs.h"

#if defined(__cplusplus)
extern "C" {
#endif
	
	int esp32_spi_flash_read(unsigned int addr, unsigned int size, unsigned char *dst);
	int esp32_spi_flash_write(unsigned int addr, unsigned int size, unsigned char *src);
	int esp32_spi_flash_erase(unsigned int addr, unsigned int size);

	#define low_spiffs_read  (spiffs_read  *)esp32_spi_flash_read
	#define low_spiffs_write (spiffs_write *)esp32_spi_flash_write
	#define low_spiffs_erase (spiffs_erase *)esp32_spi_flash_erase

#if defined(__cplusplus)
	}
#endif

#endif  // __ESP_SPIFFS_H__
