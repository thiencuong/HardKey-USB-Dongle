/*
 * This file is part of the TREZOR project, https://trezor.io/
 *
 * Copyright (C) 2014 Pavol Rusnak <stick@satoshilabs.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdint.h>
#include "main.h"
#include "sha2.h"
/*

 flash memory layout:

   name    |          range          |  size   |     function
-----------+-------------------------+---------+------------------
 Sector  0 | 0x08000000 - 0x08003FFF |  16 KiB | bootloader code
 Sector  1 | 0x08004000 - 0x08007FFF |  16 KiB | bootloader code
-----------+-------------------------+---------+------------------
 Sector  2 | 0x08008000 - 0x0800BFFF |  16 KiB | metadata area
 Sector  3 | 0x0800C000 - 0x0800FFFF |  16 KiB | metadata area
-----------+-------------------------+---------+------------------
 Sector  4 | 0x08010000 - 0x0801FFFF |  64 KiB | application code
 Sector  5 | 0x08020000 - 0x0803FFFF | 128 KiB | application code
 Sector  6 | 0x08040000 - 0x0805FFFF | 128 KiB | application code
 Sector  7 | 0x08060000 - 0x0807FFFF | 128 KiB | application code
===========+=========================+============================
 Sector  8 | 0x08080000 - 0x0809FFFF | 128 KiB | application code
 Sector  9 | 0x080A0000 - 0x080BFFFF | 128 KiB | application code
 Sector 10 | 0x080C0000 - 0x080DFFFF | 128 KiB | application code
 Sector 11 | 0x080E0000 - 0x080FFFFF | 128 KiB | application code

 metadata area:

 offset | type/length |  description
--------+-------------+-------------------------------
 0x0000 |  4 bytes    |  magic = 'TRZR'
 0x0004 |  uint32     |  length of the code (codelen)
 0x0008 |  uint8      |  signature index #1
 0x0009 |  uint8      |  signature index #2
 0x000A |  uint8      |  signature index #3
 0x000B |  uint8      |  flags
 0x000C |  52 bytes   |  reserved
 0x0040 |  64 bytes   |  signature #1
 0x0080 |  64 bytes   |  signature #2
 0x00C0 |  64 bytes   |  signature #3
 0x0100 |  32K-256 B  |  persistent storage

 flags & 0x01 -> restore storage after flashing (if signatures are ok)

 */

#define FLASH_ORIGIN		(0x08000000)

#define FLASH_PTR(x)		(const uint8_t*) (x)

#define FLASH_BOOT_START	(FLASH_ORIGIN)
#define FLASH_BOOT_LEN		(0x6000)

#define FLASH_META_START		(FLASH_BOOT_START + FLASH_BOOT_LEN)
#define FLASH_META_LEN			(0x2000)
#define FLASH_BOOT_COMMAND  (FLASH_META_START)
#define FIRMWARE_READY 			(FLASH_BOOT_COMMAND + 2)
#define PASSWORD_SAVE 			(FIRMWARE_READY + 2)

#define FLASH_APP_START		(FLASH_META_START + FLASH_META_LEN)

#define FLASH_USER_START_ADDR  FLASH_APP_START
#define FLASH_USER_END_ADDR    (FLASH_USER_START_ADDR + 0x18000)

/* Base address of the Flash sectors */
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base address of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base address of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base address of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base address of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base address of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base address of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base address of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base address of Sector 7, 128 Kbytes */


void memory_protect(void);
void memory_write_unlock(void);
int memory_bootloader_hash(uint8_t *hash);

static inline void flash_write32(uint32_t addr, uint32_t word) 
{
	*(volatile uint32_t *) FLASH_PTR(addr) = word;
}
static inline void flash_write8(uint32_t addr, uint8_t byte) 
{
	*(volatile uint8_t *) FLASH_PTR(addr) = byte;
}

#endif
