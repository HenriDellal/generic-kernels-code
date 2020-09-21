/* drivers/video/sprdfb/lcd/lcd_st7796s_mipi.c
 *
 * Support for ST7796S mipi LCD device
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
#include <linux/bug.h>
#include <linux/delay.h>
#include "../sprdfb_panel.h"

#define MAX_DATA   56

typedef struct LCM_Init_Code_tag {
	unsigned int tag;
	unsigned char data[MAX_DATA];
} LCM_Init_Code;

#define LCM_TAG_SHIFT 24
#define LCM_TAG_MASK  ((1 << 24) - 1)
#define LCM_SEND(len) ((1 << LCM_TAG_SHIFT) | len)
#define LCM_SLEEP(ms) ((2 << LCM_TAG_SHIFT) | ms)
#define LCM_TAG_SEND  (1 << 0)
#define LCM_TAG_SLEEP (1 << 1)
//#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

static LCM_Init_Code init_data[] = {
	{LCM_SEND(2), {0x03,0x00}},
	{LCM_SEND(1), {0x11}},
	{LCM_SLEEP(120)},
	{LCM_SEND(2), {0xF0,0xC3}},
	{LCM_SEND(2), {0xF0,0x96}},
	{LCM_SEND(6), {4,0,0xEC,0x00,0x00,0x01}},
	{LCM_SEND(2), {0xB4,0x00}},
	{LCM_SEND(2), {0xB7,0xC6}},
	{LCM_SEND(5), {3,0,0xB1,0xB0,0x10}},
	{LCM_SEND(2), {0xB9,0x02}},
	{LCM_SEND(11), {9,0,0xE8,0x40,0x8A,0x00,0x00,0x29,0x19,0xA5,0x33}},
	{LCM_SEND(5), {3,0,0xC0,0x80,0x20}},
	{LCM_SEND(2), {0xC1,0x12}},
	{LCM_SEND(2), {0xC2,0xA7}},
	{LCM_SEND(2), {0xC5,0x1D}},
	{LCM_SEND(17), {15,0,0xE0,0xF0,0x09,0x0B,0x06,0x04,0x15,0x2F,0x54,0x42,0x3C,0x17,0x14,0x18,0x1B}},
	{LCM_SEND(17), {15,0,0xE1,0xF0,0x09,0x0B,0x06,0x04,0x03,0x2D,0x43,0x42,0x3B,0x16,0x14,0x17,0x1B}},
	{LCM_SEND(2), {0xF0,0x3C}},
	{LCM_SEND(2), {0xF0,0x69}},
	{LCM_SEND(2), {0x36,0x48}},
	{LCM_SEND(2), {0x3A,0x77}},
	{LCM_SEND(1), {0x29}},
	{LCM_SLEEP(120)},
};

static LCM_Init_Code sleep_in[] = {
	{LCM_SEND(1), {0x28}},
	{LCM_SLEEP(50)},
	{LCM_SEND(1), {0x10}},
	{LCM_SLEEP(120)},
};

static LCM_Init_Code sleep_out[] = {
	{LCM_SEND(1), {0x11}},
	{LCM_SLEEP(120)},
	{LCM_SEND(1), {0x29}},
	{LCM_SLEEP(50)},
};

static int32_t st7796s_mipi_init(struct panel_spec *self)
{
	int32_t i = 0;
	LCM_Init_Code *init = init_data;

	mipi_set_cmd_mode_t mipi_set_cmd_mode = self->info.mipi->ops->mipi_set_cmd_mode;
	mipi_dcs_write_t mipi_dcs_write = self->info.mipi->ops->mipi_dcs_write;
	mipi_eotp_set_t mipi_eotp_set = self->info.mipi->ops->mipi_eotp_set;

	printk("st7796s mipi init\n");

	mipi_set_cmd_mode();
	mipi_eotp_set(0x01, 0x00);

	for (i = 0; i < ARRAY_SIZE(init_data); i++) {
		if (init->tag & (LCM_TAG_SEND << LCM_TAG_SHIFT)) {
			mipi_dcs_write(init->data, (init->tag & LCM_TAG_MASK));
			udelay(20);
		}
		else if (init->tag & (LCM_TAG_SLEEP << LCM_TAG_SHIFT)) {
			msleep((init->tag & LCM_TAG_MASK));
		}
		init++;
	}
	mipi_eotp_set(0x01, 0x01);
	return 0;
}

static uint32_t st7796s_readid(struct panel_spec *self)
{
	return 0x7796;
}

static int32_t st7796s_enter_sleep(struct panel_spec *self, uint8_t is_sleep)
{
	int32_t i;
	LCM_Init_Code *sleep_in_out = NULL;
	unsigned int tag;
	int32_t size = 0;

	mipi_set_cmd_mode_t mipi_set_cmd_mode = self->info.mipi->ops->mipi_set_cmd_mode;
	mipi_dcs_write_t mipi_dcs_write = self->info.mipi->ops->mipi_dcs_write;
	mipi_eotp_set_t mipi_eotp_set = self->info.mipi->ops->mipi_eotp_set;

	printk("kernel st7796s_enter_sleep, is_sleep = %d\n", is_sleep);

	if(is_sleep){
		sleep_in_out = sleep_in;
		size = ARRAY_SIZE(sleep_in);
	}else{
		sleep_in_out = sleep_out;
		size = ARRAY_SIZE(sleep_out);
	}

	mipi_set_cmd_mode();
	mipi_eotp_set(0x01,0x00);

	for(i = 0; i <size ; i++){
		tag = (sleep_in_out->tag >>24);
		if(tag & LCM_TAG_SEND){
			mipi_dcs_write(sleep_in_out->data, (sleep_in_out->tag & LCM_TAG_MASK));
		}else if(tag & LCM_TAG_SLEEP){
			msleep(sleep_in_out->tag & LCM_TAG_MASK);
		}
		sleep_in_out++;
	}
	mipi_eotp_set(0x01,0x00);

	return 0;
}

static struct panel_operations lcd_st7796s_mipi_operations = {
	.panel_init = st7796s_mipi_init,
	.panel_readid = st7796s_readid,
	.panel_enter_sleep = st7796s_enter_sleep,
};

static struct timing_rgb lcd_st7796s_mipi_timing = {
	.hfp = 45,
	.hbp = 45,
	.hsync = 9,
	.vfp = 20,
	.vbp = 20,
	.vsync = 10,
};

static struct info_mipi lcd_st7796s_mipi_info = {
	.work_mode = SPRDFB_MIPI_MODE_VIDEO,
	.video_bus_width = 24,
	.lan_number = 1,
	.phy_feq = 370000,
	.h_sync_pol = SPRDFB_POLARITY_POS,
	.v_sync_pol = SPRDFB_POLARITY_POS,
	.de_pol = SPRDFB_POLARITY_POS,
	.te_pol = SPRDFB_POLARITY_POS,
	.color_mode_pol = SPRDFB_POLARITY_NEG,
	.shut_down_pol = SPRDFB_POLARITY_NEG,
	.timing = &lcd_st7796s_mipi_timing,
	.ops = NULL,
};

struct panel_spec lcd_st7796s_mipi_spec = {
	.width = 320,
	.height = 480,
	.reset_timing = {5, 40, 20},
	.is_clean_lcd = true,
	.fps = 60,
	.type = SPRDFB_PANEL_TYPE_MIPI,
	.direction = LCD_DIRECT_NORMAL,
	.info = {
		.mipi = &lcd_st7796s_mipi_info
	},
	.ops = &lcd_st7796s_mipi_operations,
};

struct panel_cfg lcd_st7796s_mipi = {
	.dev_id = SPRDFB_MAINLCD_ID,
	.lcd_id = 0x7796,
	.lcd_name = "lcd_st7796s_mipi",
	.panel = &lcd_st7796s_mipi_spec,
};

static int __init lcd_st7796s_mipi_init(void)
{
	return sprdfb_panel_register(&lcd_st7796s_mipi);
}

subsys_initcall(lcd_st7796s_mipi_init);
