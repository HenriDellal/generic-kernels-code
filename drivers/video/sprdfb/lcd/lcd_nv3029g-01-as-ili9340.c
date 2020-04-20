/*
 * Support for ili9340 LCD device
 *
 * Copyright (C) 2010 Spreadtrum
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include "../sprdfb_panel.h"

static const uint8_t init_data[] = {
	0, 6, 0xBC, 0x00, 0x18, 0x00, 0x10, 0x0B,
	0, 2, 0xD6, 0x01,
	0, 8, 0xB7, 0xFF, 0x44, 0x04, 0x44, 0x04, 0x02, 0x04,
	0, 4, 0xBB, 0x03, 0x66, 0x33,
	0, 4, 0xCD, 0x26, 0x26, 0x00,
	0, 8, 0xEA, 0x01, 0x22, 0x3F, 0x82, 0x04, 0x00, 0x00,
	0, 2, 0xB4, 0x80,
	0, 4, 0xBA, 0x13, 0x1A, 0x21,
	0, 6, 0xE8, 0x11, 0x11, 0x33, 0x33, 0x55,
	0, 10, 0xE9, 0x40, 0x84, 0x65, 0x70, 0xC0, 0x00, 0xFF, 0x33, 0x88,
	0, 2, 0xF5, 0x00,
	0, 3, 0xF2, 0x00, 0x00,
	0, 16, 0xE4, 0x00, 0x02, 0x09, 0x06, 0x15, 0x19, 0x3F, 0x57, 0x4E,
		0x05, 0x0E, 0x0C, 0x21, 0x23, 0x0D,
	0, 16, 0xE5, 0x00, 0x02, 0x08, 0x07, 0x14, 0x19, 0x3E, 0x47, 0x4F,
		0x05, 0x0D, 0x0A, 0x20, 0x23, 0x0D,
	0, 2, 0x35, 0x00,
	0, 3, 0x44, 0x00, 0x8F,
	0, 2, 0x3A, 0x55,
	120, 1, 0x11, // SLPOUT
	20, 1, 0x29, // DISPON
	0, 1, 0x2C // RAMWR
};

static int32_t ili9340_init(struct panel_spec *self)
{
	const uint8_t *init = init_data;
	unsigned int ms, count;
	send_data_t send_cmd = self->info.mcu->ops->send_cmd;
	send_data_t send_data = self->info.mcu->ops->send_data;

	while (init < init_data + sizeof(init_data)) {
		ms = *init++;
		count = *init++;
		send_cmd(*init++);
		while (--count) send_data(*init++);
		if (ms) mdelay(ms);
	}
	return 0;
}

static int32_t ili9340_set_window(struct panel_spec *self,
		uint16_t left, uint16_t top, uint16_t right, uint16_t bottom)
{
	send_data_t send_cmd = self->info.mcu->ops->send_cmd;
	send_data_t send_data = self->info.mcu->ops->send_data;

	send_cmd(0x002A); // col
	send_data(left);
	send_data(right);

	send_cmd(0x002B); // row
	send_data(top);
	send_data(bottom);

	send_cmd(0x002C);

	return 0;
}

static int32_t ili9340_invalidate(struct panel_spec *self)
{
	return self->ops->panel_set_window(self, 0, 0,
			self->width - 1, self->height - 1);
}

static int32_t ili9340_invalidate_rect(struct panel_spec *self,
				uint16_t left, uint16_t top,
				uint16_t right, uint16_t bottom)
{
	left 	= (left   >= self->width)  ? (self->width - 1) : left;
	right 	= (right  >= self->width)  ? (self->width - 1) : right;
	top 	= (top    >= self->height) ? (self->height - 1) : top;
	bottom 	= (bottom >= self->height) ? (self->height - 1) : bottom;

	return self->ops->panel_set_window(self, left, top,
			right, bottom);
}

static int32_t ili9340_set_direction(struct panel_spec *self, uint16_t direction)
{
	send_cmd_data_t send_cmd_data = self->info.mcu->ops->send_cmd_data;

	switch (direction) {
	case LCD_DIRECT_NORMAL:
		send_cmd_data(0x0036, 0x09);
		break;
	case LCD_DIRECT_ROT_90:
		send_cmd_data(0x0036, 0x00E8);
		break;
	case LCD_DIRECT_ROT_180:
	case LCD_DIRECT_MIR_HV:
		send_cmd_data(0x0036, 0x0008);
		break;
	case LCD_DIRECT_ROT_270:
		send_cmd_data(0x001E, 0x0028);
		break;
	case LCD_DIRECT_MIR_H:
		send_cmd_data(0x0036, 0x0088);
		break;
	case LCD_DIRECT_MIR_V:
		send_cmd_data(0x0036, 0x0048);
		break;
	default:
		printk("unknown lcd direction!\n");
		send_cmd_data(0x0036, 0x00C8);
		direction = LCD_DIRECT_NORMAL;
		break;
	}

	self->direction = direction;

	return 0;
}

static int32_t ili9340_enter_sleep(struct panel_spec *self, uint8_t is_sleep)
{
	if ( is_sleep ) {
		self->info.mcu->ops->send_cmd(0x0028);
		mdelay(120);
		self->info.mcu->ops->send_cmd(0x0010);
		mdelay(200);
	}
	else {
		self->ops->panel_reset(self);
		self->ops->panel_init(self);
	}
	return 0;
}

static uint32_t ili9340_read_id(struct panel_spec *self)
{
	return 0x9340;
}

static struct panel_operations lcd_ili9340_operations = {
	.panel_init            = ili9340_init,
	.panel_set_window      = ili9340_set_window,
	.panel_invalidate      = ili9340_invalidate,
	.panel_invalidate_rect = ili9340_invalidate_rect,
	.panel_set_direction   = ili9340_set_direction,
	.panel_enter_sleep     = ili9340_enter_sleep,
	.panel_readid          = ili9340_read_id,
};

static struct timing_mcu lcd_ili9340_timing[] = {
	[MCU_LCD_REGISTER_TIMING] = {
		.rcss = 45,
		.rlpw = 60,
		.rhpw = 100,
		.wcss = 30,
		.wlpw = 45,
		.whpw = 45,
	},
	[MCU_LCD_GRAM_TIMING] = {
		.rcss = 25,
		.rlpw = 70,
		.rhpw = 70,
		.wcss = 30,
		.wlpw = 45,
		.whpw = 45,
	}
};

static struct info_mcu lcd_ili9340_info = {
	.bus_mode = LCD_BUS_8080,
	.bus_width = 8,
	.bpp = 16,
	.timing = &lcd_ili9340_timing,
	.ops = NULL,
};

struct panel_spec lcd_panel_ili9340 = {
	.width = 240,
	.height = 320,
	.fps = 60,
	.suspend_mode = SEND_SLEEP_CMD,
	.type = LCD_MODE_MCU,
	.direction = LCD_DIRECT_NORMAL,
	.info = { .mcu = &lcd_ili9340_info },
	.ops = &lcd_ili9340_operations,
};

struct panel_cfg lcd_ili9340 = {
	.dev_id = SPRDFB_MAINLCD_ID,
	.lcd_id = 0x9340,
	.lcd_name = "lcd_ili9340",
	.panel = &lcd_panel_ili9340,
};

static int __init lcd_ili9340_init(void)
{
	return sprdfb_panel_register(&lcd_ili9340);
}

subsys_initcall(lcd_ili9340_init);
