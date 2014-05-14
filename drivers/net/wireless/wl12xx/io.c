/*
 * This file is part of wl1271
 *
 * Copyright (C) 2008-2010 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/interrupt.h>

#include "wl12xx.h"
#include "debug.h"
#include "wl12xx_80211.h"
#include "io.h"
#include "tx.h"

#define OCP_CMD_LOOP  32

#define OCP_CMD_WRITE 0x1
#define OCP_CMD_READ  0x2

#define OCP_READY_MASK  BIT(18)
#define OCP_STATUS_MASK (BIT(16) | BIT(17))

#define OCP_STATUS_NO_RESP    0x00000
#define OCP_STATUS_OK         0x10000
#define OCP_STATUS_REQ_FAILED 0x20000
#define OCP_STATUS_RESP_ERROR 0x30000

struct wl1271_partition_set wl12xx_part_table[PART_TABLE_LEN] = {
	[PART_DOWN] = {
		.mem = {
			.start = 0x00000000,
			.size  = 0x000177c0
		},
		.reg = {
			.start = REGISTERS_BASE,
			.size  = 0x00008800
		},
		.mem2 = {
			.start = 0x00000000,
			.size  = 0x00000000
		},
		.mem3 = {
			.start = 0x00000000,
			.size  = 0x00000000
		},
	},

	[PART_WORK] = {
		.mem = {
			.start = 0x00040000,
			.size  = 0x00014fc0
		},
		.reg = {
			.start = REGISTERS_BASE,
			.size  = 0x0000a000
		},
		.mem2 = {
			.start = 0x003004f8,
			.size  = 0x00000004
		},
		.mem3 = {
			.start = 0x00040404,
			.size  = 0x00000000
		},
	},

	[PART_DRPW] = {
		.mem = {
			.start = 0x00040000,
			.size  = 0x00014fc0
		},
		.reg = {
			.start = DRPW_BASE,
			.size  = 0x00006000
		},
		.mem2 = {
			.start = 0x00000000,
			.size  = 0x00000000
		},
		.mem3 = {
			.start = 0x00000000,
			.size  = 0x00000000
		}
	}
};

bool wl1271_set_block_size(struct wl1271 *wl)
{
	if (wl->if_ops->set_block_size) {
		wl->if_ops->set_block_size(wl->dev, WL12XX_BUS_BLOCK_SIZE);
		return true;
	}

	return false;
}

void wl1271_disable_interrupts(struct wl1271 *wl)
{
	disable_irq(wl->irq);
}

void wl1271_enable_interrupts(struct wl1271 *wl)
{
	enable_irq(wl->irq);
}

int wl1271_set_partition(struct wl1271 *wl,
			 struct wl1271_partition_set *p)
{
	
	memcpy(&wl->part, p, sizeof(*p));

	wl1271_debug(DEBUG_SPI, "mem_start %08X mem_size %08X",
		     p->mem.start, p->mem.size);
	wl1271_debug(DEBUG_SPI, "reg_start %08X reg_size %08X",
		     p->reg.start, p->reg.size);
	wl1271_debug(DEBUG_SPI, "mem2_start %08X mem2_size %08X",
		     p->mem2.start, p->mem2.size);
	wl1271_debug(DEBUG_SPI, "mem3_start %08X mem3_size %08X",
		     p->mem3.start, p->mem3.size);

	
	wl1271_raw_write32(wl, HW_PART0_START_ADDR, p->mem.start);
	wl1271_raw_write32(wl, HW_PART0_SIZE_ADDR, p->mem.size);
	wl1271_raw_write32(wl, HW_PART1_START_ADDR, p->reg.start);
	wl1271_raw_write32(wl, HW_PART1_SIZE_ADDR, p->reg.size);
	wl1271_raw_write32(wl, HW_PART2_START_ADDR, p->mem2.start);
	wl1271_raw_write32(wl, HW_PART2_SIZE_ADDR, p->mem2.size);
	wl1271_raw_write32(wl, HW_PART3_START_ADDR, p->mem3.start);

	return 0;
}
EXPORT_SYMBOL_GPL(wl1271_set_partition);

void wl1271_io_reset(struct wl1271 *wl)
{
	if (wl->if_ops->reset)
		wl->if_ops->reset(wl->dev);
}

void wl1271_io_init(struct wl1271 *wl)
{
	if (wl->if_ops->init)
		wl->if_ops->init(wl->dev);
}

void wl1271_top_reg_write(struct wl1271 *wl, int addr, u16 val)
{
	
	addr = (addr >> 1) + 0x30000;
	wl1271_write32(wl, OCP_POR_CTR, addr);

	
	wl1271_write32(wl, OCP_DATA_WRITE, val);

	
	wl1271_write32(wl, OCP_CMD, OCP_CMD_WRITE);
}

u16 wl1271_top_reg_read(struct wl1271 *wl, int addr)
{
	u32 val;
	int timeout = OCP_CMD_LOOP;

	
	addr = (addr >> 1) + 0x30000;
	wl1271_write32(wl, OCP_POR_CTR, addr);

	
	wl1271_write32(wl, OCP_CMD, OCP_CMD_READ);

	
	do {
		val = wl1271_read32(wl, OCP_DATA_READ);
	} while (!(val & OCP_READY_MASK) && --timeout);

	if (!timeout) {
		wl1271_warning("Top register access timed out.");
		return 0xffff;
	}

	
	if ((val & OCP_STATUS_MASK) == OCP_STATUS_OK)
		return val & 0xffff;
	else {
		wl1271_warning("Top register access returned error.");
		return 0xffff;
	}
}

