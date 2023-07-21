/**
 * \file ProBookFanReset.h
 */

/*-
 * Copyright (c) 2012 RehabMan
 *               2023 Ubihazard
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef __ProBookFanReset_H__
#define __ProBookFanReset_H__

// Registers of the embedded controller
#define EC_DATAPORT         (0x62)      // EC data io-port
#define EC_CTRLPORT         (0x66)      // EC control io-port

// Embedded controller status register bits
#define EC_STAT_OBF         (0x01)      // Output buffer full
#define EC_STAT_IBF         (0x02)      // Input buffer full
#define EC_STAT_CMD         (0x08)      // Last write was a command write (0=data)

// Embedded controller commands
// (write to EC_CTRLPORT to initiate read/write operation)
#define EC_CTRLPORT_READ    ((byte)0x80)
#define EC_CTRLPORT_WRITE   ((byte)0x81)
#define EC_CTRLPORT_QUERY   ((byte)0x84)

// Embedded controller offsets
#define EC_ZONE_SEL         (0x22)      // CRZN in DSDT
#define EC_ZONE_TEMP        (0x26)      // TEMP in DSDT
#define EC_FAN_CONTROL      (0x2F)      // FTGC in DSDT

#if 0
  #include "io_inline.h"
#endif

#endif
